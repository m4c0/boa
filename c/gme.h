#ifndef GME_H
#define GME_H

#include "snk.h"

typedef struct gme_storage {
  float first_seen;
  float seen;
} gme_storage_t;

typedef struct gme_ivec2 {
  unsigned x, y;
} gme_ivec2_t;
typedef struct gme_upc {
  float       aspect;
  float       time;
  float       dead_at;
  float       pad;
  float       grid_width;
  float       grid_height;
  gme_ivec2_t food;
  gme_ivec2_t party;
  float       party_start;
} gme_upc_t;

extern gme_upc_t gme_pc;

snk_outcome_t gme_resize(unsigned w, unsigned h);
snk_outcome_t gme_new_game();

void gme_update(gme_storage_t * buf, snk_outcome_t outcome);

#ifdef GME_IMPLEMENTATION
#include "sfx.h"

gme_upc_t gme_pc;

static snk_outcome_t gme_reset() {
  gme_pc = (gme_upc_t) {
    .pad         = 0,
    .grid_width  = snk_grid_w,
    .grid_height = snk_grid_h,
    .food        = { 10000, 10000 },
    .party       = { 10000, 10000 },
    .party_start = -1,
  };
  return snk_reset();
}

snk_outcome_t gme_resize(unsigned w, unsigned h) {
  snk_resize(w, h);
  return gme_reset();
}

snk_outcome_t gme_new_game() {
  return snk_is_over() ? gme_reset() : snk_o_none;
}

void gme_update(gme_storage_t * buf, snk_outcome_t outcome) {
  if (outcome == snk_o_none) return;

  int cells = snk_grid_w * snk_grid_h;
  for (int i = 0; i < cells; i++) buf[i].seen = 0;

  int s = snk_size;
  int p = snk_head;
  while (p != -1) {
    if (buf[p].first_seen == 0) buf[p].first_seen = gme_pc.time;
    buf[p].seen = s-- / (float)snk_size;
    p = snk_next(p);
  }

  for (int i = 0; i < cells; i++) {
    if (buf[i].seen == 0) buf[i].first_seen = 0;
  }

  unsigned x = snk_food % snk_grid_w;
  unsigned y = snk_food / snk_grid_w;
  if (gme_pc.food.x != x || gme_pc.food.y != y) {
    if (outcome == snk_o_eat_food) {
      sfx_eat();
      gme_pc.party_start = gme_pc.time;
      gme_pc.party = gme_pc.food;
    }
    gme_pc.food.x = x;
    gme_pc.food.y = y;
  }
  if (snk_is_new()) {
    sfx_reset();
    gme_pc.party.x = gme_pc.party.y = 10000;
    gme_pc.party_start = 0;
    gme_pc.dead_at = 0.0;
  } else if (gme_pc.dead_at == 0.0)
    gme_pc.dead_at = snk_is_over() ? gme_pc.time : 0;

  if (outcome == snk_o_move ) sfx_walk();
  if (outcome == snk_o_death) sfx_death();
}

#endif
#endif
