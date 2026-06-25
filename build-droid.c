#include "build.h"

#include <sys/stat.h>

#define RES_PATH "droid"

#ifdef ARCH
#define OBJ(x) ("droid/" ARCH "/" x)

static int cc(char * src, char * o) {
  CC(src, o,
      "-fdata-sections", "-ffunction-sections", "-funwind-tables",
      "-fstack-protector-strong", "-no-canonical-prefixes",
      "--target=" ARCH,
      "--sysroot", ANDROID_NDK_PREBUILT_ROOT "/sysroot/",
      "-IVulkan-Headers/include");
  return 0;
}
#define CC1(src, o) cc(src, "droid/" ARCH "/" o)

static int hdr(char * src, char * o, char * d) {
  HDR(src, o,
      "-fdata-sections", "-ffunction-sections", "-funwind-tables",
      "-fstack-protector-strong", "-no-canonical-prefixes",
      "--target=" ARCH,
      "--sysroot", ANDROID_NDK_PREBUILT_ROOT "/sysroot/",
      "-D", d);
  return 0;
}
#define HDR1(src, o, d) hdr(src, OBJ(o), d)

static int link_exe() {
  RUN("clang", "-Wall", "-shared",
      "-Wl,-Bsymbolic", "-fuse-ld=lld", "-Wl,--no-undefined",
      "-resource-dir", ANDROID_NDK_PREBUILT_ROOT "/lib/clang/21",
      "--target=" ARCH,
      "--sysroot", ANDROID_NDK_PREBUILT_ROOT "/sysroot/",
      "-o", "droid/" ARCH "/libboas.so", 
      OBJ("gme.o"),
      OBJ("sfx.o"),
      OBJ("snd.o"),
      OBJ("snk.o"),
      OBJ("tmr.o"),
      OBJ("vulkan.o"),
      OBJ("vulkan-droid.o"));
  return 0;
}
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

  SHADER("boav.frag", RES_PATH);
  SHADER("boav.vert", RES_PATH);

  return 0;
#else
  mkdir("droid/" ARCH, 0777);

  if (CC1("vulkan.c", "vulkan.o")) return 1;
  if (CC1("vulkan-droid.c", "vulkan-droid.o")) return 1;

  if (HDR1("gme.h", "gme.o", "GME_IMPLEMENTATION")) return 1;
  if (HDR1("sfx.h", "sfx.o", "SFX_IMPLEMENTATION")) return 1;
  if (HDR1("snd.h", "snd.o", "SND_IMPLEMENTATION")) return 1;
  if (HDR1("snk.h", "snk.o", "SNK_IMPLEMENTATION")) return 1;
  if (HDR1("tmr.h", "tmr.o", "TMR_IMPLEMENTATION")) return 1;

  if (link_exe()) return 1;

  return 0;
#endif
}

