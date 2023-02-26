export module boa:wasm;
import :ecs_objects;
import :render;
import casein;
import hai;

extern "C" void boa_fill_colour(unsigned r, unsigned g, unsigned b);
extern "C" void boa_fill_rect(unsigned x, unsigned y, unsigned w, unsigned h);

namespace boa {
class r_pimpl {
  ecs::rgba m_map[ecs::grid_cells];

public:
  void map_colours(auto fn) { fn(m_map); }
};
renderer::renderer() : m_data{hai::uptr<r_pimpl>::make()} {}
renderer::~renderer() {}
void renderer::setup(casein::native_handle_t) {}
void renderer::update(const ecs::grid &g) {
  m_data->map_colours([&](auto is) {
    constexpr const ecs::rgba on{1, 1, 1, 1};
    constexpr const ecs::rgba off{0, 0.3, 0, 1};

    for (auto b : g) {
      *is = b ? on : off;
      is++;
    }
  });
}
void renderer::repaint() {
  m_data->map_colours([&](auto is) {
    for (auto i = 0; i < ecs::grid_cells; i++) {
      const auto &b = is[i];
      const auto w = 800 / ecs::grid_w;
      const auto h = 600 / ecs::grid_h;
      const auto x = w * (i % ecs::grid_w);
      const auto y = h * (i / ecs::grid_w);

      boa_fill_colour(b.r * 256.0, b.g * 256.0, b.b * 256.0);
      boa_fill_rect(x, y, w, h);
    }
  });
}
void renderer::quit() {}
} // namespace boa
