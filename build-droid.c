#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

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

static int shader(char * name) {
  char spv[1024];
  sprintf(spv, "app/%s.spv", name);

  char * args[] = { "glslang", "-V", name, "-o", spv, 0 };
  return run(args);
}

static int cc(char * src, char * o) {
  char * args[] = {
    "clang", "-Wall", "-g",
    "-IVulkan-Headers/include",
    "-o", o, "-c", src, 0 };
  return run(args);
}

static int hdr(char * src, char * o, char * d) {
  char * args[] = {
    "clang", "-Wall", "-x", "c", "-g", "-D", d, "-o", o, "-c", src, 0
  };
  return run(args);
}
#define HDR(src, o, d) hdr(src, "droid/" ARCH "/" o, d)

static int link_exe() {
  char * args[] = {
    "clang", "-Wall",
    "-o", "boas.app/Contents/MacOS/boas", 
    "gme.o", "sfx.o", "snd.o", "snk.o", "tmr.o",
    "vulkan.o", "vulkan-osx.o",
    0 };
  return run(args);
}

static int meta(char * tgt) {
  char o[1024];
  snprintf(o, 1024, "droid/build-%s", tgt);
  char d[1024];
  snprintf(d, 1024, "-DARCH=\"%s\"", tgt);

  { char * args[] = { "clang", d, "-o", o, "build-droid.c", 0 };
    if (run(args)) return 1; }

  char * args[] = { o, 0 };
  return run(args);
}

int main(int argc, char ** argv) {
#ifndef ARCH
  mkdir("droid", 0777);
  if (meta("aarch64-none-linux-android26"  )) return 1;
  if (meta("armv7-none-linux-androideabi26")) return 1;
  if (meta("i686-none-linux-android26"     )) return 1;
  if (meta("x86_64-none-linux-android26"   )) return 1;

  return 0;
#else
  mkdir("droid/" ARCH, 0777);

  if (HDR("gme.h", "gme.o", "GME_IMPLEMENTATION")) return 1;
  if (HDR("sfx.h", "sfx.o", "SFX_IMPLEMENTATION")) return 1;
  if (HDR("snd.h", "snd.o", "SND_IMPLEMENTATION")) return 1;
  if (HDR("snk.h", "snk.o", "SNK_IMPLEMENTATION")) return 1;
  if (HDR("tmr.h", "tmr.o", "TMR_IMPLEMENTATION")) return 1;

  return 0;
#endif
}

