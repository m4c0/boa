//#define OPT "-gdwarf"
#define OPT "-O3"

#define CFLAGS OPT, "-IVulkan-Headers/include"
#define RES_PATH "."
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
      "-o", "boas.exe",
      OBJS, "vulkan-win.o", "volk.o", "vlk.o", "main.res",
      "-lole32", "-luser32");
  return 0;
}

static int icon() {
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

static void print_key(FILE * f, const char * p) {
  char * env = getenv(p);
  if (strncmp(p, "WIN_", 4)) {
    assert(fprintf(f, "&%s;", p));
  } else if (env) {
    assert(fprintf(f, "%s", env));
  } else {
    fprintf(stderr, "Missing environment: %s\n", p);
    exit(1);
  }
}
static int pack() {
  if (getenv("WIN_BUILD_ONLY")) return 0;

  // https://learn.microsoft.com/en-us/uwp/schemas/appxpackage/how-to-create-a-basic-package-manifest
  if (apply("AppxManifest.xml.in", "AppxManifest.xml")) return 1;

  unlink("boas.msix");

  char argv0[1024];
  snprintf(argv0, 1024,
      "c:\\Program Files (x86)\\Windows Kits\\10\\bin\\%s\\x64\\makeappx.exe",
      getenv("WIN_KIT_VERSION"));

  char argv1[1024];
  snprintf(argv1, 1024, "\"%s\"", argv0);
  return _spawnl(_P_WAIT, argv0, argv1, "pack", "/f", "AppxMapping.ini", "/p", "boas.msix", NULL);
}

int main(int argc, char ** argv) {
  if (pch()) return 1;

  if (icon())    return 1;
  if (shaders()) return 1;
  RUN("llvm-rc", "/FO", "main.res", "main.rc");

  CC("volk");
  CC("vulkan-win");
  HDR("vlk", "VLK_IMPLEMENTATION");
  if (compile_and_link_exe()) return 1;

  if (pack()) return 1;

  return 0;
}
