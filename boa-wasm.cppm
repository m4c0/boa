export module boa:wasm;
import :ecs_objects;
import :render;
import casein;
import hai;

extern "C" void boa_paint_grid(boa::ecs::rgba *data);

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
    constexpr const ecs::rgba off{0, 0.1, 0, 1};

    for (auto b : g) {
      *is = b ? on : off;
      is++;
    }
  });
}
void renderer::repaint() { m_data->map_colours(boa_paint_grid); }
void renderer::quit() {}
} // namespace boa
