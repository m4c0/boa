export module boa;
import :agg;
import casein;

class event_map {
  using callback = void (*)(const casein::event &e);

  callback m_map[casein::MAX_EVENT_TYPE];

public:
  [[nodiscard]] constexpr auto &operator[](casein::event_type idx) noexcept {
    return m_map[idx];
  }

  constexpr void handle(const casein::event &e) const {
    auto fn = m_map[e.type()];
    if (fn)
      fn(e);
  }
};

template <typename ET, ET::type Max> class subevent_map {
  using callback = void (*)(const casein::event &e);

  callback m_map[Max];

public:
  [[nodiscard]] constexpr auto &operator[](ET::type idx) noexcept {
    return m_map[idx];
  }

  constexpr void handle(const casein::event &e) const {
    auto fn = m_map[*e.as<ET>()];
    if (fn)
      fn(e);
  }
};

extern "C" void casein_handle(const casein::event &e) {
  static boa::agg gg{};
  static constexpr auto g_map = [] {
    subevent_map<casein::events::gesture, casein::G_MAX> res{};
    res[casein::G_SWIPE_UP] = [](auto) { gg.up(); };
    res[casein::G_SWIPE_DOWN] = [](auto) { gg.down(); };
    res[casein::G_SWIPE_LEFT] = [](auto) { gg.left(); };
    res[casein::G_SWIPE_RIGHT] = [](auto) { gg.right(); };
    res[casein::G_SHAKE] = [](auto) { gg.reset(); };
    return res;
  }();
  static constexpr auto k_map = [] {
    subevent_map<casein::events::key_down, casein::K_MAX> res{};
    res[casein::K_UP] = [](auto) { gg.up(); };
    res[casein::K_DOWN] = [](auto) { gg.down(); };
    res[casein::K_LEFT] = [](auto) { gg.left(); };
    res[casein::K_RIGHT] = [](auto) { gg.right(); };
    res[casein::K_SPACE] = [](auto) { gg.reset(); };
    return res;
  }();
  static constexpr auto map = [] {
    event_map res{};
    res[casein::CREATE_WINDOW] = [](auto) { gg.create_window(); };
    res[casein::REPAINT] = [](auto) { gg.repaint(); };
    res[casein::GESTURE] = [](auto e) { g_map.handle(e); };
    res[casein::KEY_DOWN] = [](auto e) { k_map.handle(e); };
    return res;
  }();

  gg.process_event(e);
  map.handle(e);
}
