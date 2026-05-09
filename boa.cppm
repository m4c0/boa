module;
#include "snk.h"

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

export class game {
  static constexpr const auto min_ticks_per_move = 2;
  static constexpr const auto max_ticks_per_move = 16;
  static constexpr const auto food_per_decrement = 4;

  unsigned grid_w;
  unsigned grid_h;
  unsigned grid_cells = grid_w * grid_h;

  enum { O, L, R, U, D, E } m_dir{};
  unsigned m_ticks{};
  unsigned m_tpm{max_ticks_per_move};
  unsigned m_fpd{food_per_decrement};
  unsigned m_food{~0U};
  unsigned x{grid_w / 2};
  unsigned y{grid_h / 2};

  void reset_food(unsigned n = 0) {
    m_food = rng::rand(grid_cells);
    if (!snk_hits(m_food)) return;
    if (n < 100) {
      reset_food(n + 1);
      return;
    }
    auto wtf = m_food;
    do {
      m_food = (m_food + 1) % grid_cells;
      if (!snk_hits(m_food)) return;
    } while (wtf != m_food);
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
    if (snk_hits(p)) {
      m_dir = E;
      return outcome::death;
    }
    if (m_food == p) {
      if (m_tpm > min_ticks_per_move && --m_fpd == 0) {
        m_fpd = food_per_decrement;
        m_tpm--;
      }
      snk_eat(p);
      reset_food();
      return outcome::eat_food;
    }

    snk_grow(p);
    return outcome::move;
  }

public:
  constexpr game(unsigned w, unsigned h) : grid_w{w}, grid_h{h} {
    rng::seed();
    snk_reset(y * grid_w + x);
  }

  [[nodiscard]] constexpr auto grid_width() const noexcept { return grid_w; }
  [[nodiscard]] constexpr auto grid_height() const noexcept { return grid_h; }

  [[nodiscard]] auto up() { return update_dir(U, D); }
  [[nodiscard]] auto down() { return update_dir(D, U); }
  [[nodiscard]] auto left() { return update_dir(L, R); }
  [[nodiscard]] auto right() { return update_dir(R, L); }

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
