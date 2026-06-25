#include "build.h"

#include <sys/stat.h>

static void usage() {
  fprintf(stderr, "just call 'build' without arguments\n");
}

static int shader(char * name) {
  char spv[1024];
  sprintf(spv, "boas.app/Contents/Resources/%s.spv", name);
  RUN("glslang", "-V", name, "-o", spv);
  return 0;
}

static int cc(char * src, char * o) {
  CC(src, o, "-g", "-IVulkan-Headers/include");
  return 0;
}

static int hdr(char * src, char * o, char * d) {
  HDR(src, o, "-g", "-D", d);
  return 0;
}

static int link_exe() {
  RUN("clang", "-Wall",
    "-framework", "AppKit",
    "-framework", "AudioToolbox",
    "-framework", "MetalKit",
    "-o", "boas.app/Contents/MacOS/boas", 
    "gme.o", "sfx.o", "snd.o", "snk.o", "tmr.o",
    "vulkan.o", "vulkan-osx.o");
  return 0;
}

int main(int argc, char ** argv) {
  if (argc != 1) return (usage(), 1);

  mkdir("boas.app", 0777);
  mkdir("boas.app/Contents", 0777);
  mkdir("boas.app/Contents/MacOS", 0777);
  mkdir("boas.app/Contents/Resources", 0777);

  RUN("cp", "libvulkan.dylib", "boas.app/Contents/MacOS/");

  if (cc("vulkan.c",     "vulkan.o"    )) return 1;
  if (cc("vulkan-osx.m", "vulkan-osx.o")) return 1;
  if (hdr("gme.h", "gme.o", "GME_IMPLEMENTATION")) return 1;
  if (hdr("sfx.h", "sfx.o", "SFX_IMPLEMENTATION")) return 1;
  if (hdr("snd.h", "snd.o", "SND_IMPLEMENTATION")) return 1;
  if (hdr("snk.h", "snk.o", "SNK_IMPLEMENTATION")) return 1;
  if (hdr("tmr.h", "tmr.o", "TMR_IMPLEMENTATION")) return 1;
  if (link_exe()) return 1;

  if (shader("boav.frag")) return 1;
  if (shader("boav.vert")) return 1;

  return 0;
}
