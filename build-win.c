#define CFLAGS OPT, "-IVulkan-Headers/include"
#include "build.h"

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

static int link_exe() {
  RUN("clang", "-Wall", OPT,
      "-o", "app/boas.exe",
      OBJS, "vulkan-win.o",
      "-lole32", "-luser32");
  return 0;
}

int main(int argc, char ** argv) {
  if (argc != 1) return (usage(), 1);

  _mkdir("app");

  CC("vulkan-win.c", "vulkan-win.o", CFLAGS);
  if (compile_common()) return 1;;
  if (link_exe()) return 1;

  SHADER("boav.frag", RES_PATH);
  SHADER("boav.vert", RES_PATH);

  return 0;
}
