export module boa:game;
import :ecs_objects;

namespace boa {
class xor_ll {
  static constexpr const auto null = ~0U;
  unsigned m_data[ecs::grid_cells]{};
  unsigned m_start{null};
  unsigned m_end{null};

  constexpr void iterate(auto fn) const noexcept {
    unsigned it = m_start;
    unsigned prev = null;
    while (it != null) {
      fn(it);
      auto p = prev;
      prev = it;
      it = p ^ m_data[it];
    }
  }

public:
  [[nodiscard]] constexpr auto size() const noexcept {
    auto res = 0U;
    iterate([&](auto) { res++; });
    return res;
  }
  constexpr void push_front(unsigned p) noexcept {
    if (m_start == null) {
      m_start = m_end = p;
      m_data[p] = null ^ null;
      return;
    }
    m_data[m_start] ^= null ^ p;
    m_data[p] = null ^ m_start;
    m_start = p;
  }
  constexpr void pop_back() noexcept {
    if (m_start == m_end) {
      m_data[m_end] = null ^ null; // 0
      m_start = m_end = null;
      return;
    }
    auto last = m_data[m_end] ^ null;
    m_data[last] ^= m_end ^ null;
    m_end = last;
  }
};
static_assert(xor_ll{}.size() == 0);
static_assert([] {
  xor_ll l{};
  l.push_front(5);
  return l.size() == 1;
}());
static_assert([] {
  xor_ll l{};
  l.push_front(3);
  l.push_front(4);
  return l.size() == 2;
}());
static_assert([] {
  xor_ll l{};
  l.push_front(9);
  l.pop_back();
  return l.size() == 0;
}());
static_assert([] {
  xor_ll l{};
  l.push_front(6);
  l.push_front(7);
  l.push_front(8);
  l.pop_back();
  return l.size() == 2;
}());
static_assert([] {
  xor_ll l{};
  l.push_front(6);
  l.push_front(7);
  l.push_front(8);
  l.pop_back();
  l.pop_back();
  l.pop_back();
  return l.size() == 0;
}());
class game {
  static constexpr const auto ticks = 10;

  enum { O, L, R, U, D } m_dir{};
  xor_ll m_snake{};
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
