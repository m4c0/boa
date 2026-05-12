module;
extern "C" {
#include "snk.h"
#include "tmr.h"
}

export module boa;
import hai;
import rng;

namespace boa {
export class game {
  unsigned m_timer = 300;

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
      tmr_deinit();
      m_timer -= 10;
      if (m_timer < 25) m_timer = 25;
      tmr_init(m_timer);
      return snk_o_eat_food;
    }

    snk_grow(p);
    return snk_o_move;
  }

  [[nodiscard]] snk_outcome_t update_dir(snk_dir_t n, snk_dir_t opp) {
    if (snk_dir == n  ) return snk_o_none;
    if (snk_dir == opp) return snk_o_none;
    if (snk_is_over() ) return snk_o_none;

    snk_dir = n;
    return run_tick();
  }

public:
  game(unsigned w, unsigned h) {
    snk_reset(w, h);
    tmr_init(m_timer);
  }
  ~game() {
    tmr_deinit();
  }


  [[nodiscard]] auto up()    { return update_dir(snk_d_u, snk_d_d); }
  [[nodiscard]] auto down()  { return update_dir(snk_d_d, snk_d_u); }
  [[nodiscard]] auto left()  { return update_dir(snk_d_l, snk_d_r); }
  [[nodiscard]] auto right() { return update_dir(snk_d_r, snk_d_l); }

  [[nodiscard]] snk_outcome_t tick() {
    return run_tick();
  }
};
} // namespace boa
