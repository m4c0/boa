export module boa:render;
import :ecs_objects;
import casein;
import hai;

namespace boa {
class r_pimpl;
class renderer {
  hai::uptr<r_pimpl> m_data;

public:
  renderer();
  ~renderer();

  void setup(casein::native_handle_t);
  void update(const ecs::grid &);
  void repaint();
  void quit();
};
} // namespace boa
