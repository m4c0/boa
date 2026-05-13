#define GME_IMPLEMENTATION
#define SFX_IMPLEMENTATION
#define SND_IMPLEMENTATION
#define SNK_IMPLEMENTATION
#define TMR_IMPLEMENTATION

#include "gme.h"
#include "sfx.h"
#include "snd.h"
#include "snk.h"
#include "tmr.h"

#ifdef __APPLE__
#pragma leco add_framework AudioToolbox
#elif _WIN32
#pragma leco add_library ole32
#endif

