module;
extern "C" {
#include "snk.h"
}

export module boa;
import hai;
import rng;

namespace boa {
export class game {
  static constexpr const auto min_ticks_per_move = 2;
  static constexpr const auto max_ticks_per_move = 16;
  static constexpr const auto food_per_decrement = 4;

  unsigned m_ticks{};
  unsigned m_tpm{max_ticks_per_move};
  unsigned m_fpd{food_per_decrement};

  [[nodiscard]] snk_outcome_t die() {
    snk_dir = snk_d_e;
    return snk_o_death;
  }

  [[nodiscard]] snk_outcome_t run_tick() {
    switch (snk_dir) {
      case snk_d_e: return snk_o_game_over;
      case snk_d_o: return snk_o_new_game;
      case snk_d_u: --snk_y; break;
      case snk_d_d: ++snk_y; break;
      case snk_d_l: --snk_x; break;
      case snk_d_r: ++snk_x; break;
    }
    if (snk_x > snk_grid_w - 1) return die();
    if (snk_y > snk_grid_h - 1) return die();

    const auto p = snk_y * snk_grid_w + snk_x;
    if (snk_hits(p)) return die();
    if (snk_check_food(p)) {
      if (m_tpm > min_ticks_per_move && --m_fpd == 0) {
        m_fpd = food_per_decrement;
        m_tpm--;
      }
      return snk_o_eat_food;
    }

    snk_grow(p);
    return snk_o_move;
  }

  [[nodiscard]] snk_outcome_t update_dir(snk_dir_t n, snk_dir_t opp) {
    if (snk_dir == n      ) return snk_o_none;
    if (snk_dir == opp    ) return snk_o_none;
    if (snk_dir == snk_d_e) return snk_o_none;

    snk_dir = n;
    m_ticks = ((m_ticks / m_tpm) + 1) * m_tpm;
    return run_tick();
  }

public:
  constexpr game(unsigned w, unsigned h) {
    snk_reset(w, h);
  }

  [[nodiscard]] auto up()    { return update_dir(snk_d_u, snk_d_d); }
  [[nodiscard]] auto down()  { return update_dir(snk_d_d, snk_d_u); }
  [[nodiscard]] auto left()  { return update_dir(snk_d_l, snk_d_r); }
  [[nodiscard]] auto right() { return update_dir(snk_d_r, snk_d_l); }

  [[nodiscard]] constexpr auto is_new_game () const noexcept { return snk_dir == snk_d_o; }
  [[nodiscard]] constexpr auto is_game_over() const noexcept { return snk_dir == snk_d_e; }

  [[nodiscard]] snk_outcome_t tick() {
    m_ticks++;
    if (m_ticks % m_tpm > 0) return snk_o_none;
    return run_tick();
  }
};
} // namespace boa
