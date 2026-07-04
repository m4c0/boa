#include <android/native_activity.h>
#include <android/log.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

ANativeWindow * vlk_nwnd;

void vlk_init();
void vlk_frame();

static AAssetManager * aam;
static AAsset        * ass;

static int destroyed;
static pthread_t pth;

unsigned vlk_open(const char * name, const char * ext, const void ** ptr) {
  if (ass) AAsset_close(ass);

  char fn[1024];
  snprintf(fn, 1024, "%s.%s", name, ext);
  ass = AAssetManager_open(aam, fn, AASSET_MODE_BUFFER);

  *ptr = AAsset_getBuffer(ass);
  return AAsset_getLength(ass);
}

void vlk_log(int r, const char * msg) {
  __android_log_print(ANDROID_LOG_ERROR, "m4c0", "vulkan error [%d]: %s\n", r, msg);
  abort();
}

static void * thread(void * ptr) {
  while (!destroyed) {
    vlk_frame();
  }
  return NULL;
}

static void on_start(ANativeActivity * activity) {
  __android_log_print(ANDROID_LOG_INFO, "m4c0", "starting\n");
}

static void on_destroy(ANativeActivity * activity) {
  destroyed = 1;
  if (ass) AAsset_close(ass);
}

static void on_native_window_created(ANativeActivity * act, ANativeWindow * wnd) {
  __android_log_print(ANDROID_LOG_INFO, "m4c0", "window created: %p\n", wnd);
  vlk_nwnd = wnd;
  vlk_init();

  pthread_attr_t attr; 
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create(&pth, &attr, thread, NULL);
}

void ANativeActivity_onCreate(ANativeActivity * activity, void * state, size_t state_sz) {
  __android_log_print(ANDROID_LOG_INFO, "m4c0", "creating activity: %p\n", activity);

  aam = activity->assetManager;

  activity->callbacks->onStart = on_start;
  activity->callbacks->onDestroy = on_destroy;
  activity->callbacks->onNativeWindowCreated = on_native_window_created;
}
