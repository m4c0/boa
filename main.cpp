import boa;
import casein;
import hai;
import sires;
import traits;
import vee;

using device_stuff = boa::vulkan::per_device;
using extent_stuff = boa::vulkan::per_extent;

struct inflight_stuff {
  vee::command_pool *pool;
  vee::command_buffer cb = vee::allocate_secondary_command_buffer(**pool);

  vee::semaphore img_available_sema = vee::create_semaphore();
  vee::semaphore rnd_finished_sema = vee::create_semaphore();
  vee::fence f = vee::create_fence_signaled();
};
struct inflights {
  unsigned qf;

  vee::command_pool cp = vee::create_command_pool(qf);

  inflight_stuff front{&cp};
  inflight_stuff back{&cp};
};

struct frame_stuff {
  const extent_stuff *xs;
  vee::image_view iv;

  vee::command_buffer cb =
      vee::allocate_primary_command_buffer(xs->command_pool());
  vee::framebuffer fb = xs->create_framebuffer(iv);
};

inline void flip(inflights &i) {
  auto tmp = traits::move(i.front);
  i.front = traits::move(i.back);
  i.back = traits::move(tmp);
}

enum states {
  waiting_nptr,
  setup_stuff,
  ready_to_paint,
  done,
  failed_to_start,
};

extern "C" void casein_handle(const casein::event &e) {
  static volatile casein::native_handle_t nptr{};
  static hai::uptr<device_stuff> dev{};
  static hai::uptr<extent_stuff> ext{};
  static hai::uptr<inflights> infs{};
  static hai::holder<hai::uptr<frame_stuff>[]> frms{};
  static states state = waiting_nptr;

  switch (e.type()) {
  case casein::CREATE_WINDOW:
    switch (state) {
    case waiting_nptr:
      vee::initialise();
      nptr = e.as<casein::events::create_window>().native_window_handle();
      dev = hai::uptr<device_stuff>::make(nptr);
      break;
    default:
      break;
    }
    state = sires::open("main.vert.spv")
                .map([](auto &&) { return setup_stuff; })
                .unwrap(failed_to_start);
    break;
  case casein::REPAINT:
    switch (state) {
    case setup_stuff: {
      ext = hai::uptr<extent_stuff>::make(&*dev);
      infs = hai::uptr<inflights>::make(dev->queue_family());

      auto imgs = vee::get_swapchain_images(ext->swapchain());
      frms = decltype(frms)::make(imgs.size());
      for (auto i = 0; i < imgs.size(); i++) {
        auto img = (imgs.data())[i];
        vee::image_view iv = vee::create_rgba_image_view(
            img, dev->physical_device(), dev->surface());
        (*frms)[i] = hai::uptr<frame_stuff>::make(&*ext, traits::move(iv));
      }

      ext->map_vertices([](auto *vs) {
        vs[0] = {-1, -1};
        vs[1] = {0, 1};
        vs[2] = {1, -1};
      });

      state = ready_to_paint;
      break;
    }
    case ready_to_paint: {
      try {
        flip(*infs);

        auto &inf = infs->back;
        vee::wait_and_reset_fence(*inf.f);

        auto idx =
            vee::acquire_next_image(ext->swapchain(), *inf.img_available_sema);
        auto &frame = (*frms)[idx];

        {
          ext->begin_secondary_cmdbuf(inf.cb);
          vee::cmd_draw(inf.cb, 3);
          vee::end_cmd_buf(inf.cb);
        }
        {
          vee::begin_cmd_buf_one_time_submit(frame->cb);
          vee::cmd_begin_render_pass({
              .command_buffer = frame->cb,
              .render_pass = ext->render_pass(),
              .framebuffer = *frame->fb,
              .extent = ext->extent_2d(),
          });
          vee::cmd_execute_command(frame->cb, inf.cb);
          vee::cmd_end_render_pass(frame->cb);
          vee::end_cmd_buf(frame->cb);
        }

        vee::queue_submit({
            .queue = dev->queue(),
            .fence = *inf.f,
            .command_buffer = frame->cb,
            .wait_semaphore = *inf.img_available_sema,
            .signal_semaphore = *inf.rnd_finished_sema,
        });
        vee::queue_present({
            .queue = dev->queue(),
            .swapchain = ext->swapchain(),
            .wait_semaphore = *inf.rnd_finished_sema,
            .image_index = idx,
        });
      } catch (vee::out_of_date_error) {
        state = setup_stuff;
        vee::device_wait_idle();
      }
      break;
    }
    default:
      break;
    }
    break;
  case casein::QUIT:
    vee::device_wait_idle();
    frms = {};
    infs = {};
    ext = {};
    dev = {};
    state = done;
    break;
  default:
    break;
  }
}
