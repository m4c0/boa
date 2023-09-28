#version 450

layout(push_constant) uniform upc {
  float aspect;
  float time;
} pc;

layout(location = 0) in vec2 frag_coord;

layout(location = 0) out vec4 frag_colour;

float sdf_circle(vec2 p, float r) {
  return length(p) - r;
}

void main() { 
  float d = sdf_circle(frag_coord, 0.3 + sin(pc.time) * 0.1);

  d = 0.01 / abs(d);

  frag_colour = vec4(d, 0, 0, 1); 
}
