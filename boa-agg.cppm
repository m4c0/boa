export module boa:agg;
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
