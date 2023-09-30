#version 450

#define PI 3.14159265358979323

layout(push_constant) uniform upc {
  float aspect;
  float time;
  vec2 grid;
  vec2 food;
} pc;
layout(set = 0, binding = 0) readonly buffer usb {
  vec2 grid[];
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

float sd_circle(vec2 p, float b) {
  return length(p) - b;
}

vec2 grid(vec2 p) {
  ivec2 idx = ivec2(p);
  return sb.grid[idx.y * int(pc.grid.x) + idx.x];
}
float is_snake(vec2 p, float dx, float dy) {
  vec2 g = grid(p + vec2(dx, dy));
  return step(0.0001, g.y);
}
float edge_snake(vec2 p) {
  float i = is_snake(p, 0.0, 0.0);

  if (i == 1) {
    vec2 uv = fract(p);
    vec2 st = 1.0 - uv;

    vec4 sa = vec4(uv.x, st.x, uv.y, st.y);
    vec4 va = vec4(
      is_snake(p, 1.0, 0.0),
      is_snake(p, -1.0, 0.0),
      is_snake(p, 0.0, 1.0),
      is_snake(p, 0.0, -1.0)
    );
    vec4 wa = smoothstep(0.0, 1.0, sa) * (1.0 - va);
    float ma = max(wa.x, max(wa.y, max(wa.z, wa.w)));

    vec4 sb = vec4(uv.x * uv.y, st.x * uv.y, st.x * st.y, uv.x * st.y);
    vec4 vb = vec4(
      is_snake(p, 1.0, 1.0),
      is_snake(p, -1.0, 1.0),
      is_snake(p, -1.0, -1.0),
      is_snake(p, 1.0, -1.0)
    );
    vec4 wb = smoothstep(0.0, 1.0, sb) * (1.0 - vb);
    float mb = max(wb.x, max(wb.y, max(wb.z, wb.w)));

    return max(ma, mb);
  }
 
  return 0;
}

vec4 snake2(vec2 p) {
  float e = edge_snake(p);
  float val = 1.0 - e;

  vec2 pp = fract(p * 0.5) - 0.5;
  float hue = step(pp.x * pp.y, 0.0);

  float sat = 1.0;

  vec3 rgb = hsv2rgb(vec3(hue, sat, val));
  rgb = pow(rgb, vec3(0.7));
  return vec4(rgb, e);
}

vec4 snake(vec2 p) {
  vec2 gp = grid(p);

  float tail = smoothstep(0.0, 0.6, gp.y);
  tail = 0.7 * tail + 0.3;

  float dt = pc.time - gp.x;

  float i = smoothstep(0.0, 1.0, dt * 5.0);
  i = i * mix(0.0, tail, step(0.0, gp.y));
  i = i * (1.0 + 0.4 * sin((pc.time + gp.y) * 5.0));
  i = 2.0 * i; // TODO: curve over direction, like "l"

  //float l = 1.0 - sin(0.0 * p.y + pc.time);
    
  vec2 dd = sin(mod(p * 3.14, 3.14)) * 0.5;
    
  float c = dd.x * dd.y * i;

  vec3 rgb = vec3(4.0, 12.0, 2.0) * c;
  float a = is_snake(p, 0.0, 0.0) * pow(c, 0.8);
  return vec4(rgb, a);
}

vec3 food(vec2 p) {
  p = p - pc.food - 0.5;

  float r1 = 0.2 + 0.05 * sin(-2.0 * pc.time);
  float d1 = sd_circle(p, r1);

  float t0 = smoothstep(7.0, 8.0, 8.0 * fract(pc.time / 8.0));
  float r0 = max(r1, 3.0 * pow(t0, 3.0));
  float d0 = sd_circle(p, r0);

  float hue = mix(2.9, 3.0, smoothstep(-0.5, -0.2, d1));

  float val = min(abs(d0), abs(d1));
  val = 0.1 / val;
  val = mix(val, 0.5, step(d1, 0));

  float a = 1.0 - smoothstep(0.0, 3.0, length(p));
  return hsv2rgb(vec3(hue, 1.0, val)) * a;
}

void main() { 
  vec3 bg = background(frag_coord);
  vec4 sn = snake(frag_grid);
  vec3 fd = food(frag_grid);

  vec3 rgb = mix(bg, sn.rgb, sn.a) + fd;

  frag_colour = vec4(rgb, 1); 
}
