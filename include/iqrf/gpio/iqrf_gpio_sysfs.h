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

#ifndef WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

#include "iqrf/common/iqrf_sleep.h"
#include "iqrf/gpio/iqrf_gpio_common.h"
#include "iqrf/log/iqrf_log.h"

#define IQRF_GPIO_SYSFS_BASE_PATH "/sys/class/gpio/"
#define IQRF_GPIO_SYSFS_BUFFER_SIZE 64
#define IQRF_GPIO_SYSFS_DIRECTION_BUFFER_SIZE 4
#define IQRF_GPIO_SYSFS_PIN_BUFFER_SIZE 20
#define IQRF_GPIO_SYSFS_VALUE_BUFFER_SIZE 2

typedef enum iqrf_gpio_sysfs_actions {
	IQRF_GPIO_SYSFS_ACTION_DIRECTION,
	IQRF_GPIO_SYSFS_ACTION_VALUE,
} iqrf_gpio_sysfs_action_t;

static const int64_t IQRF_GPIO_SYSFS_PIN_UNKNOWN = -1;

/**
 * Exports the GPIO pin
 * @param pin GPIO pin to export
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_sysfs_export(const iqrf_gpio_t *gpio);

/**
 * Unexports the GPIO pin
 * @param pin GPIO pin to unexport
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_sysfs_unexport(const iqrf_gpio_t *gpio);

/**
 * Checks if the GPIO pin is exported
 * @param gpio GPIO pin to check
 * @param isExported Is GPIO pin exported
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_sysfs_is_exported(const iqrf_gpio_t *gpio, bool *isExported);

/**
 * Retrieves the direction for GPIO pin
 * @param pin GPIO pin
 * @param direction GPIO pin direction
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_sysfs_get_direction(const iqrf_gpio_t *gpio, iqrf_gpio_direction_t *direction);

/**
 * Sets the direction for GPIO pin
 * @param pin GPIO pin
 * @param direction GPIO pin direction
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_sysfs_set_direction(const iqrf_gpio_t *gpio, iqrf_gpio_direction_t direction);

/**
 * Retrieves the direction for GPIO pin
 * @param pin GPIO pin
 * @param value GPIO pin output value
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_sysfs_get_value(const iqrf_gpio_t *gpio, bool *value);

/**
 * Sets the direction for GPIO pin
 * @param pin GPIO pin
 * @param value GPIO pin output value
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_sysfs_set_value(const iqrf_gpio_t *gpio, bool value);

/**
 * Initializes a GPIO pin
 * @param pin GPIO pin number
 * @param direction GPIO pin direction
 * @param initialValue Initial output value
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_sysfs_init(const iqrf_gpio_t *gpio, iqrf_gpio_direction_t direction, bool initialValue);

/**
 * Destroys a GPIO pin
 * @param pin GPIO pin number
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_sysfs_destroy(const iqrf_gpio_t *gpio);

#ifdef __cplusplus
}
#endif
