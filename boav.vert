#version 450

layout(push_constant) uniform upc {
  float aspect;
  float time;
  float dead_at;
  vec2 grid;
} pc;

layout(location = 0) out vec2 frag_coord;
layout(location = 1) out vec2 frag_grid;

void main() {
  ivec2 position = ivec2(gl_VertexIndex % 2, gl_VertexIndex / 2);
  vec2 p = position * 3 - 1;
  gl_Position = vec4(p, 0.0, 1.0);
  frag_coord = p * vec2(pc.aspect, 1.0);

  p = position;
  p.x *= pc.aspect < 1.0 ? pc.grid.x : (pc.grid.y * pc.aspect);
  p.y *= pc.aspect < 1.0 ? (pc.grid.x / pc.aspect) : pc.grid.y;
  
  frag_grid = p;
}
