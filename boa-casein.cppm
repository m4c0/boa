export module boa:casein;
import :ecs_objects;
import :vulkan;
import casein;
import hai;
import sires;
import traits;
import vee;

using device_stuff = boa::vulkan::per_device;
using extent_stuff = boa::vulkan::per_extent;
using inflight_stuff = boa::vulkan::per_inflight;
using inflights = boa::vulkan::inflight_pair;
using frame_stuff = boa::vulkan::per_frame;

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
      infs = hai::uptr<inflights>::make(&*dev);

      auto imgs = vee::get_swapchain_images(ext->swapchain());
      frms = decltype(frms)::make(imgs.size());
      for (auto i = 0; i < imgs.size(); i++) {
        auto img = (imgs.data())[i];
        (*frms)[i] = hai::uptr<frame_stuff>::make(&*dev, &*ext, img);
      }

      // TODO: return number of vertices, store in pipeline
      ext->map_vertices([](auto *vs) {
        vs[0] = {-1, -1};
        vs[1] = {1, 1};
        vs[2] = {1, -1};

        vs[3] = {1, 1};
        vs[4] = {-1, -1};
        vs[5] = {-1, 1};
        return 6;
      });

      state = ready_to_paint;
      break;
    }
    case ready_to_paint: {
      try {
        auto &inf = infs->flip();

        auto idx = inf.wait_and_takeoff(&*ext);

        ext->build_pipeline(inf.command_buffer());

        inf.submit(&*dev, (*frms)[idx]->one_time_submit([&inf](auto cb) {
          vee::cmd_execute_command(cb, inf.command_buffer());
        }));
        vee::queue_present({
            .queue = dev->queue(),
            .swapchain = ext->swapchain(),
            .wait_semaphore = inf.render_finished_sema(),
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
