#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#include <sys/stat.h>
#include <assert.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>

static void usage() {
  fprintf(stderr, "just call 'build' without arguments\n");
}

static int run(char ** args) {
  assert(args && args[0]);

  if (0 == _spawnvp(_P_WAIT, args[0], (const char * const *)args)) {
    return 0;
  }

  fprintf(stderr, "failed to run child process: %s\n", args[0]);
  return 1;
}

static int shader(char * name) {
  char spv[1024];
  sprintf(spv, "%s.spv", name);

  char * args[] = { "glslang", "-V", name, "-o", spv, 0 };
  return run(args);
}

static int cc(char * src, char * o) {
  char * args[] = {
    "clang", "-Wall", "-gdwarf",
    "-IVulkan-Headers/include",
    "-o", o, "-c", src, 0 };
  return run(args);
}

static int hdr(char * src, char * o, char * d) {
  char * args[] = {
    "clang", "-Wall", "-x", "c", "-gdwarf", "-D", d, "-o", o, "-c", src, 0
  };
  return run(args);
}

static int link_exe() {
  char * args[] = {
    "clang", "-Wall", "-gdwarf",
    "-o", "boas.exe",
    "gme.o", "sfx.o", "snd.o", "snk.o", "tmr.o",
    "vulkan.o", "vulkan-win.o",
    "-lole32", "-luser32",
    0 };
  return run(args);
}

int main(int argc, char ** argv) {
  if (argc != 1) return (usage(), 1);

  if (cc("vulkan.c",     "vulkan.o"    )) return 1;
  if (cc("vulkan-win.c", "vulkan-win.o")) return 1;
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
