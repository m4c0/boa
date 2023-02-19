import casein;
import hai;
import sires;
import traits;
import vee;

struct point {
  float x;
  float y;
  float z;
  float w;
};

struct device_stuff {
  casein::native_handle_t nptr;

  vee::instance i = vee::create_instance("my-app");
  vee::debug_utils_messenger dbg = vee::create_debug_utils_messenger();
  vee::surface s = vee::create_surface(nptr);
  vee::physical_device_pair pdqf =
      vee::find_physical_device_with_universal_queue(*s);
  vee::device d =
      vee::create_single_queue_device(pdqf.physical_device, pdqf.queue_family);

  vee::descriptor_set_layout dsl =
      vee::create_descriptor_set_layout<vee::dsl_fragment_uniform>();

  vee::queue q = vee::get_queue_for_family(pdqf.queue_family);
};

struct extent_stuff {
  vee::physical_device pd;
  vee::surface &s;
  unsigned qf;

  vee::extent extent = vee::get_surface_capabilities(pd, *s).currentExtent;

  vee::command_pool cp = vee::create_command_pool(qf);
  vee::render_pass rp = vee::create_render_pass(pd, *s);
  vee::swapchain swc = vee::create_swapchain(pd, *s);

  vee::pipeline_layout pl = vee::create_pipeline_layout();

  vee::shader_module vert =
      vee::create_shader_module_from_resource("poc.vert.spv");
  vee::shader_module frag =
      vee::create_shader_module_from_resource("poc.frag.spv");
  vee::gr_pipeline gp =
      vee::create_graphics_pipeline(*pl, *rp,
                                    {
                                        vee::pipeline_vert_stage(*vert, "main"),
                                        vee::pipeline_frag_stage(*frag, "main"),
                                    },
                                    {
                                        vee::vertex_input_bind(sizeof(point)),
                                    },
                                    {
                                        vee::vertex_attribute_vec2(0, 0),
                                    });

  vee::buffer v_buf = vee::create_vertex_buffer(sizeof(point) * 3);
  vee::device_memory v_mem = vee::create_host_buffer_memory(pd, *v_buf);
  decltype(nullptr) v_bind = vee::bind_buffer_memory(*v_buf, *v_mem);

  vee::image d_img = vee::create_depth_image(pd, *s);
  vee::device_memory d_mem = vee::create_local_image_memory(pd, *d_img);
  decltype(nullptr) d_bind = vee::bind_image_memory(*d_img, *d_mem);
  vee::image_view d_iv = vee::create_depth_image_view(*d_img);
};

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

  vee::command_buffer cb = vee::allocate_primary_command_buffer(*xs->cp);

  vee::fb_params fbp{
      .physical_device = xs->pd,
      .surface = *xs->s,
      .render_pass = *xs->rp,
      .image_buffer = *iv,
      .depth_buffer = *xs->d_iv,
  };
  vee::framebuffer fb = vee::create_framebuffer(fbp);
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
    state = sires::open("poc.vert.spv")
                .map([](auto &&) { return setup_stuff; })
                .unwrap(failed_to_start);
    break;
  case casein::REPAINT:
    switch (state) {
    case setup_stuff: {
      const auto &[pd, qf] = dev->pdqf;
      ext = hai::uptr<extent_stuff>::make(pd, dev->s, qf);
      infs = hai::uptr<inflights>::make(qf);

      auto imgs = vee::get_swapchain_images(*ext->swc);
      frms = decltype(frms)::make(imgs.size());
      for (auto i = 0; i < imgs.size(); i++) {
        auto img = (imgs.data())[i];
        vee::image_view iv = vee::create_rgba_image_view(img, ext->pd, *ext->s);
        (*frms)[i] = hai::uptr<frame_stuff>::make(&*ext, traits::move(iv));
      }

      vee::map_memory<point>(*ext->v_mem, [](auto *vs) {
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

        auto idx = vee::acquire_next_image(*ext->swc, *inf.img_available_sema);
        auto &frame = (*frms)[idx];

        {
          vee::begin_cmd_buf_render_pass_continue(inf.cb, *ext->rp);
          vee::cmd_set_scissor(inf.cb, ext->extent);
          vee::cmd_set_viewport(inf.cb, ext->extent);
          vee::cmd_bind_gr_pipeline(inf.cb, *ext->gp);
          vee::cmd_bind_vertex_buffers(inf.cb, 0, *ext->v_buf);
          vee::cmd_draw(inf.cb, 3);
          vee::end_cmd_buf(inf.cb);
        }
        {
          vee::begin_cmd_buf_one_time_submit(frame->cb);
          vee::cmd_begin_render_pass({
              .command_buffer = frame->cb,
              .render_pass = *ext->rp,
              .framebuffer = *frame->fb,
              .extent = ext->extent,
          });
          vee::cmd_execute_command(frame->cb, inf.cb);
          vee::cmd_end_render_pass(frame->cb);
          vee::end_cmd_buf(frame->cb);
        }

        vee::queue_submit({
            .queue = dev->q,
            .fence = *inf.f,
            .command_buffer = frame->cb,
            .wait_semaphore = *inf.img_available_sema,
            .signal_semaphore = *inf.rnd_finished_sema,
        });
        vee::queue_present({
            .queue = dev->q,
            .swapchain = *ext->swc,
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
