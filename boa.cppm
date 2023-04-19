export module boa;
import :agg;
import casein;

extern "C" void casein_handle(const casein::event &e) {
  static boa::agg gg{};
  static constexpr auto g_map = [] {
    casein::subevent_map<casein::events::gesture, casein::G_MAX> res{};
    res[casein::G_SWIPE_UP] = [](auto) { gg.up(); };
    res[casein::G_SWIPE_DOWN] = [](auto) { gg.down(); };
    res[casein::G_SWIPE_LEFT] = [](auto) { gg.left(); };
    res[casein::G_SWIPE_RIGHT] = [](auto) { gg.right(); };
    res[casein::G_SHAKE] = [](auto) { gg.reset(); };
    return res;
  }();
  static constexpr auto k_map = [] {
    casein::subevent_map<casein::events::key_down, casein::K_MAX> res{};
    res[casein::K_UP] = [](auto) { gg.up(); };
    res[casein::K_DOWN] = [](auto) { gg.down(); };
    res[casein::K_LEFT] = [](auto) { gg.left(); };
    res[casein::K_RIGHT] = [](auto) { gg.right(); };
    res[casein::K_SPACE] = [](auto) { gg.reset(); };
    return res;
  }();
  static constexpr auto map = [] {
    casein::event_map res{};
    res[casein::CREATE_WINDOW] = [](auto) { gg.create_window(); };
    res[casein::REPAINT] = [](auto) { gg.repaint(); };
    res[casein::GESTURE] = [](auto e) { g_map.handle(e); };
    res[casein::KEY_DOWN] = [](auto e) { k_map.handle(e); };
    return res;
  }();

  gg.process_event(e);
  map.handle(e);
}
