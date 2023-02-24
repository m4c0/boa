export module boa:ecs_objects;
import traits;

namespace boa::ecs {
struct xy {
  float x;
  float y;
};
struct rgba {
  float r;
  float g;
  float b;
  float a;
};

constexpr const auto grid_w = 60;
constexpr const auto grid_h = 40;
constexpr const auto grid_cells = grid_w * grid_h;
class grid {
  bool m_data[grid_cells]{};

public:
  constexpr void set(unsigned p) { m_data[p] = true; }

  [[nodiscard]] const auto begin() const noexcept { return m_data; }
  [[nodiscard]] const auto end() const noexcept { return &m_data[grid_cells]; }
};
} // namespace boa::ecs
