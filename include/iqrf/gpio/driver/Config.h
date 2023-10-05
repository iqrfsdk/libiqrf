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
#include <string>
#include <sstream>
#include <iostream>

namespace iqrf::gpio::driver {

	/**
	 *
   */
	class Config {
		
		/**
		 * Returns textual representation of the configuration.
		 * 
		 * Additionally, as an only always present virtual class used for safe downcasting.
		 */
		virtual const ::std::string to_string() const {
			::std::ostringstream ss;
			ss << "Base driver configuration";
			return ss.str();
		};
	};
}
