#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WIN32
#include <unistd.h>
#define IQRF_SLEEP(ms) usleep(1000*ms)
#else
#include "Windows.h"
#define IQRF_SLEEP(ms) Sleep(ms)
#endif

#ifdef __cplusplus
}
#endif
