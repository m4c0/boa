export module boa:ecs_objects;
import traits;

template <typename T> inline constexpr const auto offset_of(auto T::*M) {
  return (traits::size_t) & (((T *)0)->*M);
}

namespace boa::ecs {
struct xy {
  float x;
  float y;
};
struct rgba {
  float r;
  float g;
  float b;
  float a;
};

struct point {
  xy pos;
};
struct quad {
  xy pos;
  rgba color;
};
} // namespace boa::ecs
