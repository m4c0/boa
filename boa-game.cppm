export module boa:game;
import :ecs_objects;
import :xorll;

namespace boa {
class game {
  static constexpr const auto ticks = 10;
  static constexpr const auto initial_size = 3;
  static constexpr const auto size_increment = 3;

  enum { O, L, R, U, D, E } m_dir{};
  xor_ll m_snake{};
  unsigned m_ticks{};
  unsigned m_food{~0U};
  unsigned m_target = initial_size;
  unsigned x{ecs::grid_w / 2};
  unsigned y{ecs::grid_h / 2};

  void reset_food() {
    m_food = m_ticks % ecs::grid_cells;
    // TODO: fix randomness (as it is clearly monotonically increasing)
    // TODO: avoid positions owned by snakes
  }

public:
  constexpr game() { m_snake.push_front(y * ecs::grid_w + x); }

  // TODO: find a way to avoid walking reverse
  void up() {
    if (m_dir != D && m_dir != E)
      m_dir = U;
  }
  void down() {
    if (m_dir != U && m_dir != E)
      m_dir = D;
  }
  void left() {
    if (m_dir != R && m_dir != E)
      m_dir = L;
  }
  void right() {
    if (m_dir != L && m_dir != E)
      m_dir = R;
  }

  [[nodiscard]] ecs::grid grid() {
    ecs::grid g{};
    if (m_food != ~0U)
      g.set(m_food);
    m_snake.iterate([&](auto p) { g.set(p); });
    return g;
  }

  [[nodiscard]] bool tick() {
    m_ticks++;
    if (m_ticks % ticks > 0)
      return false;

    switch (m_dir) {
    case E:
      return false;
    case O:
      return false;
    case U:
      y = (y - 1 + ecs::grid_h) % ecs::grid_h;
      break;
    case D:
      y = (y + 1 + ecs::grid_h) % ecs::grid_h;
      break;
    case L:
      x = (x - 1 + ecs::grid_w) % ecs::grid_w;
      break;
    case R:
      x = (x + 1 + ecs::grid_w) % ecs::grid_w;
      break;
    }

    if (m_food == ~0)
      reset_food();

    const auto p = y * ecs::grid_w + x;
    if (!m_snake.is_empty(p)) {
      m_dir = E;
      return true;
    }
    if (m_food == p) {
      m_target += size_increment;
      reset_food();
    }

    m_snake.push_front(y * ecs::grid_w + x);
    if (m_snake.size() > m_target)
      m_snake.pop_back();
    return true;
  }
};
} // namespace boa
