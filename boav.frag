#version 450

#define PI 3.14159265358979323

layout(push_constant) uniform upc {
  float aspect;
  float time;
  vec2 grid;
} pc;
layout(set = 0, binding = 0) readonly buffer usb {
  float grid[];
} sb;

layout(location = 0) in vec2 frag_coord;
layout(location = 1) in vec2 frag_grid;

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

vec3 background(vec2 p) {
  p = p * 5.0;
  p = op_rot(p, PI + 0.3 * sin(pc.time * 0.025));

  float col = floor(p.x);
  p.y -= sin(pc.time * 0.1 + col) * rand(vec2(123.456, col));

  float rp = rand(floor(p));

  float hue = 1.8 + rp * 0.8;

  float sat = 0.5 + rp * 0.5;

  float val = 1.0 - rp * 0.5;
  val = val * smoothstep(0.1, 0.3, fract(p.x)) * 0.4 + 0.4;
  val = val * smoothstep(0.0, 0.1, min(fract(p.y), fract(p.x)));

  return hsv2rgb(vec3(hue, sat, val)) * 0.025;
}

// b = half size
float sd_box(vec2 p, vec2 b) {
  vec2 d = abs(p) - b;
  return length(max(d, 0)) + min(max(d.x, d.y), 0);
}

float grid(vec2 p) {
  ivec2 idx = ivec2(p);
  return sb.grid[idx.y * int(pc.grid.x) + idx.x];
}
float sd_snake(vec2 p) {
  float i = grid(p);

  if (i > 0) {
    vec2 uv = fract(p);
    vec2 st = 1.0 - uv;

    vec4 sa = vec4(uv.x, st.x, uv.y, st.y);
    vec4 va = vec4(
      grid(p + vec2(1.0, 0.0)),
      grid(p + vec2(-1.0, 0.0)),
      grid(p + vec2(0.0, 1.0)),
      grid(p + vec2(0.0, -1.0))
    );
    vec4 wa = smoothstep(0.8, 0.9, sa) * (1.0 - va);
    float ma = max(wa.x, max(wa.y, max(wa.z, wa.w)));

    vec4 sb = vec4(uv.x * uv.y, st.x * uv.y, st.x * st.y, uv.x * st.y);
    vec4 vb = vec4(
      grid(p + vec2(1.0, 1.0)),
      grid(p + vec2(-1.0, 1.0)),
      grid(p + vec2(-1.0, -1.0)),
      grid(p + vec2(1.0, -1.0))
    );
    vec4 wb = smoothstep(0.8, 0.9, sb) * (1.0 - vb);
    float mb = max(wb.x, max(wb.y, max(wb.z, wb.w)));

    return max(ma, mb);
  }
 
  return 0;
}

vec3 snake(vec2 p) {
  float d = sd_snake(frag_grid);

  // d = 0.001 / abs(d);

  return vec3(d);
}

void main() { 
  vec2 p = frag_coord;

  vec3 rgb = background(p) + snake(frag_grid);

  frag_colour = vec4(rgb, 1); 
}
