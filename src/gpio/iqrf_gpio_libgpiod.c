#include "iqrf/gpio/iqrf_gpio_libgpiod.h"

/**
 * Checks the GPIO pin sanity
 * @param gpio GPIO pin
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_libgpiod_sanity_check(iqrf_gpio_t *gpio) {
    if (gpio == NULL) {
        IQRF_DEBUG_PRINT("GPIO is NULL");
        return IQRF_GPIO_ERROR_NULL_POINTER;
    }
    if (gpio->driver != IQRF_GPIO_DRIVER_LIBGPIOD) {
        IQRF_DEBUG_PRINTF("Invalid GPIO driver: %d", gpio->driver);
        return IQRF_GPIO_ERROR_INVALID_DRIVER;
    }
    return IQRF_GPIO_ERROR_OK;
}

iqrf_gpio_error_t iqrf_gpio_libgpiod_init(iqrf_gpio_t *gpio, iqrf_gpio_direction_t direction, bool initialValue) {
#ifdef WIN32
    return IQRF_GPIO_ERROR_OK;
#else
    iqrf_gpio_error_t error = iqrf_gpio_libgpiod_sanity_check(gpio);
    if (error != IQRF_GPIO_ERROR_OK) {
        return error;
    }
    if (direction == IQRF_GPIO_DIRECTION_IN) {
        gpiod_line_request_input(gpio->libgpiod.line, gpio->libgpiod.consumer);
    } else {
        gpiod_line_request_output(gpio->libgpiod.line, gpio->libgpiod.consumer, initialValue);
    }
    return IQRF_GPIO_ERROR_OK;
#endif
}