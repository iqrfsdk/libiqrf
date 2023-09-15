#include "iqrf/gpio/iqrf_gpio_sysfs.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

static const char* IQRF_GPIO_SYSFS_ACTION_DIRECTION_STR = "direction";
static const char* IQRF_GPIO_SYSFS_ACTION_VALUE_STR = "value";
static const char* IQRF_GPIO_SYSFS_DIRECTION_IN_STR = "in";
static const char* IQRF_GPIO_SYSFS_DIRECTION_OUT_STR = "out";

/**
 * Checks the GPIO pin sanity
 * @param gpio GPIO pin
 * @return Execution status
 */
iqrf_gpio_error_t iqrf_gpio_sysfs_sanity_check(const iqrf_gpio_t *gpio) {
	if (gpio == NULL) {
		IQRF_LOG_PRINT(IQRF_LOG_LEVEL_FATAL, "GPIO is NULL");
		return IQRF_GPIO_ERROR_NULL_POINTER;
	}
	if (gpio->driver != IQRF_GPIO_DRIVER_SYSFS) {
		IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_FATAL, "Invalid GPIO driver: %d", gpio->driver);
		return IQRF_GPIO_ERROR_INVALID_DRIVER;
	}
	if (gpio->sysfs.pin < 0) {
		IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_ERROR, "Invalid GPIO pin number: %"PRId64, gpio->sysfs.pin);
		return IQRF_GPIO_ERROR_INVALID_PIN;
	}
	return IQRF_GPIO_ERROR_OK;
}

/**
 * Creates sysfs path
 * @param pin GPIO pin
 * @param action GPIO action
 * @param targetPath
 */
void iqrf_gpio_sysfs_create_path(const iqrf_gpio_t *gpio,  iqrf_gpio_sysfs_action_t action, char *targetPath) {
	const char *actionString = action == IQRF_GPIO_SYSFS_ACTION_DIRECTION ? IQRF_GPIO_SYSFS_ACTION_DIRECTION_STR : IQRF_GPIO_SYSFS_ACTION_VALUE_STR;
	snprintf(targetPath, IQRF_GPIO_SYSFS_BUFFER_SIZE, IQRF_GPIO_SYSFS_BASE_PATH"gpio%"PRId64"/%s", gpio->sysfs.pin, actionString);
}

iqrf_gpio_error_t iqrf_gpio_sysfs_export(const iqrf_gpio_t *gpio) {
	iqrf_gpio_error_t error = iqrf_gpio_sysfs_sanity_check(gpio);
	if (error != IQRF_GPIO_ERROR_OK) {
		return error;
	}
	IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_DEBUG, "Exporting GPIO pin #%"PRId64" via sysfs", gpio->sysfs.pin);
#ifdef WIN32
	return IQRF_GPIO_ERROR_OK;
#else
	bool isAlreadyExported;
	error = iqrf_gpio_sysfs_is_exported(gpio, &isAlreadyExported);
	if (error == IQRF_GPIO_ERROR_OK && isAlreadyExported) {
		IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_INFO, "GPIO pin #%"PRId64" is already exported", gpio->sysfs.pin);
		return IQRF_GPIO_ERROR_OK;
	}
	char *path = IQRF_GPIO_SYSFS_BASE_PATH"export";
	int fd = open(path, O_WRONLY);
	if (fd == -1) {
		IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_ERROR, "Unable to open path \"%s\". Reason: %s", path, strerror(errno));
		return IQRF_GPIO_ERROR_OPEN_FAILED;
	}
	char buffer[IQRF_GPIO_SYSFS_PIN_BUFFER_SIZE] = "";
	snprintf(buffer, IQRF_GPIO_SYSFS_PIN_BUFFER_SIZE, "%"PRId64, gpio->sysfs.pin);
	ssize_t writtenSize = write(fd, buffer, IQRF_GPIO_SYSFS_PIN_BUFFER_SIZE);
	if (writtenSize == -1) {
		close(fd);
		IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_ERROR, "Unable to write '%s' into \"%s\". Reason: %s", buffer, path, strerror(errno));
		return IQRF_GPIO_ERROR_WRITE_FAILED;
	}
	close(fd);
	IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_INFO, "GPIO pin #%"PRId64" is successfully exported via sysfs", gpio->sysfs.pin);
	return IQRF_GPIO_ERROR_OK;
#endif
}

