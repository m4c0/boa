export module boav;
import casein;
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

        while (!interrupted()) {
          // Flip
          vee::wait_and_reset_fence(*f);
          auto idx = vee::acquire_next_image(*swc, *img_available_sema);

          // Build command buffer
          vee::begin_cmd_buf_one_time_submit(cb);
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
