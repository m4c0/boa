export module boa:casein;
import :game;
import casein;
import quack;

extern "C" void casein_handle(const casein::event &e) {
  static boa::game g{};
  static quack::renderer r{quack::params{
      .grid_w = boa::ecs::grid_w,
      .grid_h = boa::ecs::grid_h,
      .max_quads = boa::ecs::grid_cells,
  }};

  switch (e.type()) {
  case casein::CREATE_WINDOW:
    r.setup(e.as<casein::events::create_window>().native_window_handle());
    r.fill_pos(boa::ecs::gridpos{});
    r.fill_colour(boa::ecs::grid2colour{g.grid()});
    r.load_atlas(16, 16, [](auto *) {});
    break;
  case casein::REPAINT:
    if (g.tick())
      r.fill_colour(boa::ecs::grid2colour{g.grid()});
    r.repaint(boa::ecs::grid_cells);
    break;
  case casein::KEY_DOWN:
    switch (e.as<casein::events::key_down>().key()) {
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
    r.fill_colour(boa::ecs::grid2colour{g.grid()});
    break;
  case casein::QUIT:
    r.quit();
    break;
  default:
    break;
  }
}
