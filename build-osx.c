#define CFLAGS "-g", "-IVulkan-Headers/include"
#define RES_PATH "boas.app/Contents/Resources"
#include "build.h"

#include <sys/stat.h>

static void usage() {
  fprintf(stderr, "just call 'build' without arguments\n");
}

static int link_exe() {
  RUN("clang", "-Wall",
    "-framework", "AppKit",
    "-framework", "AudioToolbox",
    "-framework", "MetalKit",
    "-o", "boas.app/Contents/MacOS/boas", 
    OBJS, "vulkan-osx.o");
  return 0;
}

int main(int argc, char ** argv) {
  if (argc != 1) return (usage(), 1);

  mkdir("boas.app", 0777);
  mkdir("boas.app/Contents", 0777);
  mkdir("boas.app/Contents/MacOS", 0777);
  mkdir("boas.app/Contents/Resources", 0777);

  RUN("cp", "libvulkan.dylib", "boas.app/Contents/MacOS/");

  CC("vulkan-osx.m", "vulkan-osx.o", CFLAGS);
  if (compile_and_link_exe()) return 1;

  return 0;
}
