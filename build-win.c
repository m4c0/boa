//#define OPT "-gdwarf"
#define OPT "-O3"
#include <stdio.h>

#define CFLAGS OPT, "-IVulkan-Headers/include"
#define RES_PATH "app"
#include "build.h"

#include <direct.h>

static int pch() {
  RUN("clang", "-Wall", "-x", "c-header", CFLAGS,
      "-D", "VK_USE_PLATFORM_WIN32_KHR",
      "-D", "VLK_USE_VOLK",
      "-o", "pch.pch", "pch.h");
  return 0;
}

static int link_exe() {
  RUN("clang", "-Wall", OPT,
      "-o", "app/boas.exe",
      OBJS, "vulkan-win.o",
      "-lole32", "-luser32");
  return 0;
}

int main(int argc, char ** argv) {
  _mkdir("app");

  if (pch()) return 1;

  CC("vulkan-win");
  if (compile_and_link_exe()) return 1;
  if (shaders()) return 1;

  return 0;
}
