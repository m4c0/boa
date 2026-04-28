// Covers a 4:1 screen, as if such thing will ever exist
static const unsigned max_cells = 24 * (24 * 4);

typedef struct ivec2 {
  int x, y;
} ivec2_t;

typedef struct upc {
  float aspect;
  float time;
  float dead_at;
  float pad;
  float grid_width;
  float grid_height;
  ivec2_t food;
  ivec2_t party;
  float party_start;
} upc_t;
static upc_t make_upc() {
  return (upc_t) {
    .grid_width = 24,
    .grid_height = 24,
    .food = { 10000, 10000 },
    .party = { 10000, 10000 },
    .party_start = -1,
  };
}
