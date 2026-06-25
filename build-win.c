#include <sys/stat.h>
#include <assert.h>
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>

//#define OPT "-gdwarf"
#define OPT "-O3"

#define RES_PATH "app"

#define CFLAGS OPT, "-IVulkan-Headers/include"

static void usage() {
  fprintf(stderr, "just call 'build' without arguments\n");
}

static int link_exe() {
  RUN("clang", "-Wall", OPT,
      "-o", "app/boas.exe",
      "gme.o", "sfx.o", "snd.o", "snk.o", "tmr.o",
      "vulkan.o", "vulkan-win.o",
      "-lole32", "-luser32");
  return 0;
}

int main(int argc, char ** argv) {
  if (argc != 1) return (usage(), 1);

  _mkdir("app");

  CC("vulkan.c",     "vulkan.o",     CFLAGS);
  CC("vulkan-win.c", "vulkan-win.o", CFLAGS);
  HDR("gme.h", "gme.o", CFLAGS, "-D", "GME_IMPLEMENTATION");
  HDR("sfx.h", "sfx.o", CFLAGS, "-D", "SFX_IMPLEMENTATION");
  HDR("snd.h", "snd.o", CFLAGS, "-D", "SND_IMPLEMENTATION");
  HDR("snk.h", "snk.o", CFLAGS, "-D", "SNK_IMPLEMENTATION");
  HDR("tmr.h", "tmr.o", CFLAGS, "-D", "TMR_IMPLEMENTATION");
  if (link_exe()) return 1;

  SHADER("boav.frag", RES_PATH);
  SHADER("boav.vert", RES_PATH);

  return 0;
}
