export module boa:ecs_objects;
import :render;
import traits;

namespace boa::ecs {
struct xy {
  float x;
  float y;
};

constexpr const auto grid_w = 32;
constexpr const auto grid_h = 24;
constexpr const auto grid_cells = grid_w * grid_h;
class grid {
  bool m_data[grid_cells]{};

public:
  constexpr void set(unsigned p) { m_data[p] = true; }

  [[nodiscard]] const auto begin() const noexcept { return m_data; }
  [[nodiscard]] const auto end() const noexcept { return &m_data[grid_cells]; }
};

class grid2colour : public filler {
  const grid &m_g;

public:
  explicit constexpr grid2colour(const grid &g) : m_g{g} {}

  void operator()(quad *is) const noexcept override {
    constexpr const quad on{1, 1, 1, 1};
    constexpr const quad off{0, 0.1, 0, 1};

    for (auto b : m_g) {
      *is = b ? on : off;
      is++;
    }
  }
};
} // namespace boa::ecs
