#include <sys/stat.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void usage() {
  fprintf(stderr, "just call 'build' without arguments\n");
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

static int shader(char * name) {
  char spv[1024];
  sprintf(spv, "boas.app/Contents/Resources/%s.spv", name);

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

static int link_exe() {
  char * args[] = {
    "clang", "-Wall",
    "-framework", "AppKit",
    "-framework", "AudioToolbox",
    "-framework", "MetalKit",
    "-o", "boas.app/Contents/MacOS/boas", 
    "gme.o", "sfx.o", "snd.o", "snk.o", "tmr.o",
    "vulkan.o", "vulkan-osx.o",
    0 };
  return run(args);
}

int main(int argc, char ** argv) {
  if (argc != 1) return (usage(), 1);

  mkdir("boas.app", 0777);
  mkdir("boas.app/Contents", 0777);
  mkdir("boas.app/Contents/MacOS", 0777);
  mkdir("boas.app/Contents/Resources", 0777);

  { char * args[] = { "cp", "libvulkan.dylib", "boas.app/Contents/MacOS/", 0 };
    if (run(args)) return 1; }

  if (cc("vulkan.c",     "vulkan.o"    )) return 1;
  if (cc("vulkan-osx.m", "vulkan-osx.o")) return 1;
  if (hdr("gme.h", "gme.o", "GME_IMPLEMENTATION")) return 1;
  if (hdr("sfx.h", "sfx.o", "SFX_IMPLEMENTATION")) return 1;
  if (hdr("snd.h", "snd.o", "SND_IMPLEMENTATION")) return 1;
  if (hdr("snk.h", "snk.o", "SNK_IMPLEMENTATION")) return 1;
  if (hdr("tmr.h", "tmr.o", "TMR_IMPLEMENTATION")) return 1;
  if (link_exe()) return 1;

  if (shader("boav.frag")) return 1;
  if (shader("boav.vert")) return 1;

  return 0;
}
