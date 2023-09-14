#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <gpiod.h>

#include "iqrf/gpio/iqrf_gpio_common.h"
#include "iqrf/log/iqrf_log.h"

/**
 * Initializes the GPIO pin
 * @param gpio GPIO pin to initialize
 * @param direction GPIO pin direction
 * @param initialValue Output pin initial value
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_libgpiod_init(const iqrf_gpio_t *gpio, iqrf_gpio_direction_t direction, bool initialValue);

#ifdef __cplusplus
}
#endif
