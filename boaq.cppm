export module boaq;
import boa;
import casein;
import quack;
import silog;

class agg {
  static constexpr const auto max_cells = 128 * 128;

  boa::game m_g{32, 24};
  quack::renderer m_r{1};
  quack::ilayout m_il{&m_r, max_cells};

public:
  void process_event(const casein::event &e) {
    m_r.process_event(e);
    m_il.process_event(e);
  }

  void create_window() { resize(32, 24); }
  void resize(unsigned w, unsigned h) {
    auto grid_h = 24.0f;
    auto grid_w = grid_h;
    if (w > h) {
      grid_w = grid_w * w / h;
    } else {
      grid_h = grid_h * h / w;
    }

    silog::log(silog::info, "using a %.2fx%.2f board", grid_w, grid_h);

    m_g = {static_cast<unsigned>(grid_w), static_cast<unsigned>(grid_h)};
    m_il->center_at(grid_w / 2.0, grid_h / 2.0);
    m_il->set_grid(grid_w, grid_h);
    m_il->resize(w, h);
    m_il->load_atlas(16, 16, [](auto *) {});
    paint();
  }

  void paint() {
    auto count = 0;
    m_il->map_all([&](auto p) {
      auto &[cs, ms, ps, us] = p;
      const auto paint = [&](auto x, auto y) {
        auto xf = static_cast<float>(x);
        auto yf = static_cast<float>(y);
        *ps++ = {{xf, yf}, {1, 1}};
        *us++ = {};
        *ms++ = {1, 1, 1, 1};
        count++;
      };
      for (auto [x, y] : m_g) {
        paint(x, y);
        *cs++ = {1, 1, 0, 1};
      }
      auto [x, y] = m_g.food();
      paint(x, y);
      *cs++ = {1, 0, 1, 1};
    });
    m_il->set_count(count);
  }
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
    m_g = {m_g.grid_width(), m_g.grid_height()};
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
    res[casein::RESIZE_WINDOW] = [](const casein::event &e) {
      auto [w, h, a, b] = *e.as<casein::events::resize_window>();
      gg.resize(w, h);
    };
    res[casein::REPAINT] = [](auto) { gg.repaint(); };
    res[casein::GESTURE] = [](auto e) { g_map.handle(e); };
    res[casein::KEY_DOWN] = [](auto e) { k_map.handle(e); };
    return res;
  }();

  gg.process_event(e);
  map.handle(e);
}
