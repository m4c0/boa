export module boa:xorll;
import hai;

namespace boa {
static constexpr const auto null = ~0U;

class xor_ll_iter {
  const unsigned *m_data{};
  unsigned m_pos{null};
  unsigned m_prev{null};

public:
  constexpr xor_ll_iter() = default;
  constexpr xor_ll_iter(const unsigned *d, unsigned p) : m_data{d}, m_pos{p} {}

  constexpr auto operator*() const noexcept { return m_pos; }
  constexpr bool operator==(xor_ll_iter o) const noexcept {
    return m_pos == o.m_pos;
  }
  constexpr auto &operator++() noexcept {
    auto p = m_prev;
    m_prev = m_pos;
    m_pos = p ^ m_data[m_pos];
    return *this;
  }
};
class xor_ll {
  hai::array<unsigned> m_data;
  unsigned m_start{null};
  unsigned m_end{null};
  unsigned m_size{0};

public:
  constexpr xor_ll(unsigned cells) : m_data{cells} {}

  constexpr auto begin() const noexcept {
    return xor_ll_iter{m_data.begin(), m_start};
  }
  constexpr auto end() const noexcept { return xor_ll_iter{}; }

  constexpr void iterate(auto fn) const noexcept {
    for (auto it = begin(); it != end(); ++it) {
      fn(*it);
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
static_assert(xor_ll{32}.size() == 0);
static_assert([] {
  xor_ll l{32};
  l.push_front(5);
  return l.size() == 1;
}());
static_assert([] {
  xor_ll l{32};
  l.push_front(3);
  l.push_front(4);
  return l.size() == 2;
}());
static_assert([] {
  xor_ll l{32};
  l.push_front(9);
  l.pop_back();
  return l.size() == 0;
}());
static_assert([] {
  xor_ll l{32};
  l.push_front(6);
  l.push_front(7);
  l.push_front(8);
  l.pop_back();
  return l.size() == 2;
}());
static_assert([] {
  xor_ll l{32};
  l.push_front(6);
  l.push_front(7);
  l.push_front(8);
  l.pop_back();
  l.pop_back();
  l.pop_back();
  return l.size() == 0;
}());
} // namespace boa
