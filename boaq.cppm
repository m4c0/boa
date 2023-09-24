#pragma leco app
export module boaq;
import boa;
import casein;
import quack;

using namespace boa;

class grid2colour {
  const ecs::grid &m_g;

public:
  explicit constexpr grid2colour(const ecs::grid &g) : m_g{g} {}

  void operator()(quack::colour *is) const noexcept {
    constexpr const quack::colour on{1, 1, 1, 1};
    constexpr const quack::colour off{0, 0.1, 0, 1};

    for (auto b : m_g) {
      *is = b ? on : off;
      is++;
    }
  }
};

class agg {
  game m_g{};
  quack::renderer m_r{1};
  quack::ilayout m_il{&m_r, ecs::grid_cells};

public:
  void process_event(const casein::event &e) {
    m_r.process_event(e);
    m_il.process_event(e);
  }

  void create_window() {
    m_il->center_at(ecs::grid_w / 2.0, ecs::grid_h / 2.0);
    m_il->set_grid(ecs::grid_w, ecs::grid_h);
    m_il->set_count(ecs::grid_cells);
    m_il->load_atlas(16, 16, [](auto *) {});
    m_il->map_all([](auto p) {
      auto &[cs, ms, ps, us] = p;
      for (float y = 0; y < ecs::grid_h; y++) {
        for (float x = 0; x < ecs::grid_w; x++) {
          *ps++ = {{x, y}, {1, 1}};
          *us++ = {};
          *ms++ = {1, 1, 1, 1};
        }
      }
    });
    paint();
  }

  void paint() { m_il->map_colours(grid2colour{m_g.grid()}); }
  void repaint() {
    if (m_g.tick())
      paint();
  }

  void up() {
    m_g.up();
    paint();
  }
  void down() {
    m_g.down();
    paint();
  }
  void left() {
    m_g.left();
    paint();
  }
  void right() {
    m_g.right();
    paint();
  }

  void reset() {
    m_g = {};
    paint();
  }
};

extern "C" void casein_handle(const casein::event &e) {
  static agg gg{};
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
