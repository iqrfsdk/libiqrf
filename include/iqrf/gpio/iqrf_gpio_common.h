#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <gpiod.h>

typedef enum iqrf_gpio_drivers {
	IQRF_GPIO_DRIVER_SYSFS,
	IQRF_GPIO_DRIVER_LIBGPIOD,
} iqrf_gpio_driver_t;

typedef enum {
	IQRF_GPIO_DIRECTION_UNKNOWN = -1,
	IQRF_GPIO_DIRECTION_IN,
	IQRF_GPIO_DIRECTION_OUT,
} iqrf_gpio_direction_t;

typedef enum iqrf_gpio_errors {
	IQRF_GPIO_ERROR_OK,
	IQRF_GPIO_ERROR_NULL_POINTER,
	IQRF_GPIO_ERROR_INVALID_DRIVER,
	IQRF_GPIO_ERROR_INVALID_PIN,
	IQRF_GPIO_ERROR_OPEN_FAILED,
	IQRF_GPIO_ERROR_WRITE_FAILED,
	IQRF_GPIO_ERROR_NOT_IMPLEMENTED,
} iqrf_gpio_error_t;

typedef struct iqrf_gpio {
	iqrf_gpio_driver_t driver;
	union {
		struct {
			int64_t pin;
		} sysfs;
		struct {
			struct gpiod_chip *chip;
			struct gpiod_line *line;
			const char *consumer;
		} libgpiod;
	};
} iqrf_gpio_t;

#ifdef __cplusplus
}
#endif
