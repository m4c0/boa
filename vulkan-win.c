#define _CRT_SECURE_NO_WARNINGS
#define WIN32_MEAN_AND_LEAN
#include <stdio.h>
#include <windows.h>

static HWND g_hwnd;

void vlk_init();
void vlk_frame();
void vlk_deinit();

FILE * vlk_open(const char * name) {
  return fopen(name, "rb");
}

static char vlk_log_buf[1024];
void vlk_log(int r, const char * msg) {
  snprintf(vlk_log_buf, 1024, "Vulkan call failed (code=%d): %s\n", r, msg);
  MessageBox(NULL, vlk_log_buf, "Vulkan error", 0);

  // This is the only way to properly programatically exit an app from any
  // thread. Other attempts froze the app or kept it as a "background app".
  // i.e. We use WM_CLOSE instead of WM_QUIT (or PostQuitMessage etc) and we
  // need to use SendNotifyMessage instead of SendMessage.
  if (g_hwnd) SendNotifyMessage(g_hwnd, WM_CLOSE, 0, 0);
}

static LRESULT window_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
  switch (msg) {
    case WM_CREATE:
      vlk_init();
      return 0;
    case WM_CLOSE:
      // Required to enable another thread sending "plz exit" messages
      DestroyWindow(hwnd);
      return 0;
    case WM_DESTROY:
      vlk_deinit();
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProc(hwnd, msg, w_param, l_param);
}

int WinMain(HINSTANCE h_instance, HINSTANCE h_prev, LPSTR cmd_line, int cmd_show) {
  HICON h_icon = LoadIcon(h_instance, "IDI_ICON");

  WNDCLASSEX wcex  = {
    .cbSize        = sizeof(WNDCLASSEX),
    .style         = CS_HREDRAW | CS_VREDRAW,
    .lpfnWndProc   = &window_proc,
    .hInstance     = h_instance,
    .hIcon         = h_icon,
    .hCursor       = LoadCursor(NULL, IDC_ARROW),
    .hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
    .lpszClassName = "m4c0-snake-window",
    .hIconSm       = h_icon,
  };
  if (!RegisterClassEx(&wcex)) {
    MessageBox(NULL, "Failed to register window class", "Unhandled error", 0);
    return 1;
  }

  g_hwnd = CreateWindow(
      "m4c0-snake-window",
      "Casually Casual Snake Game",
      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
      600, 800, 
      NULL, NULL, h_instance, NULL);
  if (!g_hwnd) {
    MessageBox(NULL, "Failed to create window", "Unhandled error", 0);
    return 1;
  }

  ShowWindow(g_hwnd, cmd_show);
  UpdateWindow(g_hwnd);

  MSG msg;
  while (GetMessage(&msg, 0, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return msg.wParam;
}
