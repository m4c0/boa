export module boa;
import :game;
import casein;
import quack;

extern "C" void casein_handle(const casein::event &e) {
  static boa::game g{};
  static quack::renderer r{1};
  static quack::instance_layout<bool, boa::ecs::grid_cells> il{&r};

  r.process_event(e);
  il.process_event(e);

  switch (e.type()) {
  case casein::CREATE_WINDOW:
    il.batch()->positions().map(boa::ecs::gridpos{});
    il.batch()->colours().map(boa::ecs::grid2colour{g.grid()});
    il.batch()->resize(boa::ecs::grid_w, boa::ecs::grid_h, boa::ecs::grid_w,
                       boa::ecs::grid_h);
    r.load_atlas(16, 16, [](auto *) {});
    break;
  case casein::REPAINT:
    if (g.tick())
      il.batch()->colours().map(boa::ecs::grid2colour{g.grid()});
    break;
  case casein::KEY_DOWN:
    switch (*e.as<casein::events::key_down>()) {
    case casein::K_UP:
      g.up();
      break;
    case casein::K_DOWN:
      g.down();
      break;
    case casein::K_LEFT:
      g.left();
      break;
    case casein::K_RIGHT:
      g.right();
      break;
    case casein::K_SPACE:
      g = {};
      break;
    default:
      break;
    }
    il.batch()->colours().map(boa::ecs::grid2colour{g.grid()});
    break;
  default:
    break;
  }
}
