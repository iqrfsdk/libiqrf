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

#include <string>
#include <sstream>
#include <iostream>

#include "GpioResolver.h"

namespace iqrf::gpio {

	/**
	 * GPIO chip config
	 */
	class GpioConfig {
	public:
		/// GPIO chip name 
		::std::string chip;
		/// GPIO line offset
		unsigned int line;
		/// TODO: Lines can be named, allow for declaring the line by ::std::string
		/// GPIO pin number (sysfs compatibility)
		int64_t pin;

		/**
		 * Empty constructor for filling up the data later
		 */
		GpioConfig();

		/**
		 * Constructor
		 */
		GpioConfig( ::std::string chip, unsigned int line );

		/**
		 * Compatibility constructor for sysfs
		 */
		GpioConfig( int64_t pin );

		/**
		 * Returns textual representation of the configuration.
		 */
		const ::std::string to_string() const;
	};
}
