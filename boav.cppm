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
    [[maybe_unused]] vee::queue q = vee::get_queue_for_family(qf);
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
