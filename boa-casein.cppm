export module boa:casein;
import :vulkan_fsm;
import casein;
import hai;

extern "C" void casein_handle(const casein::event &e) {
  static auto fsm = hai::uptr<boa::vulkan::fsm>::make();

  switch (e.type()) {
  case casein::CREATE_WINDOW:
    fsm->create_window(e.as<casein::events::create_window>());
    break;
  case casein::REPAINT:
    fsm->repaint();
    break;
  case casein::QUIT:
    fsm->quit();
    fsm = {};
    break;
  default:
    break;
  }
}
