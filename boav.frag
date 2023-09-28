#version 450

layout(push_constant) uniform upc {
  float aspect;
  float time;
} pc;

layout(location = 0) in vec2 frag_coord;

layout(location = 0) out vec4 frag_colour;

void main() { frag_colour = vec4(frag_coord, 1, 1); }
