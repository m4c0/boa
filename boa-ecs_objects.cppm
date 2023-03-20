export module boa:ecs_objects;
import quack;

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

struct gridpos {
  void operator()(quack::pos *is) const noexcept {
    unsigned i = 0;
    for (auto y = 0; y < ecs::grid_h; y++) {
      for (auto x = 0; x < ecs::grid_w; x++, i++) {
        is[i].x = x;
        is[i].y = y;
      }
    }
  }
};
class grid2colour {
  const grid &m_g;

public:
  explicit constexpr grid2colour(const grid &g) : m_g{g} {}

  void operator()(quack::colour *is) const noexcept {
    constexpr const quack::colour on{1, 1, 1, 1};
    constexpr const quack::colour off{0, 0.1, 0, 1};

    for (auto b : m_g) {
      *is = b ? on : off;
      is++;
    }
  }
};
} // namespace boa::ecs
