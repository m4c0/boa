#define SFX_IMPLEMENTATION
#include "sfx.h"

#include <stdio.h>

int main() {
  sfx_init();
  printf("%f\n", sfx_now());
  Sleep(2000);
  printf("%f\n", sfx_now());
}
