#version 450

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 i_pos;
layout(location = 2) in vec4 i_color;

layout(location = 0) out vec4 q_color;

void main() {
  vec2 f_pos = (pos + i_pos - 5.0) / 5.0;
  q_color = i_color;
  gl_Position = vec4(f_pos, 0, 1);
}
