#ifndef TMR_H
#define TMR_H

extern void (*tmr_fn)();

void tmr_init  (unsigned ms);
void tmr_deinit();

#ifdef TMR_IMPLEMENTATION

void (*tmr_fn)();

#ifdef __ANDROID__
#elif __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#elif _WIN32
#define WIN32_MEAN_AND_LEAN
#include <windows.h>
#endif

#ifdef __ANDROID__
#elif __APPLE__
static CFRunLoopTimerRef tmr_h;
static void tmr_callback(CFRunLoopTimerRef ref, void * ctx) { tmr_fn(); }
void tmr_init(unsigned ms) {
  CFAbsoluteTime secs = (float)ms / 1000.0f;
  CFAbsoluteTime when = CFAbsoluteTimeGetCurrent() + secs;

  tmr_h = CFRunLoopTimerCreate(NULL, when, secs, 0, 0, tmr_callback, NULL);
  CFRunLoopAddTimer(CFRunLoopGetMain(), tmr_h, kCFRunLoopCommonModes);
}
void tmr_deinit() {
  if (!tmr_h) return;
  CFRunLoopRemoveTimer(CFRunLoopGetMain(), tmr_h, kCFRunLoopCommonModes);
  CFRelease(tmr_h);
  tmr_h = NULL;
}
#elif _WIN32
static HANDLE tmr_h;
static void tmr_callback(void * ctx, BOOLEAN b) { tmr_fn(); }
void tmr_init(unsigned ms) {
  CreateTimerQueueTimer(&tmr_h, NULL, tmr_callback, NULL, ms, ms, 0);
}
void tmr_deinit() {
  if (tmr_h) DeleteTimerQueueTimer(NULL, tmr_h, NULL);
  tmr_h = NULL;
}
#endif
#endif
#endif
