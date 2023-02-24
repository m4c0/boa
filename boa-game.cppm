export module boa:game;
import :ecs_objects;

namespace boa {
class xor_ll {
  static constexpr const auto null = ~0U;
  unsigned m_data[ecs::grid_cells]{};
  unsigned m_start{null};
  unsigned m_end{null};
  unsigned m_size{0};

public:
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
  constexpr bool is_empty(unsigned p) const noexcept { return m_data[p] == 0; }

  [[nodiscard]] constexpr auto size() const noexcept { return m_size; }
  constexpr void push_front(unsigned p) noexcept {
    m_size++;
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
    m_size--;
    if (m_start == m_end) {
      m_data[m_end] = null ^ null; // 0
      m_start = m_end = null;
      return;
    }
    auto last = m_data[m_end] ^ null;
    m_data[last] ^= m_end ^ null;
    m_data[m_end] = 0;
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
