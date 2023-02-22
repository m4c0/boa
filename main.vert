#version 450

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 i_pos;

void main() {
  vec2 f_pos = (pos + i_pos - 5.0) / 5.0;
  gl_Position = vec4(f_pos, 0, 1);
}
