export module boa:wasm;
import :ecs_objects;
import :render;
import casein;
import hai;

extern "C" void boa_fill_colour(float r, float g, float b);
extern "C" void boa_fill_rect(unsigned x, unsigned y, unsigned w, unsigned h);

namespace boa {
class w_renderer : public renderer {
  ecs::rgba m_map[ecs::grid_cells];

public:
  void setup(casein::native_handle_t) {}
  void update(const ecs::grid2colour &g) { g(m_map); }
  void repaint() {
    for (auto i = 0; i < ecs::grid_cells; i++) {
      const auto &b = m_map[i];
      const auto w = 800 / ecs::grid_w;
      const auto h = 600 / ecs::grid_h;
      const auto x = w * (i % ecs::grid_w);
      const auto y = h * (i / ecs::grid_w);

      boa_fill_colour(b.r, b.g, b.b);
      boa_fill_rect(x, y, w, h);
    }
  }
  void quit() {}
};
hai::uptr<renderer> create_renderer() {
  return hai::uptr<renderer>{new w_renderer{}};
}
} // namespace boa
