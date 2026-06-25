#include <sys/stat.h>
#include <assert.h>
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>

//#define OPT "-gdwarf"
#define OPT "-O3"

#define RES_PATH "app"

static void usage() {
  fprintf(stderr, "just call 'build' without arguments\n");
}

static int cc(char * src, char * o) {
  CC(src, o, OPT, "-IVulkan-Headers/include");
  return 0;
}

static int hdr(char * src, char * o, char * d) {
  HDR(src, o, OPT, "-D", d);
  return 0;
}

static int link_exe() {
  CC("clang", "-Wall", OPT,
      "-o", "app/boas.exe",
      "gme.o", "sfx.o", "snd.o", "snk.o", "tmr.o",
      "vulkan.o", "vulkan-win.o",
      "-lole32", "-luser32");
  return 0;
}

int main(int argc, char ** argv) {
  if (argc != 1) return (usage(), 1);

  _mkdir("app");

  if (cc("vulkan.c",     "vulkan.o"    )) return 1;
  if (cc("vulkan-win.c", "vulkan-win.o")) return 1;
  if (hdr("gme.h", "gme.o", "GME_IMPLEMENTATION")) return 1;
  if (hdr("sfx.h", "sfx.o", "SFX_IMPLEMENTATION")) return 1;
  if (hdr("snd.h", "snd.o", "SND_IMPLEMENTATION")) return 1;
  if (hdr("snk.h", "snk.o", "SNK_IMPLEMENTATION")) return 1;
  if (hdr("tmr.h", "tmr.o", "TMR_IMPLEMENTATION")) return 1;
  if (link_exe()) return 1;

  SHADER("boav.frag", RES_PATH);
  SHADER("boav.vert", RES_PATH);

  return 0;
}
