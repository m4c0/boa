export module boa:casein;
import :game;
import :vulkan_fsm;
import casein;
import hai;

extern "C" void casein_handle(const casein::event &e) {
  static boa::game g{};
  static auto fsm = hai::uptr<boa::vulkan::fsm>::make();

  switch (e.type()) {
  case casein::CREATE_WINDOW:
    fsm->create_window(e.as<casein::events::create_window>());
    fsm->update(g.grid());
    break;
  case casein::REPAINT:
    if (g.tick())
      fsm->update(g.grid());
    fsm->repaint();
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
    fsm->update(g.grid());
    break;
  case casein::QUIT:
    fsm->quit();
    fsm = {};
    break;
  default:
    break;
  }
}
