#pragma once
#include <stdlib.h>

#define SNK_MAX_CELLS (24 * 24 * 4)

typedef struct snk_node {
  unsigned used;
  int next;
  int prev;
} snk_node_t;

snk_node snk_data[SNK_MAX_CELLS];
int snk_head;
int snk_tail;
unsigned snk_size;
unsigned snk_target;

static void snk_reset() {
  for (int i = 0; i < SNK_MAX_CELLS; i++) snk_data[i] = {};
  snk_size = 0;
  snk_target = 3;
  snk_head = snk_tail = -1;
}

static void snk_grow(int p) {
  if (snk_head == -1) {
    snk_head = snk_tail = p;
    snk_data[p] = {
      .used = 1,
      .next = -1,
      .prev = -1,
    };
  } else {
    snk_data[snk_head].prev = p;
    snk_data[p] = {
      .used = 1,
      .next = snk_head,
      .prev = -1,
    };
    snk_head = p;
  }

  snk_size++;
  if (snk_size <= snk_target) return;
  snk_size--;

  snk_node * n = snk_data + snk_tail;
  snk_data[n->prev].next = -1;
  snk_tail = n->prev;
  *n = {};
}
