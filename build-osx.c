#define APP "boas"

#define CFLAGS "-g", "-IVulkan-Headers/include"
#define RES_PATH "."
#include "build.h"

#include <sys/stat.h>

static void print_key(FILE * f, const char * key) {}

static int pch() {
  RUN("clang", "-Wall", "-g", "-x", "c-header",
    "-IVulkan-Headers/include",
    "-D", "VK_USE_PLATFORM_METAL_EXT",
    "-D", "VLK_USE_VOLK",
    "-o", "pch.pch", "pch.h");
  return 0;
}

static int link_exe() {
  RUN("clang", "-Wall",
    "-framework", "AppKit",
    "-framework", "AudioToolbox",
    "-framework", "Metal",
    "-framework", "MetalKit",
    "-o", APP".app/Contents/MacOS/main", 
    OBJS, "app-osx.o", "volk.o");
  return 0;
}

static int link_shots_exe() {
  RUN("clang", "-Wall",
    "-framework", "AppKit",
    "-framework", "AudioToolbox",
    "-framework", "MetalKit",
    "-o", APP".app/Contents/MacOS/shots", 
    OBJS, "volk.o", "shots.o", "vlk.o");
  return 0;
}

#define CROSS(X) RUN("spirv-cross", "shader."X".spv", "--msl", "--output", APP".app/Contents/Resources/shader."X".metal", "--flip-vert-y");

int main(int argc, char ** argv) {
  mkdir(APP".app", 0777);
  mkdir(APP".app/Contents", 0777);
  mkdir(APP".app/Contents/MacOS", 0777);
  mkdir(APP".app/Contents/Resources", 0777);

  RUN("cp", "libvulkan.dylib", APP".app/Contents/MacOS/");

  if (pch()) return 1;

  CC("volk");
  CM("app-osx");
  if (compile_and_link_exe()) return 1;
  if (shaders()) return 1;

  CROSS("frag");
  CROSS("vert");

  CC("shots");
  HDR("vlk", "VLK_IMPLEMENTATION");
  if (link_shots_exe()) return 1;

  return 0;
}
