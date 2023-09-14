#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NDEBUG

#ifndef IQRF_LOG_PRINT
#define IQRF_LOG_PRINT(level, fmt, ...)
#endif

#ifndef IQRF_LOG_PRINTF
#define IQRF_LOG_PRINTF(level, fmt, ...)
#endif

#else

#include <stdio.h>

typedef enum {
	IQRF_LOG_LEVEL_FATAL,
	IQRF_LOG_LEVEL_ERROR,
	IQRF_LOG_LEVEL_WARNING,
	IQRF_LOG_LEVEL_INFO,
	IQRF_LOG_LEVEL_DEBUG,
	IQRF_LOG_LEVEL_TRACE,
} iqrf_log_level_t;

static const char *iqrf_log_level_names[] = {
	"FATAL",
	"ERROR",
	"WARNING",
	"INFO",
	"DEBUG",
	"TRACE",
};

static iqrf_log_level_t iqrf_log_level = IQRF_LOG_LEVEL_INFO;

#ifndef IQRF_LOG_PRINT
#define IQRF_LOG_PRINT(level, str) do { \
	if (level > iqrf_log_level) { \
		break; \
	} \
	fprintf(stderr, "[%s] %s:%d - %s(): " str "\n", iqrf_log_level_names[level], __FILE__, __LINE__, __func__); \
} while (0)
#endif

#ifndef IQRF_LOG_PRINTF
#define IQRF_LOG_PRINTF(level, fmt, ...) do { \
	if (level > iqrf_log_level) { \
		break; \
	} \
	fprintf(stderr, "[%s] %s:%d - %s(): " fmt "\n", iqrf_log_level_names[level], __FILE__, __LINE__, __func__, __VA_ARGS__); \
} while (0)
#endif

#endif

#ifdef __cplusplus
}
#endif
