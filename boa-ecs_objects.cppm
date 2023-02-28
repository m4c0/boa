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

class grid2colour {
  const grid &m_g;

public:
  explicit constexpr grid2colour(const grid &g) : m_g{g} {}

  void operator()(rgba *is) const {
    constexpr const ecs::rgba on{1, 1, 1, 1};
    constexpr const ecs::rgba off{0, 0.1, 0, 1};

    for (auto b : m_g) {
      *is = b ? on : off;
      is++;
    }
  }
};
} // namespace boa::ecs
