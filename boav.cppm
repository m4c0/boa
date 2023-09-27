export module boav;
import casein;
import hai;
import sith;
import vee;

class thread : public sith::thread {
  casein::native_handle_t m_nptr;

public:
  void start(casein::native_handle_t n) {
    m_nptr = n;
    sith::thread::start();
  }

  void run() override {
    // Instance
    vee::instance i = vee::create_instance("boas");
    vee::debug_utils_messenger dbg = vee::create_debug_utils_messenger();
    vee::surface s = vee::create_surface(m_nptr);
    auto [pd, qf] = vee::find_physical_device_with_universal_queue(*s);

    // Device
    vee::device d = vee::create_single_queue_device(pd, qf);
    vee::queue q = vee::get_queue_for_family(qf);

    // Command pool + buffers
    vee::command_pool cp = vee::create_command_pool(qf);
    vee::command_buffer cb = vee::allocate_primary_command_buffer(*cp);

    vee::semaphore img_available_sema = vee::create_semaphore();
    vee::semaphore rnd_finished_sema = vee::create_semaphore();
    vee::fence f = vee::create_fence_signaled();

    while (!interrupted()) {
      try {
        // Swapchain
        vee::swapchain swc = vee::create_swapchain(pd, *s);
        auto swc_imgs = vee::get_swapchain_images(*swc);

        // Depth buffer
        vee::image d_img = vee::create_depth_image(pd, *s);
        vee::device_memory d_mem = vee::create_local_image_memory(pd, *d_img);
        vee::bind_image_memory(*d_img, *d_mem);
        vee::image_view d_iv = vee::create_depth_image_view(*d_img);

        vee::extent ext = vee::get_surface_capabilities(pd, *s).currentExtent;
        vee::render_pass rp = vee::create_render_pass(pd, *s);

        // Frame buffers
        hai::array<vee::image_view> c_ivs{swc_imgs.size()};
        hai::array<vee::framebuffer> fbs{swc_imgs.size()};
        for (auto i = 0; i < fbs.size(); i++) {
          c_ivs[i] = vee::create_rgba_image_view(swc_imgs[i], pd, *s);
          fbs[i] = vee::create_framebuffer({
              .physical_device = pd,
              .surface = *s,
              .render_pass = *rp,
              .image_buffer = *c_ivs[i],
              .depth_buffer = *d_iv,
          });
        }

        while (!interrupted()) {
          // Flip
          vee::wait_and_reset_fence(*f);
          auto idx = vee::acquire_next_image(*swc, *img_available_sema);

          // Build command buffer
          vee::begin_cmd_buf_one_time_submit(cb);
          vee::cmd_begin_render_pass({
              .command_buffer = cb,
              .render_pass = *rp,
              .framebuffer = *fbs[idx],
              .extent = ext,
              .clear_color = {{0.1, 0.2, 0.3, 1.0}},
          });
          vee::cmd_end_render_pass(cb);
          vee::end_cmd_buf(cb);

          // Submit and present
          vee::queue_submit({
              .queue = q,
              .fence = *f,
              .command_buffer = cb,
              .wait_semaphore = *img_available_sema,
              .signal_semaphore = *rnd_finished_sema,
          });
          vee::queue_present({
              .queue = q,
              .swapchain = *swc,
              .wait_semaphore = *rnd_finished_sema,
              .image_index = idx,
          });
        }

        vee::device_wait_idle();
      } catch (vee::out_of_date_error) {
      }
    }
  }
};

extern "C" void casein_handle(const casein::event &e) {
  static thread t{};

  static constexpr auto map = [] {
    casein::event_map res{};
    res[casein::CREATE_WINDOW] = [](const casein::event &e) {
      t.start(*e.as<casein::events::create_window>());
    };
    return res;
  }();

  map.handle(e);
}
