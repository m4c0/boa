export module boa:ecs_objects;

export namespace boa::ecs {
struct xy {
  float x;
  float y;
};

constexpr const auto grid_w = 32;
constexpr const auto grid_h = 24;
constexpr const auto grid_cells = grid_w * grid_h;
} // namespace boa::ecs
