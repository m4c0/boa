export module boa:casein;
import :game;
import :render;
import casein;
import hai;

extern "C" void casein_handle(const casein::event &e) {
  static boa::game g{};
  static boa::renderer r{};
  // static auto fsm = hai::uptr<boa::vulkan::fsm>::make();

  switch (e.type()) {
  case casein::CREATE_WINDOW:
    r.setup(e.as<casein::events::create_window>().native_window_handle());
    r.update(g.grid());
    break;
  case casein::REPAINT:
    if (g.tick())
      r.update(g.grid());
    r.repaint();
    break;
  case casein::KEY_DOWN:
    switch (e.as<casein::events::key_down>().key_code()) {
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
    r.update(g.grid());
    break;
  case casein::QUIT:
    r.quit();
    break;
  default:
    break;
  }
}
