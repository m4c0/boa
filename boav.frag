#version 450

#define PI 3.14159265358979323

layout(push_constant) uniform upc {
  float aspect;
  float time;
  float dead_at;
  vec2 grid;
  vec2 food;
  vec2 party;
  float party_start;
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

float sd_circle(vec2 p, float b) {
  return length(p) - b;
}
float sd_box(vec2 p, vec2 b) {
  vec2 d = abs(p) - b;
  return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

mat2 rot(float a) {
  return mat2(cos(a), -sin(a), sin(a), cos(a));
}
vec2 op_rot(vec2 p, float a) {
  return mat2(cos(a), -sin(a), sin(a), cos(a)) * p;
}

vec3 raw_background(vec2 p) {
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
vec3 background(vec2 p) {
  return raw_background(op_rot(p, length(p)));
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

// [0;1] from the start of the death
float death_factor() {
  float da = mix(0.0, pc.time - pc.dead_at, step(0.0001, pc.dead_at));
  return clamp(da, 0.0, 1.0);
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
  float da = death_factor();

  float tail = smoothstep(0.0, 0.6, gp.y);
  tail = 0.7 * tail + 0.3;

  float dt = pc.time - gp.x;

  float squirm = sin((pc.time + gp.y) * 5.0);
  squirm *= 1.0 - da;

  float i = smoothstep(0.0, 1.0, dt * 5.0);
  i = i * mix(0.0, tail, step(0.0, gp.y));
  i = i * (1.0 + 0.4 * squirm);
  i = 2.0 * i; // TODO: curve over direction, like "l"

  //float l = 1.0 - sin(0.0 * p.y + pc.time);
    
  vec2 dd = sin(mod(p * 3.14, 3.14)) * 0.5;
    
  float c = dd.x * dd.y * i;

  vec3 alive_c = vec3(4.0, 12.0, 2.0);
  vec3 dead_c = vec3(0.4, 0.6, 0.2);
  vec3 base_c = mix(alive_c, dead_c, pow(da, 0.3));

  vec3 rgb = base_c * c;
  float a = is_snake(p, 0.0, 0.0) * pow(c, 0.8);
  a = a * pow(1.0 - da, 0.3);
  return vec4(rgb, a);
}

vec3 food(vec2 p) {
  p = p - pc.food - 0.5;

  // Breathe
  float r1 = 0.2 + 0.05 * sin(-2.0 * pc.time);
  float d1 = sd_circle(p, r1);

  // Blip
  float t0 = smoothstep(7.0, 8.0, 8.0 * fract(pc.time / 8.0));
  float r0 = max(r1, 3.0 * pow(t0, 3.0));
  float d0 = sd_circle(p, r0);

  float h = smoothstep(-r0, 0.0, d1);
  float hue = mix(0.0, 1.0, 0.5 + 0.5 * sin(d1));

  float val = min(abs(d0), abs(d1));
  val = 0.1 / val;
  val = mix(val, 0.9, step(d1, 0));
  val = val * (1.0 - death_factor());

  float a = 1.0 - smoothstep(0.0, 3.0, length(p));
  return hsv2rgb(vec3(hue, 1.0, val)) * a;
}

vec4 party_part(vec2 p, float n, float r) {
  vec2 q = p - pc.party - 0.5;
  float t = pc.time - pc.party_start;
  t = clamp(t * 2.0, 0.0, 1.0);
  q = op_rot(q, t * 2.0);

  const float ssz = 3.14 * 2.0 / n;
  float sec = ssz * round(polar(q).y / ssz);
  vec2 qs = op_rot(q, sec);
  qs -= vec2(t * 2.0 * r, 0);

  float d = sd_circle(qs, 0.7 * sin(t * 3.1415));
  vec3 ca0 = vec3(2.0, 0.0, 0.0);
  vec3 ca1 = vec3(2.4, 2.0, 0.0);
  vec3 cam = mix(ca0, ca1, cos(d * 2.0) * 0.5 + 0.5);
  vec4 ca = vec4(cam, 0.8 + sin(d * 5.0) * 0.3);
  vec4 cb = vec4(2.0, 0.0, 0.0, 0.01 / abs(d));
  vec4 c = mix(ca, cb, smoothstep(-0.1, 0.0, d));

  return vec4(c.rgb, (1.0 - t) * c.a * 0.8);
}
vec4 party(vec2 p) {
  return party_part(p, 7.0, 1.0) + party_part(p, 23.0, 1.5);
}

void main() { 
  vec3 bg = background(frag_coord);
  vec4 sn = snake(frag_grid);
  vec3 fd = food(frag_grid);
  vec4 pt = party(frag_grid);

  vec3 rgb = mix(bg, sn.rgb, sn.a) + fd;
  rgb = mix(rgb, pt.rgb, pt.a);

  frag_colour = vec4(rgb, 1); 
}
