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
#  define RES_PATH "droid/apk"
#endif
#include "build.h"

#include <sys/stat.h>

#ifdef ARCH
static int link_exe() {
  RUN("clang", "-Wall", "-shared",
      "-Wl,-Bsymbolic", "-fuse-ld=lld", "-Wl,--no-undefined",
      "-resource-dir", ANDROID_NDK_PREBUILT_ROOT "/lib/clang/21",
      "--target=" ARCH,
      "--sysroot", ANDROID_NDK_PREBUILT_ROOT "/sysroot/",
      "-o", "droid/apk/" ARCH "/libboas.so", 
      OBJS, "vulkan-droid.o");
  RUN("cp", "droid/apk/" ARCH "/libboas.so", "droid/aab/lib/" ARCH "/");
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
  mkdir("droid/aab", 0777);
  mkdir("droid/aab/lib", 0777);
  mkdir("droid/aab/manifest", 0777);
  mkdir("droid/apk", 0777);

  if (meta("aarch64-none-linux-android26"  )) return 1;
  if (meta("armv7-none-linux-androideabi26")) return 1;
  if (meta("i686-none-linux-android26"     )) return 1;
  if (meta("x86_64-none-linux-android26"   )) return 1;

  if (shaders()) return 1;

  char * dir = getenv("ANDROID_BUILD_TOOLS");
  assert(dir && "missing env for ANDROID_BUILD_TOOLS");
  char aapt2[1024];
  snprintf(aapt2, 1024, "%s/aapt2", dir);
  char apksigner[1024];
  snprintf(apksigner, 1024, "%s/apksigner", dir);
  char zipalign[1024];
  snprintf(zipalign, 1024, "%s/zipalign", dir);

  dir = getenv("ANDROID_PLATFORM");
  assert(dir && "missing env for ANDROID_PLATFORM");
  char jar[1024];
  snprintf(jar, 1024, "%s/android.jar", dir);

  char * bundletools = getenv("ANDROID_BUILDBUNDLE");
  assert(bundletools && "missing env for ANDROID_BUILDBUNDLE");

  // APK

  RUN(aapt2, "compile", "res/values/strings.xml", "-o", "droid/");
  RUN(aapt2, "link", "droid/values_strings.arsc.flat", "-o", "droid/app.res.apk", "--manifest", "AndroidManifest.xml", "-I", jar);

  RUN("jar", "--update", "--file", "droid/app.res.apk", "-C", "droid/apk", ".");

  RUN(zipalign, "-p", "-f", "-v", "4", "droid/app.res.apk", "droid/app.apk");

  // Just an example
  RUN("keytool", "-genkeypair", "-keystore", "droid/keystore.jks", "-alias", "androidkey", "-validity", "10000", "-keyalg", "RSA", "-keysize", "2048", "-storepass", "android", "-keypass", "android", "-dname", "CN=CA");
  RUN(apksigner, "sign", "--in", "droid/app.apk", "-ks", "droid/keystore.jks", "--ks-key-alias", "androidkey", "--ks-pass", "pass:android", "--key-pass", "pass:android");

  // AAB

  RUN(aapt2, "compile", "--dir", "res", "-o", "droid/res.zip");
  RUN(aapt2, "link", "--proto-format", "-o", "droid/linked.zip", "-I", jar, "--manifest", "AndroidManifest.xml", "droid/res.zip", "--auto-add-overlay");

  RUN("jar", "xf", "droid/linked.zip", "-C", "droid/aab");
  RUN("mv", "droid/aab/AndroidManifest.xml", "droid/aab/manifest/");
  RUN("jar", "cMf", "droid/aab.zip",
      "-C", "droid/aab", "manifest",
      "-C", "droid/aab", "lib",
      "-C", "droid/aab", "resources.pb");

  RUN("java", "-jar", bundletools, "build-bundle", "--modules=droid/aab.zip", "--output=droid/app.aab");
  RUN("jarsigner", "-keystore", "droid/keystore.jks", "-storepass", "android", "droid/app.aab", "android");

  return 0;
#else
  mkdir("droid/aab/lib/" ARCH, 0777);
  mkdir("droid/apk/" ARCH, 0777);

  CC("vulkan-droid.c", "vulkan-droid.o", CFLAGS);
  if (compile_and_link_exe()) return 1;

  return 0;
#endif
}

