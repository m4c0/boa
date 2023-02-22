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

constexpr const auto grid_w = 10;
constexpr const auto grid_h = 10;
class grid {
  bool m_data[grid_w * grid_h]{};

public:
  constexpr void set(unsigned x, unsigned y, bool on) {
    m_data[y * grid_w + x] = on;
  }

  [[nodiscard]] const auto begin() const noexcept { return m_data; }
  [[nodiscard]] const auto end() const noexcept {
    return &m_data[grid_h * grid_w];
  }
};
} // namespace boa::ecs
