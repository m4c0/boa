#ifndef BUILD_H
#define BUILD_H

#ifdef __APPLE__
#  include <stdio.h>
#  include <stdlib.h>
#  include <unistd.h>
#endif

#include <assert.h>

static int run(char ** args) {
#ifdef __APPLE__
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
#endif
}
#define RUN(...) do { char * args[] = { __VA_ARGS__, 0 }; if (run(args)) return 1; } while (0)

#define CC(src, o, ...) RUN("clang", "-Wall", "-g", __VA_ARGS__, "-o", o, "-c", src);
#define HDR(src, o, d) RUN("clang", "-Wall", "-x", "c", "-g", "-D", d, "-o", o, "-c", src);

#endif
