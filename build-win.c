//#define OPT "-gdwarf"
#define OPT "-O3"

#define CFLAGS OPT, "-IVulkan-Headers/include"
#define RES_PATH "app"
#include "build.h"

#include <direct.h>
#include <stdio.h>

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
      OBJS, "vulkan-win.o", "main.res",
      "-lole32", "-luser32");
  return 0;
}

int icon() {
  unsigned sz;
  char * img = slurp("Assets.xcassets\\AppIcon.appiconset\\Icon-1024.png", &sz);

  FILE * f = fopen("icon.ico", "wb");
  fwrite("\0\0\1\0\1\0", 6, 1, f); // 0=Reserved; 1=ICO; 1 Image
  fwrite("\0\0\0\0\0\0\x20\0", 8, 1, f); // W/H/C/Res. Planes/Bits

  fwrite(&sz, 4, 1, f);
  fwrite("\x16\0\0\0", 4, 1, f); // 20=offset from BOS
  fwrite(img, sz, 1, f);

  fclose(f);
  return 0;
}

int main(int argc, char ** argv) {
  _mkdir("app");

  if (pch()) return 1;

  if (icon()) return 1;
  RUN("llvm-rc", "/FO", "main.res", "main.rc");

  CC("vulkan-win");
  if (compile_and_link_exe()) return 1;
  if (shaders()) return 1;

  return 0;
}
