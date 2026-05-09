#pragma once

extern int      snk_head;
extern int      snk_tail;
extern unsigned snk_size;

void snk_reset(unsigned gw, unsigned gh);

void snk_eat (int p);
void snk_grow(int p);
int  snk_hits(int p);
int  snk_next(int p);

#ifdef SNK_IMPLEMENTATION
#include <stdlib.h>

#define SNK_MAX_CELLS (24 * 24 * 4)

typedef struct snk_node {
  unsigned used;
  int next;
  int prev;
} snk_node_t;

static snk_node snk_data[SNK_MAX_CELLS];
static unsigned snk_grid_size;
static unsigned snk_target;

int      snk_head;
int      snk_tail;
unsigned snk_size;

void snk_reset(unsigned gw, unsigned gh) {
  srand(time(0));

  for (int i = 0; i < SNK_MAX_CELLS; i++) snk_data[i] = {};
  snk_size   = 1;
  snk_target = 3;

  snk_grid_size = gw * gh;

  snk_head = snk_tail = (gh/2) * gw + (gw/2);
  snk_data[snk_head] = {
    .used = 1,
    .next = -1,
    .prev = -1,
  };
}

void snk_eat(int p) {
  snk_target += 3;
  snk_grow(p);
}

int snk_hits(int p) { return snk_data[p].used; }
int snk_next(int p) { return snk_data[p].next; }

void snk_grow(int p) {
  snk_data[snk_head].prev = p;
  snk_data[p] = {
    .used = 1,
    .next = snk_head,
    .prev = -1,
  };
  snk_head = p;

  snk_size++;
  if (snk_size <= snk_target) return;
  snk_size--;

  snk_node * n = snk_data + snk_tail;
  snk_data[n->prev].next = -1;
  snk_tail = n->prev;
  *n = {};
}
#endif
