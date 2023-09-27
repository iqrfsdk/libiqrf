/**
 * Copyright 2023 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdio.h>

typedef enum {
	IQRF_LOG_LEVEL_FATAL,
	IQRF_LOG_LEVEL_ERROR,
	IQRF_LOG_LEVEL_WARNING,
	IQRF_LOG_LEVEL_INFO,
	IQRF_LOG_LEVEL_DEBUG,
	IQRF_LOG_LEVEL_TRACE,
} iqrf_log_level_t;

typedef void (*iqrf_log_print_t)(iqrf_log_level_t level, const char *file, int line, const char *func, const char *str);
typedef void (*iqrf_log_printf_t)(iqrf_log_level_t level, const char *file, int line, const char *func, const char *fmt, ...);

/**
 * Sets the log level
 * @param level Minimum log level to print
 */
void iqrf_log_set_level(iqrf_log_level_t level);

/**
 * Retrieves the log level
 * @return Minimum log level to print
 */
iqrf_log_level_t iqrf_log_get_level();

/**
 * Sets the custom log print function
 * @param func Custom log print function
 */
void iqrf_log_set_print(iqrf_log_print_t func);

/**
 * Unsets the custom log print function
 */
void iqrf_log_unset_print();

/**
 * Sets the custom log printf function
 * @param func Custom log printf function
 */
void iqrf_log_set_printf(iqrf_log_printf_t func);

/**
 * Unsets the custom log printf function
 */
void iqrf_log_unset_printf();

/**
 * Default log print function
 * @param level Log level
 * @param file File path
 * @param line Line
 * @param func Function name
 * @param str Message
 */
void iqrf_log_print_default(iqrf_log_level_t level, const char *file, int line, const char *func, const char *str);

/**
 * Default log printf function
 * @param level Log level
 * @param file File path
 * @param line Line
 * @param func Function name
 * @param fmt Message format
 * @param ... Message arguments
 */
void iqrf_log_printf_default(iqrf_log_level_t level, const char *file, int line, const char *func, const char *fmt, ...);

/**
 * Minimal log level to print
 */
static iqrf_log_level_t iqrf_log_level = IQRF_LOG_LEVEL_INFO;

/**
 * Log print function
 */
static iqrf_log_print_t iqrf_log_print = &iqrf_log_print_default;

/**
 * Log printf function
 */
static iqrf_log_printf_t iqrf_log_printf = &iqrf_log_printf_default;

/**
 * Log level names
 */
static const char *iqrf_log_level_names[] = {
    "FATAL",
    "ERROR",
    "WARNING",
    "INFO",
    "DEBUG",
    "TRACE",
};

#ifdef NDEBUG

#ifndef IQRF_LOG_PRINT
#define IQRF_LOG_PRINT(level, fmt, ...)
#endif

#ifndef IQRF_LOG_PRINTF
#define IQRF_LOG_PRINTF(level, fmt, ...)
#endif

#else

#ifndef IQRF_LOG_PRINT
#define IQRF_LOG_PRINT(level, str) do { \
    if (iqrf_log_print == NULL) { \
        fprintf(stderr, "Undefined iqrf_log_print function implementation."); \
        break; \
    } \
    iqrf_log_print(level, __FILE__, __LINE__, __func__, str); \
} while (0)
#endif

#ifndef IQRF_LOG_PRINTF
#define IQRF_LOG_PRINTF(level, fmt, ...) do { \
    if (iqrf_log_printf == NULL) { \
        fprintf(stderr, "Undefined iqrf_log_printf function implementation."); \
        break; \
    } \
    iqrf_log_printf(level, __FILE__, __LINE__, __func__, fmt, __VA_ARGS__); \
} while (0)
#endif

#endif

#ifdef __cplusplus
}
#endif
