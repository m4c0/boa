#ifndef BUILD_H
#define BUILD_H

#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#  include <unistd.h>
#elif _WIN32
#  define _CRT_SECURE_NO_WARNINGS
#  define _CRT_NONSTDC_NO_WARNINGS
#  include <process.h>
#endif

#include <assert.h>

int run(char ** args) {
  assert(args && args[0]);

#ifdef __APPLE__
  pid_t pid = fork();
  if (pid == 0) {
    execvp(args[0], args);
    abort();
  } else if (pid > 0) {
    int sl = 0;
    assert(0 <= waitpid(pid, &sl, 0));
    if (WIFEXITED(sl)) return WEXITSTATUS(sl);
  }
#elif _WIN32
  if (0 == _spawnvp(_P_WAIT, args[0], (const char * const *)args)) {
    return 0;
  }
#endif

  fprintf(stderr, "failed to run child process: %s\n", args[0]);
  return 1;
}
#define RUN(...) do { char * args[] = { __VA_ARGS__, 0 }; if (run(args)) return 1; } while (0)

#define CC(src, o, ...) RUN("clang", "-Wall", __VA_ARGS__, "-o", o, "-c", src)
#define HDR(src, o, ...) CC(src, o, "-x", "c", __VA_ARGS__)

#define SHADER(src, fld) RUN("glslang", "-V", src, "-o", fld "/" src ".spv")

#define OBJS "gme.o", "sfx.o", "snd.o", "snk.o", "tmr.o", "vulkan.o"
static int compile_common() {
  CC("vulkan.c", "vulkan.o", CFLAGS);
  HDR("gme.h", "gme.o", CFLAGS, "-D", "GME_IMPLEMENTATION");
  HDR("sfx.h", "sfx.o", CFLAGS, "-D", "SFX_IMPLEMENTATION");
  HDR("snd.h", "snd.o", CFLAGS, "-D", "SND_IMPLEMENTATION");
  HDR("snk.h", "snk.o", CFLAGS, "-D", "SNK_IMPLEMENTATION");
  HDR("tmr.h", "tmr.o", CFLAGS, "-D", "TMR_IMPLEMENTATION");
  return 0;
}

static int shaders() {
  SHADER("boav.frag", RES_PATH);
  SHADER("boav.vert", RES_PATH);
  return 0;
}

#endif
