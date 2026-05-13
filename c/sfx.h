#pragma once

void sfx_init();
void sfx_reset();
void sfx_fill(float * buf, unsigned len);

void sfx_death();
void sfx_eat();
void sfx_walk();

#ifdef SFX_IMPLEMENTATION

#ifdef _WIN32
#define WIN32_MEAN_AND_LEAN
#include <windows.h>
#else
#include <sys/time.h>
#endif
#include <assert.h>
#include <stdlib.h>

static struct timeval sfx_tv = {0};

static float sfx_walk_t  = -1;
static float sfx_walk_dt = -1;
static float sfx_eat_t   = -1;
static float sfx_eat_dt  = -1;
static float sfx_death_t = -1;

static void sfx_gettime(struct timeval * tv) {
#ifdef _WIN32
  SYSTEMTIME st; GetSystemTime(&st);

  // Contains a 64-bit value representing the number of 100-nanosecond
  // intervals since January 1, 1601 (UTC).
  FILETIME ft; assert(SystemTimeToFileTime(&st, &ft));

  ULARGE_INTEGER i;
  i.u.LowPart  = ft.dwLowDateTime;
  i.u.HighPart = ft.dwHighDateTime;

  ULONGLONG usec = i.QuadPart / 10;
  tv->tv_sec  = usec / (1000*1000);
  tv->tv_usec = usec % (1000*1000);
#else
  gettimeofday(tv, NULL);
#endif
}

static float sfx_now() {
  struct timeval tv; sfx_gettime(&tv);

  float secs = tv.tv_sec - sfx_tv.tv_sec;
  float usecs = tv.tv_usec - sfx_tv.tv_usec;
  return secs + usecs / (1000*1000.f);
}
static float sfx_randf() {
  return (float)rand() / (float)RAND_MAX;
}

void sfx_init() {
  sfx_gettime(&sfx_tv);
}
void sfx_reset() {
  sfx_eat_t = sfx_walk_t = -1;
}

void sfx_death() {
  sfx_death_t = sfx_now();
}
void sfx_eat() {
  sfx_eat_t = sfx_now();
  sfx_eat_dt = 1.0 + 0.1 * sfx_randf();
}
void sfx_walk() {
  sfx_walk_t = sfx_now();
  sfx_walk_dt = 1.0 + 0.1 * sfx_randf();
}

// Perlin's original permutation
static const int sfx_perlin[256] = {
  151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225,
  140, 36,  103, 30,  69,  142, 8,   99,  37,  240, 21,  10,  23,  190, 6,   148,
  247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219, 203, 117, 35,  11,  32,
  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136, 171, 168, 68,  175,
  74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158, 231, 83,  111, 229, 122,
  60,  211, 133, 230, 220, 105, 92,  41,  55,  46,  245, 40,  244, 102, 143, 54,
  65,  25,  63,  161, 1,   216, 80,  73,  209, 76,  132, 187, 208, 89,  18,  169,
  200, 196, 135, 130, 116, 188, 159, 86,  164, 100, 109, 198, 173, 186, 3,   64,
  52,  217, 226, 250, 124, 123, 5,   202, 38,  147, 118, 126, 255, 82,  85,  212,
  207, 206, 59,  227, 47,  16,  58,  17,  182, 189, 28,  42,  223, 183, 170, 213,
  119, 248, 152, 2,   44,  154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,
  129, 22,  39,  253, 19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104,
  218, 246, 97,  228, 251, 34,  242, 193, 238, 210, 144, 12,  191, 179, 162, 241,
  81,  51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181, 199, 106, 157,
  184, 84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,
  222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156, 180
};
static float sfx_noise(float t) {
  return sfx_perlin[(unsigned)(t * 4) % 256] > 128 ? 1 : -1;
}
void sfx_fill(float * buf, unsigned len) {
  float ref = sfx_now();
  for (int i = 0; i < len; i++) {
    float t = ref + i / 44100.f;

    float d = 0;
    if (sfx_death_t > 0) {
      float n = t - sfx_death_t;
      n = n * 2;
      n = 1.0 / (10.0 * n);
      n = n < 0 ? 0 : n > 1 ? 1 : n;
      n = n * 0.2;
      d = n * sfx_noise(t * 130.81); // C3 (MIDI)
    }

    float e = 0;
    if (sfx_eat_t > 0) {
      float n = t - sfx_eat_t;
      n = n * 3.0;
      n = -4 * n * n + 4 * n;
      n = n < 0 ? 0 : n > 1 ? 1 : n;
      n = n * 0.3;

      float tt = sfx_eat_dt * t * 261.63; // C4 (MIDI)
      float tf = tt - (int)tt;
      float tri = tf < 0.5f
        ? tf * 4.0f - 1.0f
        : tf * -4.0f + 3.0f;
      e = n * tri;
    }

    float w = 0;
    if (sfx_walk_t > 0) {
      float n = t - sfx_walk_t;
      n = n * 6.0;
      n = 1.0 / (10.0 * n);
      n = n < 0 ? 0 : n > 1 ? 1 : n;
      n = n * 0.01;
      w = n * sfx_noise(sfx_walk_dt * t * 1046.50); // C6 (MIDI)
    }

    *buf++ = (d + e + w) * 0.5; // global volume
  }
}

#endif
