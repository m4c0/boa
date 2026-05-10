#define SND_IMPLEMENTATION
#include "snd.h"

#include <math.h>

static int sample;
static void c4_midi(float * f, unsigned n) {
  // This should generate a smooth C4 in MIDI scale, with a small "click" at
  // start due to the abrupt change from silent to full note.
  for (int i = 0; i < n; i++, sample++) {
    float t = (float)sample / 44100.f;
    f[i] = sin(261.63 * t * 6.2830);
  }
}

int main() {
  snd_init(&c4_midi);
#ifdef _WIN32
  Sleep(2000);
#else
  sleep(2);
#endif
  snd_deinit();
}
