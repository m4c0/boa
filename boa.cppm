module;
extern "C" {
#include "snk.h"
}

export module boa;

namespace boa {
export class game {
public:
  game(unsigned w, unsigned h) {
    snk_reset(w, h);
  }

  [[nodiscard]] auto up()    { return snk_update_dir(snk_d_u); }
  [[nodiscard]] auto down()  { return snk_update_dir(snk_d_d); }
  [[nodiscard]] auto left()  { return snk_update_dir(snk_d_l); }
  [[nodiscard]] auto right() { return snk_update_dir(snk_d_r); }

  [[nodiscard]] snk_outcome_t tick() {
    return snk_run_tick();
  }
};
} // namespace boa
