#version 450

layout(push_constant) uniform upc {
  float aspect;
  float time;
  vec2 grid;
} pc;

layout(location = 0) in vec2 position;

layout(location = 0) out vec2 frag_coord;
layout(location = 1) out vec2 frag_grid;

void main() {
  vec2 p = position * 2.0 - 1.0;
  gl_Position = vec4(p, 0.0, 1.0);
  frag_coord = p * vec2(pc.aspect, 1.0);
  frag_grid = position * pc.grid;
}
