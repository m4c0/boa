export module boav;
import casein;
import sith;
import vee;

class thread : public sith::thread {
public:
  void run() override {
    // Instance
    vee::instance i = vee::create_instance("boas");
    vee::debug_utils_messenger dbg = vee::create_debug_utils_messenger();
    auto [pd, qf] = vee::find_physical_device_with_universal_queue(nullptr);

    // Device
    vee::device d = vee::create_single_queue_device(pd, qf);
    vee::queue q = vee::get_queue_for_family(qf);
  }
};

class agg {
  thread m_thread{};

public:
  void create_window() { m_thread.start(); }
};

extern "C" void casein_handle(const casein::event &e) {
  static agg gg{};

  static constexpr auto map = [] {
    casein::event_map res{};
    res[casein::CREATE_WINDOW] = [](auto) { gg.create_window(); };
    return res;
  }();

  map.handle(e);
}
