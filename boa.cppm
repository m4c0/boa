export module boa;
import :game;
import casein;
import quack;

namespace boa {
class agg {
  game m_g{};
  quack::renderer m_r{1};
  quack::instance_layout<bool, boa::ecs::grid_cells> m_il{&m_r};

public:
  void process_event(const casein::event &e) {
    m_r.process_event(e);
    m_il.process_event(e);
  }

  void create_window() {
    m_il.batch()->positions().map(boa::ecs::gridpos{});
    m_il.batch()->colours().map(boa::ecs::grid2colour{m_g.grid()});
    m_il.batch()->resize(boa::ecs::grid_w, boa::ecs::grid_h, boa::ecs::grid_w,
                         boa::ecs::grid_h);
    m_r.load_atlas(16, 16, [](auto *) {});
  }

  void paint() {
    m_il.batch()->colours().map(boa::ecs::grid2colour{m_g.grid()});
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
    m_g = {};
    paint();
  }
};
} // namespace boa

extern "C" void casein_handle(const casein::event &e) {
  static boa::agg gg{};
  gg.process_event(e);

  switch (e.type()) {
  case casein::CREATE_WINDOW:
    gg.create_window();
    break;
  case casein::REPAINT:
    gg.repaint();
    break;
  case casein::GESTURE:
    switch (*e.as<casein::events::gesture>()) {
    case casein::G_SWIPE_UP:
      gg.up();
      break;
    case casein::G_SWIPE_DOWN:
      gg.down();
      break;
    case casein::G_SWIPE_LEFT:
      gg.left();
      break;
    case casein::G_SWIPE_RIGHT:
      gg.right();
      break;
    case casein::G_SHAKE:
      gg.reset();
      break;
    default:
      break;
    }
    break;
  case casein::KEY_DOWN:
    switch (*e.as<casein::events::key_down>()) {
    case casein::K_UP:
      gg.up();
      break;
    case casein::K_DOWN:
      gg.down();
      break;
    case casein::K_LEFT:
      gg.left();
      break;
    case casein::K_RIGHT:
      gg.right();
      break;
    case casein::K_SPACE:
      gg.reset();
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }
}
