#pragma once

#include <exception>
#include <cstdint>
#include <stdexcept>
#include <string>

#include "iqrf/gpio/iqrf_gpio.h"

namespace iqrf {

	/**
	 * GPIO pin direction
	 */
	enum class GpioDirection {
		/// GPIO pin direction is input
		Input = iqrf_gpio_direction_t::IQRF_GPIO_DIRECTION_IN,
		/// GPIO pin direction is output
		Output = iqrf_gpio_direction_t::IQRF_GPIO_DIRECTION_OUT,
	};

	/**
	 * GPIO pin
	 */
	class Gpio {
	public:
		/**
		 * Construct GPIO pin instance
		 * @param gpio GPIO pin structure
		 */
		explicit Gpio(const iqrf_gpio_t *gpio);

		/**
		 * Construct GPIO pin via SYSFS interface
		 * @param pin GPIO pin number
		 * @param direction GPIO pin direction
		 * @param initialValue Initial output value
		 */
		explicit Gpio(int64_t pin);

		/**
		 * Construct GPIO pin via libgpiod interface
		 * @param chip GPIO chip
		 * @param line GPIO line
		 * @param consumer GPIO consumer name
		 */
		Gpio(struct gpiod_chip *chip, struct gpiod_line *line, const std::string &consumer);

		/**
		 * Destroys a GPIO pin
		 */
		~Gpio();

		/**
		 * Initializes the GPIO pin as an input
		 */
		void initInput();

		/**
		 * Initializes the GPIO pin as an output
		 * @param initialValue Output pin initial value
		 */
		void initOutput(bool initialValue);

		/**
		 * Retrieves the direction for GPIO pin
		 * @return GPIO pin direction
		 */
		iqrf::GpioDirection getDirection();

		/**
		 * Sets the direction for GPIO pin
		 * @param direction GPIO pin direction
		 */
		void setDirection(iqrf::GpioDirection direction);

		/**
		 * Retrieves the value of GPIO pin
		 * @return GPIO pin value
		 */
		bool getValue();

		/**
		 * Sets the value of GPIO pin
		 * @param value GPIO pin value
		 */
		void setValue(bool value);

	private:
		/// GPIO pin structure
		iqrf_gpio_t *gpio;
	};

}
