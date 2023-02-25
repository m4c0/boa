export module boa:game;
import :ecs_objects;
import :xorll;

namespace boa {
class game {
  static constexpr const auto ticks = 10;

  enum { O, L, R, U, D, E } m_dir{};
  xor_ll m_snake{};
  unsigned m_ticks{};
  unsigned m_target = 3;
  unsigned x{ecs::grid_w / 2};
  unsigned y{ecs::grid_h / 2};

public:
  constexpr game() { m_snake.push_front(y * ecs::grid_w + x); }

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
    m_snake.iterate([&](auto p) { g.set(p); });
    return g;
  }

  [[nodiscard]] bool tick() {
    m_ticks = (m_ticks + 1) % ticks;
    if (m_ticks > 0)
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

    const auto p = y * ecs::grid_w + x;
    if (!m_snake.is_empty(p)) {
      m_dir = E;
      return true;
    }

    m_snake.push_front(y * ecs::grid_w + x);
    if (m_snake.size() > m_target)
      m_snake.pop_back();
    return true;
  }
};
} // namespace boa