iqrf_gpio_error_t iqrf_gpio_sysfs_unexport(const iqrf_gpio_t *gpio) {
	iqrf_gpio_error_t error = iqrf_gpio_sysfs_sanity_check(gpio);
	if (error != IQRF_GPIO_ERROR_OK) {
		return error;
	}
	IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_DEBUG, "Unexporting GPIO pin #%"PRId64" via sysfs", gpio->sysfs.pin);
#ifdef WIN32
	return IQRF_GPIO_ERROR_OK;
#else
	bool isAlreadyExported;
	error = iqrf_gpio_sysfs_is_exported(gpio, &isAlreadyExported);
	if (error == IQRF_GPIO_ERROR_OK && !isAlreadyExported) {
		IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_INFO, "GPIO pin #%"PRId64" is already unexported", gpio->sysfs.pin);
		return IQRF_GPIO_ERROR_OK;
	}
	char *path = IQRF_GPIO_SYSFS_BASE_PATH"unexport";
	int fd = open(path, O_WRONLY);
	if (fd == -1) {
		IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_ERROR, "Unable to open path \"%s\". Reason: %s", path, strerror(errno));
		return IQRF_GPIO_ERROR_OPEN_FAILED;
	}
	char buffer[IQRF_GPIO_SYSFS_PIN_BUFFER_SIZE] = "";
	snprintf(buffer, IQRF_GPIO_SYSFS_PIN_BUFFER_SIZE, "%"PRId64, gpio->sysfs.pin);
	ssize_t writtenSize = write(fd, buffer, IQRF_GPIO_SYSFS_PIN_BUFFER_SIZE);
	if (writtenSize == -1) {
		close(fd);
		IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_ERROR, "Unable to write '%s' into \"%s\". Reason: %s", buffer, path, strerror(errno));
		return IQRF_GPIO_ERROR_WRITE_FAILED;
	}
	close(fd);
	IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_INFO, "GPIO pin #%"PRId64" is successfully unexported via sysfs", gpio->sysfs.pin);
	return IQRF_GPIO_ERROR_OK;
#endif
}

iqrf_gpio_error_t iqrf_gpio_sysfs_is_exported(const iqrf_gpio_t *gpio, bool *isExported) {
	iqrf_gpio_error_t error = iqrf_gpio_sysfs_sanity_check(gpio);
	if (error != IQRF_GPIO_ERROR_OK) {
		return error;
	}
	IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_DEBUG, "Checking if GPIO pin #%"PRId64" is exported via sysfs", gpio->sysfs.pin);
#ifdef WIN32
	return IQRF_GPIO_ERROR_OK;
#else
	char path[IQRF_GPIO_SYSFS_BUFFER_SIZE] = "";
	snprintf(path, IQRF_GPIO_SYSFS_BUFFER_SIZE, IQRF_GPIO_SYSFS_BASE_PATH"gpio%"PRId64, gpio->sysfs.pin);
	if (access(path, F_OK) == 0) {
		*isExported = true;
		return IQRF_GPIO_ERROR_OK;
	}
	*isExported = false;
	return IQRF_GPIO_ERROR_OK;
#endif
}

iqrf_gpio_error_t iqrf_gpio_sysfs_get_direction(const iqrf_gpio_t *gpio, iqrf_gpio_direction_t *direction) {
	iqrf_gpio_error_t error = iqrf_gpio_sysfs_sanity_check(gpio);
	if (error != IQRF_GPIO_ERROR_OK) {
		return error;
	}
	IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_DEBUG, "Retrieving direction for GPIO pin #%"PRId64" via sysfs", gpio->sysfs.pin);
	if (direction == NULL) {
		IQRF_LOG_PRINT(IQRF_LOG_LEVEL_ERROR, "GPIO direction is NULL");
		return IQRF_GPIO_ERROR_NULL_POINTER;
	}
#ifdef WIN32
	return IQRF_GPIO_ERROR_OK;
