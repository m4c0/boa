//#define OPT "-gdwarf"
#define OPT "-O3"
#include <stdio.h>

#define CFLAGS OPT, "-IVulkan-Headers/include"
#define RES_PATH "app"
#include "build.h"

#include <direct.h>

static int link_exe() {
  RUN("clang", "-Wall", OPT,
      "-o", "app/boas.exe",
      OBJS, "vulkan-win.o",
      "-lole32", "-luser32");
  return 0;
}

int main(int argc, char ** argv) {
  _mkdir("app");

  CC("vulkan-win.c", "vulkan-win.o", CFLAGS);
  if (compile_and_link_exe()) return 1;

  return 0;
}
