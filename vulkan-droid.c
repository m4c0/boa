#include <android/native_activity.h>
#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>

void vlk_init();

struct ANativeWindow * vlk_nwnd;

FILE * vlk_open(const char * name) {
  return NULL;
}

void vlk_log(int r, const char * msg) {
  abort();
}

static void on_start(ANativeActivity * activity) {
  __android_log_print(ANDROID_LOG_INFO, "m4c0", "starting\n");
}

static void on_native_window_created(ANativeActivity * act, ANativeWindow * wnd) {
  __android_log_print(ANDROID_LOG_INFO, "m4c0", "window created: %p\n", wnd);
  vlk_nwnd = wnd;
  vlk_init();
}

void ANativeActivity_onCreate(ANativeActivity * activity, void * state, size_t state_sz) {
  __android_log_print(ANDROID_LOG_INFO, "m4c0", "creating activity: %p\n", activity);
  activity->callbacks->onStart = on_start;
  activity->callbacks->onNativeWindowCreated = on_native_window_created;
}
