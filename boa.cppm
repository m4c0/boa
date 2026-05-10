module;
extern "C" {
#include "snk.h"
}

export module boa;
import hai;
import rng;

namespace boa {
enum class dir_et { O, L, R, U, D, E };

export class game {
  static constexpr const auto min_ticks_per_move = 2;
  static constexpr const auto max_ticks_per_move = 16;
  static constexpr const auto food_per_decrement = 4;

  unsigned grid_w;
  unsigned grid_h;

  dir_et   m_dir{};
  unsigned m_ticks{};
  unsigned m_tpm{max_ticks_per_move};
  unsigned m_fpd{food_per_decrement};
  unsigned x{grid_w / 2};
  unsigned y{grid_h / 2};

  [[nodiscard]] snk_outcome_t update_dir(dir_et n, dir_et opp) {
    if (m_dir == n        ) return snk_o_none;
    if (m_dir == opp      ) return snk_o_none;
    if (m_dir == dir_et::E) return snk_o_none;

    m_dir = n;
    m_ticks = ((m_ticks / m_tpm) + 1) * m_tpm;
    return run_tick();
  }

  [[nodiscard]] snk_outcome_t die() {
    m_dir = dir_et::E;
    return snk_o_death;
  }

  [[nodiscard]] snk_outcome_t run_tick() {
    switch (m_dir) {
      using enum dir_et;
      case E: return snk_o_game_over;
      case O: return snk_o_new_game;
      case U: --y; break;
      case D: ++y; break;
      case L: --x; break;
      case R: ++x; break;
    }
    if (x > grid_w - 1) return die();
    if (y > grid_h - 1) return die();

    const auto p = y * grid_w + x;
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

public:
  constexpr game(unsigned w, unsigned h) : grid_w{w}, grid_h{h} {
    snk_reset(w, h);
  }

  [[nodiscard]] constexpr auto grid_width() const noexcept { return grid_w; }
  [[nodiscard]] constexpr auto grid_height() const noexcept { return grid_h; }

  [[nodiscard]] auto up()    { return update_dir(dir_et::U, dir_et::D); }
  [[nodiscard]] auto down()  { return update_dir(dir_et::D, dir_et::U); }
  [[nodiscard]] auto left()  { return update_dir(dir_et::L, dir_et::R); }
  [[nodiscard]] auto right() { return update_dir(dir_et::R, dir_et::L); }

  [[nodiscard]] constexpr auto is_new_game () const noexcept { return m_dir == dir_et::O; }
  [[nodiscard]] constexpr auto is_game_over() const noexcept { return m_dir == dir_et::E; }

  [[nodiscard]] snk_outcome_t tick() {
    m_ticks++;
    if (m_ticks % m_tpm > 0) return snk_o_none;
    return run_tick();
  }
};
} // namespace boa
