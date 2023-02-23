export module boa:game;
import :ecs_objects;

namespace boa {
class game {
  unsigned x{};
  unsigned y{};

public:
  void up() { y = (y - 1 + ecs::grid_h) % ecs::grid_h; }
  void down() { y = (y + 1 + ecs::grid_h) % ecs::grid_h; }
  void left() { x = (x - 1 + ecs::grid_w) % ecs::grid_w; }
  void right() { x = (x + 1 + ecs::grid_w) % ecs::grid_w; }

  [[nodiscard]] ecs::grid grid() {
    ecs::grid g{};
    g.set(x, y, true);
    return g;
  }
};
} // namespace boa
