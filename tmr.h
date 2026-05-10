#pragma once

typedef void(*tmr_fn_t)();

void tmr_init  (tmr_fn_t fn);
void tmr_deinit();

#ifdef TMR_IMPLEMENTATION

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#elif _WIN32
#define WIN32_MEAN_AND_LEAN
#include <windows.h>
#endif

#ifdef __APPLE__
static CFRunLoopTimerRef tmr_h;
static void tmr_callback(CFRunLoopTimerRef ref, void * fn) {
  ((tmr_fn_t)fn)();
}
void tmr_init(tmr_fn_t fn) {
  CFRunLoopTimerContext ctx = { .info = (void *)fn };

  CFAbsoluteTime secs = 25.0f / 1000.0f;
  CFAbsoluteTime when = CFAbsoluteTimeGetCurrent() + secs;

  tmr_h = CFRunLoopTimerCreate(NULL, when, secs, 0, 0, tmr_callback, &ctx);
  CFRunLoopAddTimer(CFRunLoopGetMain(), tmr_h, kCFRunLoopCommonModes);
}
void tmr_deinit() {
  CFRelease(tmr_h);
}
#elif _WIN32
static HANDLE tmr_h;
static void tmr_callback(void * fn, BOOLEAN b) {
  ((tmr_fn_t)fn)();
}
void tmr_init(tmr_fn_t fn) {
  tmr_h = CreateTimerQueue();

  HANDLE t;
  CreateTimerQueueTimer(&t, tmr_h, tmr_callback, (void *)fn, 25, 25, 0);
}
void tmr_deinit() {
  DeleteTimerQueueEx(tmr_h, NULL);
}
#endif
#endif
