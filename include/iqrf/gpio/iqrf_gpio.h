#pragma once

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "iqrf/common/iqrf_sleep.h"
#include "iqrf/debug/iqrf_debug.h"

#include "iqrf/gpio/iqrf_gpio_common.h"
#include "iqrf/gpio/iqrf_gpio_libgpiod.h"
#include "iqrf/gpio/iqrf_gpio_sysfs.h"

/**
 * Initializes the GPIO pin
 * @param gpio GPIO pin to initialize
 * @param direction GPIO pin direction
 * @param initialValue Output pin initial value
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_init(iqrf_gpio_t *gpio, iqrf_gpio_direction_t direction, bool initialValue);

/**
 * Initializes the GPIO pin as an input
 * @param gpio GPIO pin to initialize
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_init_input(iqrf_gpio_t *gpio);

/**
 * Initializes the GPIO pin as an output
 * @param gpio GPIO pin to initialize
 * @param initialValue Output pin initial value
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_init_output(iqrf_gpio_t *gpio, bool initialValue);

/**
 * Returns GPIO pin direction
 * @param gpio GPIO pin
 * @param[out] direction GPIO pin direction
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_get_direction(iqrf_gpio_t *gpio, iqrf_gpio_direction_t *direction);

/**
 * Sets GPIO pin direction
 * @param gpio GPIO pin
 * @param[in] direction GPIO pin direction to set
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_set_direction(iqrf_gpio_t *gpio, iqrf_gpio_direction_t direction);

