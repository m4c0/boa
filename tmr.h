#pragma once

extern void (*tmr_fn)();

void tmr_init  ();
void tmr_deinit();

#ifdef TMR_IMPLEMENTATION

void (*tmr_fn)();

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#elif _WIN32
#define WIN32_MEAN_AND_LEAN
#include <windows.h>
#endif

#ifdef __APPLE__
static CFRunLoopTimerRef tmr_h;
static void tmr_callback(CFRunLoopTimerRef ref, void * ctx) { tmr_fn(); }
void tmr_init() {
  CFAbsoluteTime secs = 25.0f / 1000.0f;
  CFAbsoluteTime when = CFAbsoluteTimeGetCurrent() + secs;

  tmr_h = CFRunLoopTimerCreate(NULL, when, secs, 0, 0, tmr_callback, NULL);
  CFRunLoopAddTimer(CFRunLoopGetMain(), tmr_h, kCFRunLoopCommonModes);
}
void tmr_deinit() {
  CFRelease(tmr_h);
}
#elif _WIN32
static HANDLE tmr_h;
static void tmr_callback(void * ctx, BOOLEAN b) { tmr_fn(); }
void tmr_init(tmr_fn_t fn) {
  CreateTimerQueueTimer(&tmr_h, NULL, tmr_callback, NULL, 25, 25, 0);
}
void tmr_deinit() {
  DeleteTimerQueueTimer(NULL, tmr_h, NULL);
}
#endif
#endif
