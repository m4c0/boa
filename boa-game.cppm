export module boa:game;
import :xorll;
import hai;

namespace boa {
export class game {
  static constexpr const auto min_ticks_per_move = 2;
  static constexpr const auto max_ticks_per_move = 8;
  static constexpr const auto food_per_decrement = 4;
  static constexpr const auto initial_size = 3;
  static constexpr const auto size_increment = 3;
  static constexpr const auto random_prime = 5393;

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
    m_food = (m_ticks * random_prime) % grid_cells;
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
      y = (y - 1 + grid_h) % grid_h;
      break;
    case D:
      y = (y + 1 + grid_h) % grid_h;
      break;
    case L:
      x = (x - 1 + grid_w) % grid_w;
      break;
    case R:
      x = (x + 1 + grid_w) % grid_w;
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
    m_snake.push_front(y * grid_w + x);
  }

  [[nodiscard]] constexpr auto grid_width() const noexcept { return grid_w; }
  [[nodiscard]] constexpr auto grid_height() const noexcept { return grid_h; }

  void up() { update_dir(U, D); }
  void down() { update_dir(D, U); }
  void left() { update_dir(L, R); }
  void right() { update_dir(R, L); }

  [[nodiscard]] auto grid() {
    hai::array<bool> g{grid_cells};
    if (m_food != ~0U)
      g[m_food] = true;
    m_snake.iterate([&](auto p) { g[p] = true; });
    return g;
  }

  [[nodiscard]] bool tick() {
    m_ticks++;
    if (m_ticks % m_tpm > 0)
      return false;

    return run_tick();
  }
};
} // namespace boa
