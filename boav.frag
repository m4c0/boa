#version 450

#define PI 3.14159265358979323

layout(push_constant) uniform upc {
  float aspect;
  float time;
} pc;

layout(location = 0) in vec2 frag_coord;

layout(location = 0) out vec4 frag_colour;

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

void main() { 
  vec2 pol = polar(frag_coord);
  vec3 rgb = hsv2rgb(vec3(pol.y, 1.0, pol.x));

  frag_colour = vec4(rgb, 1); 
}
