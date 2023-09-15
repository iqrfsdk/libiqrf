#include "iqrf/gpio/iqrf_gpio.hpp"

namespace iqrf {

	Gpio::Gpio(const iqrf_gpio_t *gpio) {
		this->gpio = new iqrf_gpio_t();
		std::copy(gpio, gpio + sizeof(iqrf_gpio_t), this->gpio);
	}

	Gpio::Gpio(const int64_t pin) {
		this->gpio = new iqrf_gpio_t();
		this->gpio->driver = IQRF_GPIO_DRIVER_SYSFS;
		this->gpio->sysfs.pin = pin;
	}

	Gpio::Gpio(struct gpiod_chip *chip, struct gpiod_line *line, const std::string &consumer) {
		this->gpio = new iqrf_gpio_t();
		this->gpio->driver = IQRF_GPIO_DRIVER_LIBGPIOD;
		this->gpio->libgpiod.chip = chip;
		this->gpio->libgpiod.line = line;
		this->gpio->libgpiod.consumer = consumer.c_str();
	}

	Gpio::~Gpio() {
		iqrf_gpio_destroy(this->gpio);
		delete this->gpio;
	}

	void Gpio::initInput() {
		iqrf_gpio_error_t result = iqrf_gpio_init_input(this->gpio);
		if (result != IQRF_GPIO_ERROR_OK) {
			throw std::runtime_error("Failed to initialize GPIO pin as an input");
		}
	}

	void Gpio::initOutput(bool initialValue) {
		iqrf_gpio_error_t result = iqrf_gpio_init_output(this->gpio, initialValue);
		if (result != IQRF_GPIO_ERROR_OK) {
			throw std::runtime_error("Failed to initialize GPIO pin as an output");
		}
	}

    iqrf::GpioDirection Gpio::getDirection() {
		iqrf_gpio_direction_t direction;
		iqrf_gpio_error_t result = iqrf_gpio_get_direction(this->gpio, &direction);
		if (result != IQRF_GPIO_ERROR_OK) {
			throw std::runtime_error("Failed to retrieve GPIO pin direction");
		}
		return static_cast<iqrf::GpioDirection>(direction);
	}

	void Gpio::setDirection(iqrf::GpioDirection direction) {
		iqrf_gpio_error_t result = iqrf_gpio_set_direction(this->gpio, static_cast<iqrf_gpio_direction_t>(direction));
		if (result != IQRF_GPIO_ERROR_OK) {
			throw std::runtime_error("Failed to set GPIO pin direction");
		}
	}

	bool Gpio::getValue() {
		bool value;
		iqrf_gpio_error_t result = iqrf_gpio_get_value(this->gpio, &value);
		if (result != IQRF_GPIO_ERROR_OK) {
			throw std::runtime_error("Failed to retrieve GPIO pin value");
		}
		return value;
	}

	void Gpio::setValue(bool value) {
		iqrf_gpio_error_t result = iqrf_gpio_set_value(this->gpio, value);
		if (result != IQRF_GPIO_ERROR_OK) {
			throw std::runtime_error("Failed to set GPIO pin value");
		}
	}

}
