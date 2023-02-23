export module boa:game;
import :ecs_objects;

namespace boa {
class game {
public:
  void up() {}
  void down() {}
  void left() {}
  void right() {}

  [[nodiscard]] ecs::grid grid() {
    ecs::grid g{};
    g.set(2, 2, true);
    return g;
  }
};
} // namespace boa
