export module boa;
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

export enum class outcome { none, move, eat_food, death, game_over, new_game };

static constexpr const auto null = ~0U;

class snake_iter {
  const unsigned *m_data{};
  unsigned m_pos{null};
  unsigned m_prev{null};
  unsigned m_w;

public:
  explicit constexpr snake_iter(unsigned w) : m_w{w} {}
  explicit constexpr snake_iter(const unsigned * d, unsigned p, unsigned w) : m_data{d}, m_pos{p}, m_w{w} {}

  constexpr auto operator*() const noexcept { return p2point(m_pos, m_w); }
  constexpr bool operator==(snake_iter o) noexcept { return m_pos == o.m_pos; }
  constexpr auto &operator++() noexcept {
    auto p = m_prev;
    m_prev = m_pos;
    m_pos = p ^ m_data[m_pos];
    return *this;
  }
};
export class game {
  static constexpr const auto min_ticks_per_move = 2;
  static constexpr const auto max_ticks_per_move = 16;
  static constexpr const auto food_per_decrement = 4;
  static constexpr const auto initial_size = 3;
  static constexpr const auto size_increment = 3;

  unsigned grid_w;
  unsigned grid_h;
  unsigned grid_cells = grid_w * grid_h;

  hai::array<unsigned> m_snake { grid_cells };
  unsigned m_snake_start = null;
  unsigned m_snake_end   = null;
  unsigned m_snake_size  = 0;

  enum { O, L, R, U, D, E } m_dir{};
  unsigned m_ticks{};
  unsigned m_tpm{max_ticks_per_move};
  unsigned m_fpd{food_per_decrement};
  unsigned m_food{~0U};
  unsigned m_target = initial_size;
  unsigned x{grid_w / 2};
  unsigned y{grid_h / 2};

  void reset_food(unsigned n = 0) {
    m_food = rng::rand(grid_cells);
    if (m_snake[m_food] == 0) return;
    if (n < 100) {
      reset_food(n + 1);
      return;
    }
    auto wtf = m_food;
    do {
      m_food = (m_food + 1) % grid_cells;
      if (m_snake[m_food] == 0) return;
    } while (wtf != m_food);
  }

  void grow() {
    unsigned p = y * grid_w + x;

    m_snake_size++;
    if (m_snake_start == null) {
      m_snake_start = m_snake_end = p;
      m_snake[p] = null ^ null;
    } else {
      m_snake[m_snake_start] ^= null ^ p;
      m_snake[p] = null ^ m_snake_start;
      m_snake_start = p;
    }

    if (m_snake_size <= m_target) return;

    m_snake_size--;
    if (m_snake_start == m_snake_end) {
      m_snake[m_snake_end] = null ^ null; // 0
      m_snake_start = m_snake_end = null;
      return;
    }
    auto last = m_snake[m_snake_end] ^ null;
    m_snake[last] ^= m_snake_end ^ null;
    m_snake[m_snake_end] = 0;
    m_snake_end = last;
  }

  [[nodiscard]] outcome update_dir(decltype(m_dir) n, decltype(m_dir) opp) {
    if (m_dir == n)
      return outcome::none;
    if (m_dir == opp)
      return outcome::none;
    if (m_dir == E)
      return outcome::none;

    m_dir = n;
    m_ticks = ((m_ticks / m_tpm) + 1) * m_tpm;
    return run_tick();
  }

  [[nodiscard]] outcome run_tick() {
    switch (m_dir) {
    case E:
      return outcome::game_over;
    case O:
      return outcome::new_game;
    case U:
      if (y == 0) {
        m_dir = E;
        return outcome::death;
      } else {
        --y;
      }
      break;
    case D:
      if (y == grid_h - 1) {
        m_dir = E;
        return outcome::death;
      } else {
        ++y;
      }
      break;
    case L:
      if (x == 0) {
        m_dir = E;
        return outcome::death;
      } else {
        --x;
      }
      break;
    case R:
      if (x == grid_w - 1) {
        m_dir = E;
        return outcome::death;
      } else {
        ++x;
      }
      break;
    }

    if (m_food == ~0)
      reset_food();

    const auto p = y * grid_w + x;
    if (m_snake[p]) {
      m_dir = E;
      return outcome::death;
    }
    if (m_food == p) {
      if (m_tpm > min_ticks_per_move && --m_fpd == 0) {
        m_fpd = food_per_decrement;
        m_tpm--;
      }
      m_target += size_increment;
      reset_food();
      grow();
      return outcome::eat_food;
    }

    grow();
    return outcome::move;
  }

public:
  constexpr game(unsigned w, unsigned h) : grid_w{w}, grid_h{h} {
    rng::seed();
    grow();
  }

  [[nodiscard]] constexpr auto grid_width() const noexcept { return grid_w; }
  [[nodiscard]] constexpr auto grid_height() const noexcept { return grid_h; }

  [[nodiscard]] auto up() { return update_dir(U, D); }
  [[nodiscard]] auto down() { return update_dir(D, U); }
  [[nodiscard]] auto left() { return update_dir(L, R); }
  [[nodiscard]] auto right() { return update_dir(R, L); }

  [[nodiscard]] constexpr auto begin() const noexcept {
    return snake_iter{m_snake.data(), m_snake_start, grid_w};
  }
  [[nodiscard]] constexpr auto end() const noexcept {
    return snake_iter{grid_w};
  }
  [[nodiscard]] constexpr auto size() const noexcept { return m_snake_size; }
  [[nodiscard]] constexpr auto food() const noexcept {
    return p2point(m_food, grid_w);
  }

  [[nodiscard]] constexpr auto is_new_game() const noexcept {
    return m_dir == O;
  }
  [[nodiscard]] constexpr auto is_game_over() const noexcept {
    return m_dir == E;
  }

  [[nodiscard]] outcome tick() {
    m_ticks++;
    if (m_ticks % m_tpm > 0)
      return outcome::none;

    return run_tick();
  }
};
} // namespace boa
