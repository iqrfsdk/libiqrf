/**
 * Copyright 2023-2025 MICRORISC s.r.o.
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

#include "iqrf/log/iqrf_log.h"

void iqrf_log_set_level(iqrf_log_level_t level) {
	iqrf_log_level = level;
}

iqrf_log_level_t iqrf_log_get_level() {
	return iqrf_log_level;
}

void iqrf_log_set_print(iqrf_log_print_t func) {
	iqrf_log_print = func;
}

void iqrf_log_unset_print() {
	iqrf_log_print = &iqrf_log_print_default;
}

void iqrf_log_set_printf(iqrf_log_printf_t func) {
	iqrf_log_printf = func;
}

void iqrf_log_unset_printf() {
	iqrf_log_printf = &iqrf_log_printf_default;
}

void iqrf_log_print_default(iqrf_log_level_t level, const char *file, int line, const char *func, const char *str) {
	if (level > iqrf_log_level) {
		return;
	}
	fprintf(stderr, "[%s] %s:%d - %s(): %s\n", iqrf_log_level_names[level], file, line, func, str);
}

void iqrf_log_printf_default(iqrf_log_level_t level, const char *file, int line, const char *func, const char *fmt, ...) {
	if (level > iqrf_log_level) {
		return;
	}
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "[%s] %s:%d - %s(): ", iqrf_log_level_names[level], file, line, func);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
}
