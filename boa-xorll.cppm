export module boa:xorll;
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
} // namespace boa
