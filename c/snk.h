#pragma once

typedef enum snk_dir {
  snk_d_o,
  snk_d_e,
  snk_d_l = 0x10,
  snk_d_r,
  snk_d_u,
  snk_d_d,
} snk_dir_t;

typedef enum snk_outcome {
  snk_o_none,
  snk_o_move,
  snk_o_eat_food,
  snk_o_death,
  snk_o_game_over,
  snk_o_new_game,
} snk_outcome_t;

extern snk_dir_t snk_dir;
extern int       snk_food;
extern int       snk_head;
extern int       snk_tail;
extern unsigned  snk_size;

extern unsigned snk_x;
extern unsigned snk_y;
extern unsigned snk_grid_w;
extern unsigned snk_grid_h;

extern unsigned snk_timer;

void snk_resize(unsigned w, unsigned h);

int snk_check_food(int p);

void snk_grow(int p);
int  snk_hits(int p);
int  snk_next(int p);

snk_outcome_t snk_reset();
snk_outcome_t snk_run_tick();
snk_outcome_t snk_update_dir(snk_dir_t n);

static inline int snk_is_over() { return snk_dir == snk_d_e; }
static inline int snk_is_new () { return snk_dir == snk_d_o; }

#ifdef SNK_IMPLEMENTATION
#include "tmr.h"

#include <stdlib.h>
#include <time.h>

#define SNK_MAX_CELLS (24 * 24 * 4)

typedef struct snk_node {
  unsigned used;
  int next;
  int prev;
} snk_node_t;

static snk_node_t snk_data[SNK_MAX_CELLS];
static unsigned   snk_grid_size;
static unsigned   snk_target;

snk_dir_t snk_dir;
int       snk_food;
unsigned  snk_grid_w;
unsigned  snk_grid_h;
int       snk_head;
int       snk_tail;
unsigned  snk_timer;
unsigned  snk_size;
unsigned  snk_x;
unsigned  snk_y;

snk_outcome_t snk_reset() {
  tmr_deinit();

  srand(time(0));

  for (int i = 0; i < SNK_MAX_CELLS; i++) snk_data[i] = (snk_node_t) {0};
  snk_size   = 1;
  snk_target = 3;

  snk_food      = -1;
  snk_timer     = 300;

  snk_x = snk_grid_w / 2;
  snk_y = snk_grid_h / 2;

  snk_dir = snk_d_o;

  snk_head = snk_tail = snk_y * snk_grid_w + snk_x;
  snk_data[snk_head] = (snk_node_t) {
    .used = 1,
    .next = -1,
    .prev = -1,
  };

  tmr_init(snk_timer);

  return snk_o_new_game;
}
void snk_resize(unsigned w, unsigned h) {
  float grid_h = 24.0f;
  float grid_w = grid_h;
  if (w > h) {
    grid_w = grid_w * w / h;
  } else {
    grid_h = grid_h * h / w;
  }

  snk_grid_w = grid_w;
  snk_grid_h = grid_h;
  snk_grid_size = snk_grid_w * snk_grid_h;
}

void snk_eat(int p) {
  snk_target += 3;
  snk_grow(p);
}

int snk_hits(int p) { return snk_data[p].used; }
int snk_next(int p) { return snk_data[p].next; }

void snk_grow(int p) {
  snk_data[snk_head].prev = p;
  snk_data[p] = (snk_node_t) {
    .used = 1,
    .next = snk_head,
    .prev = -1,
  };
  snk_head = p;

  snk_size++;
  if (snk_size <= snk_target) return;
  snk_size--;

  snk_node_t * n = snk_data + snk_tail;
  snk_data[n->prev].next = -1;
  snk_tail = n->prev;
  *n = (snk_node_t) {0};
}

static void snk_reset_food() {
  for (int i = 0; i < 100; i++) {
    snk_food = rand() % snk_grid_size;
    if (!snk_hits(snk_food)) return;
  }
  int wtf = snk_food;
  do {
    snk_food = (snk_food + 1) % snk_grid_size;
    if (!snk_hits(snk_food)) return;
  } while (wtf != snk_food);
}
int snk_check_food(int p) {
  if (snk_food < 0) snk_reset_food();
  if (snk_food != p) return 0;
  snk_eat(p);
  snk_reset_food();
  return 1;
}

static snk_outcome_t snk_die() {
  snk_dir = snk_d_e;
  return snk_o_death;
}

snk_outcome_t snk_run_tick() {
  switch (snk_dir) {
    case snk_d_e: return snk_o_game_over;
    case snk_d_o: return snk_o_new_game;
    case snk_d_u: --snk_y; break;
    case snk_d_d: ++snk_y; break;
    case snk_d_l: --snk_x; break;
    case snk_d_r: ++snk_x; break;
  }
  if (snk_x > snk_grid_w - 1) return snk_die();
  if (snk_y > snk_grid_h - 1) return snk_die();

  const int p = snk_y * snk_grid_w + snk_x;
  if (snk_hits(p)) return snk_die();
  if (snk_check_food(p)) {
    tmr_deinit();
    snk_timer -= 10;
    if (snk_timer < 25) snk_timer = 25;
    tmr_init(snk_timer);
    return snk_o_eat_food;
  }

  snk_grow(p);
  return snk_o_move;
}

snk_outcome_t snk_update_dir(snk_dir_t n) {
  if (snk_dir == n)       return snk_o_none;
  if (snk_dir == (n ^ 1)) return snk_o_none;
  if (snk_is_over())      return snk_o_none;

  snk_dir = n;
  return snk_run_tick();
}

#endif
