#ifdef ARCH
#  define CFLAGS \
  "-fdata-sections", "-ffunction-sections", "-funwind-tables", \
  "-fstack-protector-strong", "-no-canonical-prefixes", \
  "--target=" ARCH, \
  "--sysroot", ANDROID_NDK_PREBUILT_ROOT "/sysroot/", \
  "-IVulkan-Headers/include"
#  define RES_PATH ""
#else
#  define CFLAGS ""
#  define RES_PATH "droid"
#endif
#include "build.h"

#include <sys/stat.h>

#ifdef ARCH
#define OBJ(x) ("droid/" ARCH "/" x)

static int link_exe() {
  RUN("clang", "-Wall", "-shared",
      "-Wl,-Bsymbolic", "-fuse-ld=lld", "-Wl,--no-undefined",
      "-resource-dir", ANDROID_NDK_PREBUILT_ROOT "/lib/clang/21",
      "--target=" ARCH,
      "--sysroot", ANDROID_NDK_PREBUILT_ROOT "/sysroot/",
      "-o", "droid/" ARCH "/libboas.so", 
      OBJS, "vulkan-droid.o");
  return 0;
}

#else
static int link_exe() { return 1; } // removes warning
#endif

static int meta(char * tgt) {
  fprintf(stderr, "Building for target %s\n", tgt);

  char * ndk = getenv("ANDROID_NDK_PREBUILT_ROOT");
  assert(ndk && "missing env var ANDROID_NDK_PREBUILT_ROOT");

  char o[1024];
  snprintf(o, 1024, "droid/build-%s", tgt);
  char d[1024];
  snprintf(d, 1024, "-DARCH=\"%s\"", tgt);
  char n[1024];
  snprintf(n, 1024, "-DANDROID_NDK_PREBUILT_ROOT=\"%s\"", ndk);

  RUN("clang", n, d, "-o", o, "build-droid.c");
  RUN(o);
  return 0;
}

int main(int argc, char ** argv) {
#ifndef ARCH
  mkdir("droid", 0777);
  if (meta("aarch64-none-linux-android26"  )) return 1;
  if (meta("armv7-none-linux-androideabi26")) return 1;
  if (meta("i686-none-linux-android26"     )) return 1;
  if (meta("x86_64-none-linux-android26"   )) return 1;

  if (shaders()) return 1;

  char * dir = getenv("ANDROID_BUILD_TOOLS");
  assert(dir && "missing env for ANDROID_BUILD_TOOLS");

  char buf[1024];
  snprintf(buf, 1024, "%s/aapt2", dir);
  RUN(buf, "compile", "res/values/strings.xml", "-o", ".");

  return 0;
#else
  mkdir("droid/" ARCH, 0777);

  CC("vulkan-droid.c", "vulkan-droid.o", CFLAGS);
  if (compile_and_link_exe()) return 1;

  return 0;
#endif
}

