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
#  define RES_PATH "droid/apk/assets"
#endif
#include "build.h"

#include <sys/stat.h>

#ifdef ARCH
static int pch() {
  RUN("clang", "-Wall", "-g", "-x", "c-header", CFLAGS,
    "-IVulkan-Headers/include",
    "-D", "VK_USE_PLATFORM_ANDROID_KHR",
    "-D", "VLK_USE_VOLK",
    "-o", "pch.pch", "pch.h");
  return 0;
}

static int link_exe() {
  RUN(ANDROID_NDK_PREBUILT_ROOT"/bin/"ARCH"-clang", "-Wall", "-shared",
      "-Wl,-Bsymbolic", "-Wl,--no-undefined",
      "-o", "droid/apk/lib/" ARCHDIR "/libboas.so", 
      "-landroid", "-llog",
      OBJS, "vulkan-droid.o", "volk.o");
  RUN("cp", "droid/apk/lib/" ARCHDIR "/libboas.so", "droid/aab/lib/" ARCHDIR "/");
  return 0;
}

#else
static int link_exe() { return 1; } // removes warning
#endif

static int meta(char * dir, char * tgt) {
  fprintf(stderr, "Building for target %s\n", tgt);

  char * ndk = getenv("ANDROID_NDK_PREBUILT_ROOT");
  assert(ndk && "missing env var ANDROID_NDK_PREBUILT_ROOT");

  char o[1024];
  snprintf(o, 1024, "droid/build-%s", tgt);
  char a[1024];
  snprintf(a, 1024, "-DARCH=\"%s\"", tgt);
  char d[1024];
  snprintf(d, 1024, "-DARCHDIR=\"%s\"", dir);
  char n[1024];
  snprintf(n, 1024, "-DANDROID_NDK_PREBUILT_ROOT=\"%s\"", ndk);

  RUN("clang", n, a, d, "-o", o, "build-droid.c");
  RUN(o);
  return 0;
}

int main(int argc, char ** argv) {
#ifndef ARCH
  RUN("rm", "-rf", "droid");

  mkdir("droid", 0777);
  mkdir("droid/aab", 0777);
  mkdir("droid/aab/assets", 0777);
  mkdir("droid/aab/lib", 0777);
  mkdir("droid/aab/manifest", 0777);
  mkdir("droid/apk", 0777);
  mkdir("droid/apk/assets", 0777);
  mkdir("droid/apk/lib", 0777);

  if (meta("arm64-v8a",   "aarch64-linux-android32"   )) return 1;
  if (meta("armeabi-v7a", "armv7a-linux-androideabi32")) return 1;
  if (meta("x86",         "i686-linux-android32"      )) return 1;
  if (meta("x86_64",      "x86_64-linux-android32"    )) return 1;

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
  assert(fopen(jar, "rb") && "ANDROID_PLATFORM does not contain a valid android.jar");

  char * bundletools = getenv("ANDROID_BUILDBUNDLE");
  assert(bundletools && "missing env for ANDROID_BUILDBUNDLE");
  assert(fopen(bundletools, "rb") && "invalid ANDROID_BUILDBUNDLE");

  // APK

  RUN(aapt2, "compile", "res/values/strings.xml", "-o", "droid/");
  RUN(aapt2, "link", "droid/values_strings.arsc.flat", "-o", "droid/app.res.apk", "--manifest", "AndroidManifest.xml", "-I", jar);

  RUN("cp", "-r", "droid/apk/assets", "droid/aab/");

  RUN("jar", "--update", "--file", "droid/app.res.apk", "-C", "droid/apk", ".");

  RUN(zipalign, "-p", "-f", "-v", "4", "droid/app.res.apk", "droid/app.apk");

  // Just an example
  RUN("keytool", "-genkeypair", "-keystore", "droid/keystore.jks", "-alias", "androidkey", "-validity", "10000", "-keyalg", "RSA", "-keysize", "2048", "-storepass", "android", "-keypass", "android", "-dname", "CN=CA");
  RUN(apksigner, "sign", "--in", "droid/app.apk", "-ks", "droid/keystore.jks", "--ks-key-alias", "androidkey", "--ks-pass", "pass:android", "--key-pass", "pass:android");

  // AAB

  RUN(aapt2, "compile", "--dir", "res", "-o", "droid/res.zip");
  RUN(aapt2, "link", "--proto-format", "-o", "droid/linked.zip", "-I", jar, "--manifest", "AndroidManifest.xml", "droid/res.zip", "--auto-add-overlay");
  RUN("unzip", "-t", "droid/res.zip");
  RUN("unzip", "-t", "droid/linked.zip");

  RUN("jar", "-x", "-v", "-f", "droid/linked.zip", "-C", "droid/aab", ".");
  RUN("mv", "droid/aab/AndroidManifest.xml", "droid/aab/manifest/");
  RUN("jar", "-c", "-M", "-f", "droid/aab.zip",
      "-C", "droid/aab", "manifest",
      "-C", "droid/aab", "lib",
      "-C", "droid/aab", "resources.pb");

  RUN("java", "-jar", bundletools, "build-bundle", "--modules=droid/aab.zip", "--output=droid/app.aab");
  RUN("jarsigner", "-keystore", "droid/keystore.jks", "-storepass", "android", "droid/app.aab", "androidkey");

  return 0;
#else
  mkdir("droid/aab/lib/" ARCHDIR, 0777);
  mkdir("droid/apk/lib/" ARCHDIR, 0777);

  if (pch()) return 1;

  CC("volk");
  CC("vulkan-droid");
  if (compile_and_link_exe()) return 1;

  return 0;
#endif
}

