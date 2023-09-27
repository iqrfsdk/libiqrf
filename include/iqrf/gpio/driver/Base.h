/**
 * Copyright 2023 MICRORISC s.r.o.
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

#include "iqrf/gpio/namespace.h"

namespace iqrf::gpio::driver {

	/**
	 * GPIO driver - base interface
	 */
	class Base {
	public:
		/**
		 * Destructor
		 */
		virtual ~Base() = default;
		/**
		 * Initializes GPIO as an input
		 */
		virtual void initInput() = 0;

		/**
		 * Initializes GPIO pin as an output
		 * @param initialValue
		 */
		virtual void initOutput(bool initialValue) = 0;

        /**
         * Destroys GPIO pin
         */
        virtual void destroy() = 0;

		/**
		 * Sets GPIO pin direction
		 * @param direction GPIO pin direction
		 */
		virtual void setDirection(iqrf::gpio::GpioDirection direction) = 0;

		/**
		 * Retrieves GPIO pin direction
		 * @return GPIO pin direction
		 */
		virtual iqrf::gpio::GpioDirection getDirection() = 0;

		/**
		 * Sets GPIO pin output value
		 * @param value GPIO pin output value
		 */
		virtual void setValue(bool value) = 0;

		/**
		 * Retrieves GPIO pin output value
		 * @return GPIO pin output value
		 */
		virtual bool getValue() = 0;

	};

}


