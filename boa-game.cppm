export module boa:game;
import :ecs_objects;

namespace boa {
class game {
  static constexpr const auto ticks = 10;

  enum { O, L, R, U, D } m_dir{};
  unsigned m_ticks{};
  unsigned x{};
  unsigned y{};

public:
  void up() {
    if (m_dir != D)
      m_dir = U;
  }
  void down() {
    if (m_dir != U)
      m_dir = D;
  }
  void left() {
    if (m_dir != R)
      m_dir = L;
  }
  void right() {
    if (m_dir != L)
      m_dir = R;
  }

  [[nodiscard]] ecs::grid grid() {
    ecs::grid g{};
    g.set(x, y, true);
    return g;
  }

  [[nodiscard]] bool tick() {
    m_ticks = (m_ticks + 1) % ticks;
    if (m_ticks > 0)
      return false;

    switch (m_dir) {
    case O:
      break;
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
    return true;
  }
};
} // namespace boa
