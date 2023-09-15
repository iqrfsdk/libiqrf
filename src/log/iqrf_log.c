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
