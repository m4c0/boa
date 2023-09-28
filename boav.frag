#version 450

#define PI 3.14159265358979323

layout(push_constant) uniform upc {
  float aspect;
  float time;
} pc;

layout(location = 0) in vec2 frag_coord;

layout(location = 0) out vec4 frag_colour;

float rand(vec2 co) {
  // https://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
  return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 hsv2rgb(vec3 hsv) {
  // https://en.wikipedia.org/wiki/HSL_and_HSV
  vec3 n = vec3(5, 3, 1);
  vec3 k = mod(n + hsv.r / (PI / 3.0), 6.0);
  return hsv.b - hsv.b * hsv.g * max(min(min(k, 4.0 - k), 1.0), 0.0);
}

vec2 polar(vec2 p) {
  float r = sqrt(p.x * p.x + p.y * p.y);
  float phi = sign(p.y) * acos(p.x / r);
  return vec2(r, phi);
}

mat2 rot(float a) {
  return mat2(cos(a), -sin(a), sin(a), cos(a));
}
vec2 op_rot(vec2 p, float a) {
  return mat2(cos(a), -sin(a), sin(a), cos(a)) * p;
}

void main() { 
  vec2 p = frag_coord;

  p = p * 5.0;
  p = op_rot(p, 0.1);

  float rp = rand(floor(p));

  float hue = 1.8 + rp * 0.8;

  float sat = 0.5 + rp * 0.5;

  float val = 1.0 - rp * 0.5;
  //val = val * smoothstep(0.0, 0.3, fract(p.x)) * 0.4 + 0.4;
  val = val * smoothstep(0.0, 0.05, min(fract(p.y), fract(p.x)));

  vec3 rgb = hsv2rgb(vec3(hue, sat, val));
  frag_colour = vec4(rgb, 1); 
}
