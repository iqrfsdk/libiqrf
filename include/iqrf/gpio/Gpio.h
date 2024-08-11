/**
 * Copyright 2023-2024 MICRORISC s.r.o.
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

#include "iqrf/gpio/Common.h"
#include "iqrf/gpio/driver/namespace.h"
#include "iqrf/gpio/driver/Base.h"
#include "iqrf/gpio/driver/Sysfs.h"

namespace iqrf::gpio {

	/**
	 * GPIO pin
	 */
	struct GpioConfig {
		GpioDriver driver;
		union {
			iqrf::gpio::driver::SysfsConfig sysfs;
		} config;
	};

	/**
	 * GPIO pin
	 */
	 class Gpio {
	 public:
		 /**
		  * Constructor
		  * @param config GPIO pin configuration
		  */
		 explicit Gpio(GpioConfig config);

		 /**
		  * Destructor
		  */
		 ~Gpio();

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
		  * Destroys GPIO pin
		  */
		 void destroy();

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
		  * Retrieves GPIO pin output value
		  * @return GPIO pin output value
		  */
		 bool getValue();
	 private:
		 /// GPIO driver instance
		 iqrf::gpio::driver::Base *driver;
		 /// GPIO pin configuration
		 GpioConfig config;
	 };

}

