#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "vlk.h"

unsigned vlk_open(const char * name, const char * ext, const void ** ptr) {
  char fname[1024];
#ifdef _WIN32
  sprintf(fname, "app/%s.%s", name, ext);
#else
  sprintf(fname, "simon.app/Contents/Resources/%s.%s", name, ext);
#endif
  FILE * f = fopen(fname, "rb");

  assert(0 == fseek(f, 0, SEEK_END));
  long sz = ftell(f);
  assert(sz);
  assert(0 == fseek(f, 0, SEEK_SET));

  void * data = malloc(sz + 1);
  assert(1 == fread(data, sz, 1, f));
  *ptr = data;

  fclose(f);
  return sz;
}

void vlk_log(int r, const char * msg) {
  printf("Vulkan call failed (code=%d): %s\n", r, msg);
  exit(1);
}

#ifdef _WIN32
HWND vlk_hwnd;
#else
void * vlk_metal_layer;
#endif

void shot(unsigned w, unsigned h) {
  char fn[1024];
  snprintf(fn, 1024, "shot-%dx%d.png", w, h);

  vlk_init(0);
  void * buf = vlk_headless(w, h);
  stbi_write_png(fn, w, h, 4, buf, w * 4);
  vlk_deinit();
}

int main() {
  // icon
  shot(1024, 1024);

  // apple store
  shot(1260, 2736);
  shot(1284, 2778);
 
  // itch
  shot(630, 500);
}