#else
	char path[IQRF_GPIO_SYSFS_BUFFER_SIZE] = "";
	iqrf_gpio_sysfs_create_path(gpio, IQRF_GPIO_SYSFS_ACTION_DIRECTION, path);
	int fd = open(path, O_RDONLY);
	if (fd == -1) {
		IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_ERROR, "Unable to open path \"%s\". Reason: %s", path, strerror(errno));
		return IQRF_GPIO_ERROR_OPEN_FAILED;
	}
	char buffer[IQRF_GPIO_SYSFS_DIRECTION_BUFFER_SIZE] = "";
	ssize_t readSize = read(fd, buffer, IQRF_GPIO_SYSFS_DIRECTION_BUFFER_SIZE);
	if (readSize == -1) {
		close(fd);
		IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_ERROR, "Unable to read from %s", path);
		return IQRF_GPIO_ERROR_WRITE_FAILED;
	}
	if (strncmp(buffer, IQRF_GPIO_SYSFS_DIRECTION_IN_STR, IQRF_GPIO_SYSFS_DIRECTION_BUFFER_SIZE - 1) == 0) {
		*direction = IQRF_GPIO_DIRECTION_IN;
	} else if (strncmp(buffer, IQRF_GPIO_SYSFS_DIRECTION_OUT_STR, IQRF_GPIO_SYSFS_DIRECTION_BUFFER_SIZE - 1) == 0) {
		*direction = IQRF_GPIO_DIRECTION_OUT;
	} else {
		*direction = IQRF_GPIO_DIRECTION_UNKNOWN;
	}
	close(fd);
	return IQRF_GPIO_ERROR_OK;
#endif
}

iqrf_gpio_error_t iqrf_gpio_sysfs_set_direction(const iqrf_gpio_t *gpio, iqrf_gpio_direction_t direction) {
	iqrf_gpio_error_t error = iqrf_gpio_sysfs_sanity_check(gpio);
	if (error != IQRF_GPIO_ERROR_OK) {
		return error;
	}
	IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_DEBUG, "Setting direction for GPIO pin #%"PRId64" via sysfs", gpio->sysfs.pin);
#ifdef WIN32
	return IQRF_GPIO_ERROR_OK;
#else
	char path[IQRF_GPIO_SYSFS_BUFFER_SIZE] = "";
	iqrf_gpio_sysfs_create_path(gpio, IQRF_GPIO_SYSFS_ACTION_DIRECTION, path);
	int fd = open(path, O_WRONLY);
	if (fd == -1) {
		IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_ERROR, "Unable to open path \"%s\". Reason: %s", path, strerror(errno));
		return IQRF_GPIO_ERROR_OPEN_FAILED;
	}
	const char *buffer = direction == IQRF_GPIO_DIRECTION_IN ? IQRF_GPIO_SYSFS_DIRECTION_IN_STR : IQRF_GPIO_SYSFS_DIRECTION_OUT_STR;
	ssize_t writtenSize = write(fd, buffer, strlen(buffer));
	if (writtenSize == -1) {
		close(fd);
		IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_ERROR, "Unable to write '%s' into \"%s\". Reason: %s", buffer, path, strerror(errno));
		return IQRF_GPIO_ERROR_WRITE_FAILED;
	}
	close(fd);
	return IQRF_GPIO_ERROR_OK;
#endif
}

iqrf_gpio_error_t iqrf_gpio_sysfs_get_value(const iqrf_gpio_t *gpio, bool *value) {
	iqrf_gpio_error_t error = iqrf_gpio_sysfs_sanity_check(gpio);
	if (error != IQRF_GPIO_ERROR_OK) {
		return error;
	}
	IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_DEBUG, "Retrieving value for GPIO pin #%"PRId64" via sysfs", gpio->sysfs.pin);
	if (value == NULL) {
		IQRF_LOG_PRINT(IQRF_LOG_LEVEL_ERROR, "GPIO value is NULL");
		return IQRF_GPIO_ERROR_NULL_POINTER;
	}
#ifdef WIN32
	return IQRF_GPIO_ERROR_OK;
#else
	char path[IQRF_GPIO_SYSFS_BUFFER_SIZE] = "";
	iqrf_gpio_sysfs_create_path(gpio, IQRF_GPIO_SYSFS_ACTION_VALUE, path);
	int fd = open(path, O_RDONLY);
	if (fd == -1) {
		IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_ERROR, "Unable to open path \"%s\". Reason: %s", path, strerror(errno));
		return IQRF_GPIO_ERROR_OPEN_FAILED;
	}
	char buffer[IQRF_GPIO_SYSFS_VALUE_BUFFER_SIZE] = "";
	ssize_t readSize = read(fd, buffer, IQRF_GPIO_SYSFS_VALUE_BUFFER_SIZE);
	if (readSize == -1) {
		close(fd);
		IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_ERROR, "Unable to read from %s", path);
		return IQRF_GPIO_ERROR_WRITE_FAILED;
	}
	*value = strncmp(buffer, "1", IQRF_GPIO_SYSFS_VALUE_BUFFER_SIZE - 1) == 0;
	close(fd);
	return IQRF_GPIO_ERROR_OK;
