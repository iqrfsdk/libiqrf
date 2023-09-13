#pragma once

#include <gpiod.h>

#include "iqrf/debug/iqrf_debug.h"
#include "iqrf/gpio/iqrf_gpio_common.h"

/**
 * Initializes the GPIO pin
 * @param gpio GPIO pin to initialize
 * @param direction GPIO pin direction
 * @param initialValue Output pin initial value
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_libgpiod_init(iqrf_gpio_t *gpio, iqrf_gpio_direction_t direction, bool initialValue);
