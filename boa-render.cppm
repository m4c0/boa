export module boa:render;
import casein;
import hai;

namespace boa {
struct quad {
  float r;
  float g;
  float b;
  float a; // Currently unused
};
struct filler {
  virtual void operator()(quad *) const noexcept = 0;
};
struct renderer {
  virtual ~renderer() {}
  virtual void setup(casein::native_handle_t) = 0;
  virtual void update(const filler &) = 0;
  virtual void repaint() = 0;
  virtual void quit() = 0;
};
[[nodiscard]] hai::uptr<renderer> create_renderer();
} // namespace boa
