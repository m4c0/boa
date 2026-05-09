module;
#include "snk.h"

export module boa;
import hai;
import rng;

namespace boa {
export enum class outcome { none, move, eat_food, death, game_over, new_game };

export class game {
  static constexpr const auto min_ticks_per_move = 2;
  static constexpr const auto max_ticks_per_move = 16;
  static constexpr const auto food_per_decrement = 4;

  unsigned grid_w;
  unsigned grid_h;

  enum { O, L, R, U, D, E } m_dir{};
  unsigned m_ticks{};
  unsigned m_tpm{max_ticks_per_move};
  unsigned m_fpd{food_per_decrement};
  unsigned x{grid_w / 2};
  unsigned y{grid_h / 2};

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

    const auto p = y * grid_w + x;
    if (snk_hits(p)) {
      m_dir = E;
      return outcome::death;
    }
    if (snk_check_food(p)) {
      if (m_tpm > min_ticks_per_move && --m_fpd == 0) {
        m_fpd = food_per_decrement;
        m_tpm--;
      }
      return outcome::eat_food;
    }

    snk_grow(p);
    return outcome::move;
  }

public:
  constexpr game(unsigned w, unsigned h) : grid_w{w}, grid_h{h} {
    snk_reset(w, h);
  }

  [[nodiscard]] constexpr auto grid_width() const noexcept { return grid_w; }
  [[nodiscard]] constexpr auto grid_height() const noexcept { return grid_h; }

  [[nodiscard]] auto up() { return update_dir(U, D); }
  [[nodiscard]] auto down() { return update_dir(D, U); }
  [[nodiscard]] auto left() { return update_dir(L, R); }
  [[nodiscard]] auto right() { return update_dir(R, L); }

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
