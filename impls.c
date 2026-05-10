#define SFX_IMPLEMENTATION
#include "sfx.h"
#define SND_IMPLEMENTATION
#include "snd.h"
#define SNK_IMPLEMENTATION
#include "snk.h"
#define TMR_IMPLEMENTATION
#include "tmr.h"

#ifdef __APPLE__
#pragma leco add_framework AudioToolbox
#elif _WIN32
#pragma leco add_library ole32
#endif

