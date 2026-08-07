#define APP "boas"

// You can get this path with 'xcrun --show-sdk-path --sdk iphoneos'
#define SDK_PATH "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk"
#define TARGET "arm64-apple-ios26.0"

#define RES_PATH "."
#define CFLAGS "-g", "-O3", "-target", TARGET, "-isysroot", SDK_PATH, "-IVulkan-Headers/include"
#include "build.h"

#include <sys/stat.h>
#include <string.h>
#include <time.h>

#define UPLOAD 0

#define CROSS(X) RUN("spirv-cross", "shader."X".spv", "--msl", "--output", "export.xcarchive/Products/Applications/"APP".app/shader."X".metal", "--flip-vert-y", "--msl-ios")

static time_t bundle_version;
static int uploading;

static void print_key(FILE * f, const char * p) {
  char * env = getenv(p);
  if (strncmp(p, "IOS_", 4)) {
    assert(fprintf(f, "&%s;", p));
  } else if (0 == strcmp(p, "IOS_APP_NAME")) {
    assert(fprintf(f, APP));
  } else if (0 == strcmp(p, "IOS_BUNDLE_VERSION")) {
    assert(fprintf(f, "%ld", bundle_version));
  } else if (0 == strcmp(p, "IOS_METHOD")) {
    if (uploading) {
      assert(fprintf(f, "app-store-connect"));
    } else {
      assert(fprintf(f, "debugging"));
    }
  } else if (env) {
    assert(fprintf(f, "%s", env));
  } else {
    fprintf(stderr, "Missing environment: %s\n", p);
    exit(1);
  }
}

static int pch() {
  RUN("clang", "-Wall", "-O3", "-x", "c-header",
    "-target", TARGET, "-isysroot", SDK_PATH,
    "-IVulkan-Headers/include",
    "-D", "VK_USE_PLATFORM_METAL_EXT",
    "-o", "pch.pch", "pch.h");
  return 0;
}

static int codesign() {
  char * team = getenv("IOS_TEAM");
  assert(team && "Missing IOS_TEAM environment variable");

  RUN("codesign", "-f", "-s", strdup(team), "export.xcarchive/Products/Applications/boas.app");
  return 0;
}
 
static int symbols() {
  RUN("dsymutil", "export.xcarchive/Products/Applications/boas.app/"APP, "-o", "export.xcarchive/dSYMS/"APP".app.dSYM");
  return 0;
}

static int export() {
  RUN("xcodebuild", "-exportArchive",
      "-archivePath", "export.xcarchive",
      "-exportPath", "export",
      "-exportOptionsPlist", "export.plist");
  return 0;
}

static int actool() {
  RUN("actool",
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
      "--minimum-deployment-target", "26",
      "--output-partial-info-plist", "icon-partial.plist",
      "--compile", "export.xcarchive/Products/Applications/boas.app",
      "Assets.xcassets");
  return 0;
}

static int install() {
  char * device = getenv("IOS_DEVICE");
  if (!device) {
    fprintf(stderr, "Missing IOS_DEVICE - skipping install\n");
    return 0;
  }

  RUN("xcrun", "devicectl", "device", "install", "app", "--device", device, "export/"APP".ipa");
  return 0;
}

static int validate(char * verb) {
  char * api_key = getenv("IOS_API_KEY");
  assert(api_key && "Missing IOS_API_KEY environment variable");
  char * api_issuer = getenv("IOS_API_ISSUER");
  assert(api_issuer && "Missing IOS_API_ISSUER environment variable");

  RUN("xcrun", "altool", verb, "-t", "iphoneos",
      "-f", "export/"APP".ipa",
      "--apiKey", strdup(api_key),
      "--apiIssuer", strdup(api_issuer));
  return 0;
}

static int link_exe() {
  RUN("clang", "-Wall", "-O3", "-target", TARGET, "-isysroot", SDK_PATH,
      "-framework", "AudioToolbox",
      "-framework", "CoreFoundation",
      "-framework", "CoreGraphics",
      "-framework", "Foundation",
      "-framework", "IOSurface",
      "-framework", "Metal",
      "-framework", "MetalKit",
      "-framework", "QuartzCore",
      "-framework", "UIKit",
      "-o", "export.xcarchive/Products/Applications/boas.app/"APP, 
      OBJS, "app-ios.o");
  return 0;
}

int main(int argc, char ** argv) {
  bundle_version = time(NULL);
  uploading = getenv("IOS_UPLOAD") != NULL;

  mkdir("export.xcarchive", 0777);
  mkdir("export.xcarchive/Products", 0777);
  mkdir("export.xcarchive/Products/Applications", 0777);
  mkdir("export.xcarchive/Products/Applications/boas.app", 0777);

  if (pch()) return 1;

  CM("app-ios");
  if (compile_and_link_exe()) return 1;
  if (shaders()) return 1;

  CROSS("frag");
  CROSS("vert");

  if (apply("export.plist.in",    "export.plist")) return 1;
  if (apply("xcarchive.plist.in", "export.xcarchive/Info.plist")) return 1;
  if (apply("app.plist.in",       "export.xcarchive/Products/Applications/"APP".app/Info.plist")) return 1;

  if (getenv("IOS_BUILD_ONLY")) return 0;

  if (actool())   return 1;
  if (codesign()) return 1;
  if (symbols())  return 1;
  if (export())   return 1;

  if (uploading) {
    if (validate("--upload-app")) return 1;
  } else {
    if (install()) return 1;
    if (validate("--validate-app")) return 1;
  }

  return 0;
}
