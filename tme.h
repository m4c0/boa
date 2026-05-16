#ifndef TME_H
#define TME_H
#include <assert.h>

#ifdef _WIN32
#define WIN32_MEAN_AND_LEAN
#include <windows.h>
#else
#include <sys/time.h>
#endif

static void tme_gettime(struct timeval * tv) {
#ifdef _WIN32
  SYSTEMTIME st; GetSystemTime(&st);

  // Contains a 64-bit value representing the number of 100-nanosecond
  // intervals since January 1, 1601 (UTC).
  FILETIME ft; assert(SystemTimeToFileTime(&st, &ft));

  ULARGE_INTEGER i;
  i.u.LowPart  = ft.dwLowDateTime;
  i.u.HighPart = ft.dwHighDateTime;

  ULONGLONG usec = i.QuadPart / 10;
  tv->tv_sec  = usec / (1000*1000);
  tv->tv_usec = usec % (1000*1000);
#else
  gettimeofday(tv, NULL);
#endif
}

#endif
