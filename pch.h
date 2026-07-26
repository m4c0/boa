#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#  define WIN32_MEAN_AND_LEAN
#  include <windows.h>
#  include <windowsx.h>
#else
#  include <sys/time.h>
#endif

#ifdef VLK_USE_VOLK
#  include "volk.h"
#else
#  include "Vulkan-Headers/include/vulkan/vulkan_core.h"
#  ifdef __APPLE__
#    include "Vulkan-Headers/include/vulkan/vulkan_metal.h"
#  elif _WIN32
#    include "Vulkan-Headers/include/vulkan/vulkan_win32.h"
#  endif
#endif