#endif
}

iqrf_gpio_error_t iqrf_gpio_sysfs_set_value(const iqrf_gpio_t *gpio, bool value) {
	iqrf_gpio_error_t error = iqrf_gpio_sysfs_sanity_check(gpio);
	if (error != IQRF_GPIO_ERROR_OK) {
		return error;
	}
	IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_DEBUG, "Setting value for GPIO pin #%"PRId64" via sysfs", gpio->sysfs.pin);
#ifdef WIN32
	return IQRF_GPIO_ERROR_OK;
#else
	char path[IQRF_GPIO_SYSFS_BUFFER_SIZE] = "";
	iqrf_gpio_sysfs_create_path(gpio, IQRF_GPIO_SYSFS_ACTION_VALUE, path);
	int fd = open(path, O_WRONLY);
	if (fd == -1) {
		IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_ERROR, "Unable to open path \"%s\". Reason: %s", path, strerror(errno));
		return IQRF_GPIO_ERROR_OPEN_FAILED;
	}
	const char *buffer = value ? "1" : "0";
	ssize_t writtenSize = write(fd, buffer, 2);
	if (writtenSize == -1) {
		close(fd);
		IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_ERROR, "Unable to write '%s' into \"%s\". Reason: %s", buffer, path, strerror(errno));
		return IQRF_GPIO_ERROR_WRITE_FAILED;
	}
	close(fd);
	return IQRF_GPIO_ERROR_OK;
#endif
}

iqrf_gpio_error_t iqrf_gpio_sysfs_init(const iqrf_gpio_t *gpio, iqrf_gpio_direction_t direction, bool initialValue) {
	iqrf_gpio_error_t error = iqrf_gpio_sysfs_sanity_check(gpio);
	if (error != IQRF_GPIO_ERROR_OK) {
		return error;
	}
	IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_DEBUG, "Initializing GPIO pin #%"PRId64" via sysfs", gpio->sysfs.pin);
#ifdef WIN32
	return IQRF_GPIO_ERROR_OK;
#else
	error = iqrf_gpio_sysfs_export(gpio);
	if (error != IQRF_GPIO_ERROR_OK) {
		return error;
	}

	char *directionStr = direction == IQRF_GPIO_DIRECTION_IN ? "input" : "output";
	const uint8_t maxAttempts = 10;
	for (uint8_t i = 1; i <= maxAttempts; i++) {
		error = iqrf_gpio_sysfs_set_direction(gpio, direction);
		if (error == IQRF_GPIO_ERROR_OK) {
			IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_INFO, "GPIO pin #%"PRId64" direction \"%s\" is successfully set. Attempt: %d", gpio->sysfs.pin, directionStr, i);
			break;
		} else if (i == maxAttempts) {
			IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_ERROR, "Failed to set direction \"%s\" on GPIO pin #%"PRId64". Attempt: %d", directionStr, gpio->sysfs.pin, i);
			return error;
		}
		IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_DEBUG, "Failed to set direction \"%s\" on GPIO pin #%"PRId64". Wait for 100 ms to next try: %d", directionStr, gpio->sysfs.pin, i);
		IQRF_SLEEP(100);
	}

	if (direction == IQRF_GPIO_DIRECTION_OUT) {
		error = iqrf_gpio_sysfs_set_value(gpio, initialValue);
		if (error != IQRF_GPIO_ERROR_OK) {
			return error;
		}
	}

	return IQRF_GPIO_ERROR_OK;
#endif
}

iqrf_gpio_error_t iqrf_gpio_sysfs_destroy(const iqrf_gpio_t *gpio) {
	iqrf_gpio_error_t error = iqrf_gpio_sysfs_sanity_check(gpio);
	if (error != IQRF_GPIO_ERROR_OK) {
		return error;
	}
	IQRF_LOG_PRINTF(IQRF_LOG_LEVEL_DEBUG, "Destroying GPIO pin #%"PRId64" via sysfs", gpio->sysfs.pin);
#ifdef WIN32
	return IQRF_GPIO_ERROR_OK;
#else
	return iqrf_gpio_sysfs_unexport(gpio);
#endif
}
