export module boa:render;
import :ecs_objects;
import casein;
import hai;

namespace boa {
struct renderer {
  virtual ~renderer() {}
  virtual void setup(casein::native_handle_t) = 0;
  virtual void update(const ecs::grid2colour &) = 0;
  virtual void repaint() = 0;
  virtual void quit() = 0;
};
[[nodiscard]] hai::uptr<renderer> create_renderer();
} // namespace boa
