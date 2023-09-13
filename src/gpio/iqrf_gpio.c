#include "iqrf/gpio/iqrf_gpio.h"

/**
 * Checks if the GPIO pin structure is valid
 * @param gpio GPIO pin
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_sanity_check(const iqrf_gpio_t *gpio) {
    if (gpio == NULL) {
        IQRF_DEBUG_PRINT("GPIO is NULL");
        return IQRF_GPIO_ERROR_NULL_POINTER;
    }
    if (gpio->driver != IQRF_GPIO_DRIVER_LIBGPIOD && gpio->driver != IQRF_GPIO_DRIVER_SYSFS) {
        IQRF_DEBUG_PRINTF("Invalid GPIO driver: %d", gpio->driver);
        return IQRF_GPIO_ERROR_INVALID_DRIVER;
    }
    return IQRF_GPIO_ERROR_OK;
}

iqrf_gpio_error_t iqrf_gpio_init(iqrf_gpio_t *gpio, iqrf_gpio_direction_t direction, bool initialValue) {
#ifdef WIN32
    return IQRF_GPIO_ERROR_OK;
#else
    iqrf_gpio_error_t error = iqrf_gpio_sanity_check(gpio);
    if (error != IQRF_GPIO_ERROR_OK) {
        return error;
    }
    if (gpio->driver == IQRF_GPIO_DRIVER_SYSFS) {
        return iqrf_gpio_sysfs_init(gpio, direction, initialValue);
    } else if (gpio->driver == IQRF_GPIO_DRIVER_LIBGPIOD) {
        return iqrf_gpio_libgpiod_init(gpio, direction, initialValue);
    }
    return IQRF_GPIO_ERROR_INVALID_DRIVER;
#endif
}

iqrf_gpio_error_t iqrf_gpio_init_input(iqrf_gpio_t *gpio) {
#ifdef WIN32
    return IQRF_GPIO_ERROR_OK;
#else
    return iqrf_gpio_init(gpio, IQRF_GPIO_DIRECTION_IN, false);
#endif
}

iqrf_gpio_error_t iqrf_gpio_init_output(iqrf_gpio_t *gpio, bool initialValue) {
#ifdef WIN32
    return IQRF_GPIO_ERROR_OK;
#else
    return iqrf_gpio_init(gpio, IQRF_GPIO_DIRECTION_OUT, initialValue);
#endif
}

iqrf_gpio_error_t iqrf_gpio_get_direction(iqrf_gpio_t *gpio, iqrf_gpio_direction_t *direction) {
#ifdef WIN32
    return IQRF_GPIO_ERROR_OK;
#else
    iqrf_gpio_error_t error = iqrf_gpio_sanity_check(gpio);
    if (error != IQRF_GPIO_ERROR_OK) {
        return error;
    }
    if (gpio->driver == IQRF_GPIO_DRIVER_SYSFS) {
        return iqrf_gpio_sysfs_get_direction(gpio, direction);
    } else if (gpio->driver == IQRF_GPIO_DRIVER_LIBGPIOD) {
        return IQRF_GPIO_ERROR_NOT_IMPLEMENTED;
    }
    return IQRF_GPIO_ERROR_INVALID_DRIVER;
#endif
}

iqrf_gpio_error_t iqrf_gpio_set_direction(iqrf_gpio_t *gpio, iqrf_gpio_direction_t direction) {
#ifdef WIN32
    return IQRF_GPIO_ERROR_OK;
#else
    iqrf_gpio_error_t error = iqrf_gpio_sanity_check(gpio);
    if (error != IQRF_GPIO_ERROR_OK) {
        return error;
    }
    if (gpio->driver == IQRF_GPIO_DRIVER_SYSFS) {
        return iqrf_gpio_sysfs_set_direction(gpio, direction);
    } else if (gpio->driver == IQRF_GPIO_DRIVER_LIBGPIOD) {
        return IQRF_GPIO_ERROR_NOT_IMPLEMENTED;
    }
    return IQRF_GPIO_ERROR_INVALID_DRIVER;
#endif
}
