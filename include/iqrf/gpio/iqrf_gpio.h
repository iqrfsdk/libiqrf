#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "iqrf/common/iqrf_sleep.h"
#include "iqrf/log/iqrf_log.h"

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
iqrf_gpio_error_t iqrf_gpio_init(const iqrf_gpio_t *gpio, iqrf_gpio_direction_t direction, bool initialValue);

/**
 * Destroys a GPIO pin
 * @param pin GPIO pin number
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_destroy(const iqrf_gpio_t *gpio);

/**
 * Initializes the GPIO pin as an input
 * @param gpio GPIO pin to initialize
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_init_input(const iqrf_gpio_t *gpio);

/**
 * Initializes the GPIO pin as an output
 * @param gpio GPIO pin to initialize
 * @param initialValue Output pin initial value
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_init_output(const iqrf_gpio_t *gpio, bool initialValue);

/**
 * Retrieves the direction for GPIO pin
 * @param pin GPIO pin
 * @param direction GPIO pin direction
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_get_direction(const iqrf_gpio_t *gpio, iqrf_gpio_direction_t *direction);

/**
 * Sets the direction for GPIO pin
 * @param pin GPIO pin
 * @param direction GPIO pin direction
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_set_direction(const iqrf_gpio_t *gpio, iqrf_gpio_direction_t direction);

/**
 * Retrieves the direction for GPIO pin
 * @param pin GPIO pin
 * @param value GPIO pin output value
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_get_value(const iqrf_gpio_t *gpio, bool *value);

/**
 * Sets the direction for GPIO pin
 * @param pin GPIO pin
 * @param value GPIO pin output value
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_set_value(const iqrf_gpio_t *gpio, bool value);

#ifdef __cplusplus
}
#endif
