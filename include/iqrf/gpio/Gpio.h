/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <gpiod.hpp>
#if BUILD_TESTS
#include <gtest/gtest_prod.h>
#endif

#include "iqrf/gpio/Common.h"
#include "iqrf/gpio/Base.h"

#if LIBGPIOD_VERSION_MAJOR == 1
#include "iqrf/gpio/GpiodV1.h"
#else
#include "iqrf/gpio/GpiodV2.h"
#endif

namespace iqrf::gpio {

	/**
	 * GPIO pin
	 */
	class Gpio {
	public:
		 /**
		  * Constructor
		  * @param config GPIO pin configuration
		  */
		explicit Gpio(const GpioConfig& config);

		/**
		 * Copy Constructor
		 * @param other Gpio object to copy
		 */
		Gpio(const Gpio& other) noexcept;

		/**
		 * Move Constructor
		 * @param other Gpio object to move
		 */
		Gpio(Gpio&& other) noexcept;

		 /**
		  * Destructor
		  */
		~Gpio();
		
		/**
		 * Assignment Operator
		 */
		Gpio& operator=(Gpio other) noexcept;

		 /**
		  * Initializes GPIO pin as an input
		  */
		void initInput();

		 /**
		  * Initializes GPIO pin as an output
		  * @param initialValue Initial output value
		  */
		void initOutput(bool initialValue);

		 /**
		  * Sets GPIO pin direction
		  * @param direction GPIO pin direction
		  */
		void setDirection(iqrf::gpio::GpioDirection direction);

		 /**
		  * Retrieves GPIO pin direction
		  * @return GPIO pin direction
		  */
		iqrf::gpio::GpioDirection getDirection();

		 /**
		  * Sets GPIO pin output value
		  * @param value GPIO pin output value
		  */
		void setValue(bool value);

		/**
		 * Retrieves GPIO line input value
		 * @return GPIO line input value
		 */
		bool getValue();

		 /**
		  * Swap function
		  */
		friend void swap(Gpio& first, Gpio& second);
	private:
		 /// GPIO driver instance
		std::shared_ptr<iqrf::gpio::Base> impl;

		 /// Testing friends
		FRIEND_TEST(GpioTest, VerifyCopyConstructor_GPIO);
		FRIEND_TEST(GpioTest, VerifyMoveConstructor_GPIO);
		FRIEND_TEST(GpioTest, VerifyAssignmentOperator_GPIO);
	};

}

