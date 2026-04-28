#include <sys/stat.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// You can get this path with 'xcrun --show-sdk-path --sdk iphoneos'
#define SDK_PATH "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS17.5.sdk"
#define TARGET "arm64-apple-ios17.0"

static void usage() {
  fprintf(stderr, "just call 'build' without arguments\n");
}

static char * slurp(const char * file) {
  FILE * f = fopen(file, "rb");
  assert(f);

  assert(0 == fseek(f, 0, SEEK_END));
  long sz = ftell(f);
  assert(sz);
  assert(0 == fseek(f, 0, SEEK_SET));

  char * data = malloc(sz + 1);
  assert(1 == fread(data, sz, 1, f));
  data[sz] = 0;

  fclose(f);
  return data;
}

static int run(char ** args) {
  assert(args && args[0]);

  pid_t pid = fork();
  if (pid == 0) {
    execvp(args[0], args);
    abort();
  } else if (pid > 0) {
    int sl = 0;
    assert(0 <= waitpid(pid, &sl, 0));
    if (WIFEXITED(sl)) return WEXITSTATUS(sl);
  }

  fprintf(stderr, "failed to run child process: %s\n", args[0]);
  return 1;
}

static int apply(char * src, char * tgt) {
  char * file = slurp(src);

  FILE * f = fopen(tgt, "wb");
  assert(f);

  char * p = file;
  while (*p) {
    p = strchr(file, '&');
    if (!p) break;

    assert(1 == fwrite(file, p-file, 1, f));
    file = ++p;

    char * pp = strchr(p, ';');
    if (!pp) {
      assert(0 == fputc('&', f));
      file++;
      continue;
    }
    *pp = 0;

    char * env = getenv(p);
    if (strncmp(p, "IOS_", 4)) {
      assert(fprintf(f, "&%s;", file));
      file = ++pp;
    } else if (env) {
      assert(fprintf(f, "%s", env));
      file = ++pp;
    } else {
      fprintf(stderr, "Missing environment: %s\n", p);
      exit(1);
    }
  }

  assert(fprintf(f, "%s", file));
  fclose(f);
  return 0;
}

static int codesign() {
  char * team = getenv("IOS_TEAM");
  assert(team && "Missing IOS_TEAM environment variable");

  char * args[] = {
    "codesign", "-s", strdup(team),
    "export.xcarchive/Products/Applications/boas.app",
    0 };
  return run(args);
}

static int export() {
  char * args[] = {
    "xcodebuild", "-exportArchive",
    "-archivePath", "export.xcarchive",
    "-exportPath", "export",
    "-exportOptionsPlist", "export.plist",
    0 };
  return run(args);
}

static int actool() {
  char * args[] = {
    "actool",
    "--notices", "--warnings", "--errors",
    "--output-format", "human-readable-text",
    "--app-icon", "AppIcon",
    "--accent-color", "AccentColor",
    "--compress-pngs",
    "--enable-on-demand-resources", "YES",
    "--target-device", "iphone",
    "--target-device", "ipad",
    "--platform", "iphoneos",
    //"--filter-for-thinning-device-configuration", "iPhone16,1"
    //"--filter-for-device-os-version", "17.0"
    "--development-region", "en",
    "--minimum-deployment-target", "17.5",
    "--output-partial-info-plist", "icon-partial.plist",
    "--compile", "export.xcarchive/Products/Applications/boas.app",
    "Assets.xcassets",
    0
  };
  return run(args);
}

static int install() {
  char * device = getenv("IOS_DEVICE");
  if (!device) {
    fprintf(stderr, "Missing IOS_DEVICE - skipping install\n");
    return 0;
  }

  char * args[] = {
    "xcrun", "devicectl", "device", "install", "app", "--device", device, "export/boas.ipa", 0
  };
  return run(args);
}

static int validate() {
  char * api_key = getenv("IOS_API_KEY");
  assert(api_key && "Missing IOS_API_KEY environment variable");
  char * api_issuer = getenv("IOS_API_ISSUER");
  assert(api_issuer && "Missing IOS_API_ISSUER environment variable");

  char * args[] = {
    "xcrun", "altool", "--validate-app", "-t", "iphoneos",
    "-f", "export/boas.ipa",
    "--apiKey", strdup(api_key),
    "--apiIssuer", strdup(api_issuer),
    0 };
  return run(args);
}

static int cc(char * src, char * o) {
  char * args[] = {
    "clang", "-Wall", "-O3", "-target", TARGET, "-isysroot", SDK_PATH,
    "-IVulkan-Headers/include",
    "-o", o, "-c", src, 0 };
  return run(args);
}

static int link_exe() {
  char * args[] = {
    "clang", "-Wall", "-O3", "-target", TARGET, "-isysroot", SDK_PATH,
    "-framework", "CoreFoundation",
    "-framework", "CoreGraphics",
    "-framework", "Foundation",
    "-framework", "IOSurface",
    "-framework", "Metal",
    "-framework", "MetalKit",
    "-framework", "QuartzCore",
    "-framework", "UIKit",
    "-o", "export.xcarchive/Products/Applications/boas.app/boas", 
    "game.o", "swapchain.o", "swapchain-ios.o",
    "libMoltenVK.a", "-lc++",
    0 };
  return run(args);
}

int main(int argc, char ** argv) {
  if (argc != 1) return (usage(), 1);

  mkdir("export.xcarchive", 0777);
  mkdir("export.xcarchive/Products", 0777);
  mkdir("export.xcarchive/Products/Applications", 0777);
  mkdir("export.xcarchive/Products/Applications/boas.app", 0777);

  if (cc("game.c",          "game.o"         )) return 1;
  if (cc("swapchain.c",     "swapchain.o"    )) return 1;
  if (cc("swapchain-ios.m", "swapchain-ios.o")) return 1;
  if (link_exe()) return 1;

  if (apply("export.plist.in",    "export.plist")) return 1;
  if (apply("xcarchive.plist.in", "export.xcarchive/Info.plist")) return 1;
  if (apply("app.plist.in",       "export.xcarchive/Products/Applications/boas.app/Info.plist")) return 1;

  if (actool())   return 1;
  if (codesign()) return 1;
  if (export())   return 1;
  if (install())  return 1;
  if (validate()) return 1;

  return 0;
}
