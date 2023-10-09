export module boa;
import :xorll;
import hai;
import rng;

namespace boa {
export struct point {
  unsigned x;
  unsigned y;
  unsigned idx;
};
constexpr auto p2point(unsigned p, unsigned w) {
  return point{p % w, p / w, p};
}

class snake_iter {
  xor_ll_iter m_it;
  unsigned m_w;

public:
  explicit constexpr snake_iter(xor_ll_iter i, unsigned w) : m_it{i}, m_w{w} {}

  constexpr auto operator*() const noexcept { return p2point(*m_it, m_w); }
  constexpr bool operator==(snake_iter o) noexcept { return m_it == o.m_it; }
  constexpr auto &operator++() noexcept {
    ++m_it;
    return *this;
  }
};
export class game {
  static constexpr const auto min_ticks_per_move = 2;
  static constexpr const auto max_ticks_per_move = 8;
  static constexpr const auto food_per_decrement = 4;
  static constexpr const auto initial_size = 3;
  static constexpr const auto size_increment = 3;

  unsigned grid_w;
  unsigned grid_h;
  unsigned grid_cells{grid_w * grid_h};
  enum { O, L, R, U, D, E } m_dir{};
  xor_ll m_snake{grid_cells};
  unsigned m_ticks{};
  unsigned m_tpm{max_ticks_per_move};
  unsigned m_fpd{food_per_decrement};
  unsigned m_food{~0U};
  unsigned m_target = initial_size;
  unsigned x{grid_w / 2};
  unsigned y{grid_h / 2};

  void reset_food(unsigned n = 0) {
    m_food = rng::rand(grid_cells);
    if (m_snake.is_empty(m_food))
      return;
    if (n < 100) {
      reset_food(n + 1);
      return;
    }
    auto wtf = m_food;
    do {
      m_food = (m_food + 1) % grid_cells;
      if (m_snake.is_empty(m_food))
        return;
    } while (wtf != m_food);
  }

  void update_dir(decltype(m_dir) n, decltype(m_dir) opp) {
    if (m_dir == n)
      return;
    if (m_dir == opp)
      return;
    if (m_dir == E)
      return;

    m_dir = n;
    m_ticks = ((m_ticks / m_tpm) + 1) * m_tpm;
    [[maybe_unused]] auto _ = run_tick();
  }

  [[nodiscard]] bool run_tick() {
    switch (m_dir) {
    case E:
      return false;
    case O:
      return false;
    case U:
      if (y == 0) {
        m_dir = E;
        return true;
      } else {
        --y;
      }
      break;
    case D:
      if (y == grid_h - 1) {
        m_dir = E;
        return true;
      } else {
        ++y;
      }
      break;
    case L:
      if (x == 0) {
        m_dir = E;
        return true;
      } else {
        --x;
      }
      break;
    case R:
      if (x == grid_w - 1) {
        m_dir = E;
        return true;
      } else {
        ++x;
      }
      break;
    }

    if (m_food == ~0)
      reset_food();

    const auto p = y * grid_w + x;
    if (!m_snake.is_empty(p)) {
      m_dir = E;
      return true;
    }
    if (m_food == p) {
      if (m_tpm > min_ticks_per_move && --m_fpd == 0) {
        m_fpd = food_per_decrement;
        m_tpm--;
      }
      m_target += size_increment;
      reset_food();
    }

    m_snake.push_front(y * grid_w + x);
    if (m_snake.size() > m_target)
      m_snake.pop_back();
    return true;
  }

public:
  constexpr game(unsigned w, unsigned h) : grid_w{w}, grid_h{h} {
    rng::seed();
    m_snake.push_front(y * grid_w + x);
  }

  [[nodiscard]] constexpr auto grid_width() const noexcept { return grid_w; }
  [[nodiscard]] constexpr auto grid_height() const noexcept { return grid_h; }

  void up() { update_dir(U, D); }
  void down() { update_dir(D, U); }
  void left() { update_dir(L, R); }
  void right() { update_dir(R, L); }

  [[nodiscard]] constexpr auto begin() const noexcept {
    return snake_iter{m_snake.begin(), grid_w};
  }
  [[nodiscard]] constexpr auto end() const noexcept {
    return snake_iter{m_snake.end(), grid_w};
  }
  [[nodiscard]] constexpr auto size() const noexcept { return m_snake.size(); }
  [[nodiscard]] constexpr auto food() const noexcept {
    return p2point(m_food, grid_w);
  }

  [[nodiscard]] constexpr auto is_new_game() const noexcept {
    return m_dir == O;
  }
  [[nodiscard]] constexpr auto is_game_over() const noexcept {
    return m_dir == E;
  }

  [[nodiscard]] bool tick() {
    m_ticks++;
    if (m_ticks % m_tpm > 0)
      return false;

    return run_tick();
  }
};
} // namespace boa
