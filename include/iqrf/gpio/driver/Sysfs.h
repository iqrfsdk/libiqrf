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

#include <cstdint>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <boost/filesystem.hpp>

#if BUILD_TESTS
#include <gtest/gtest_prod.h>
#endif

#include "iqrf/gpio/Common.h"
#include "iqrf/gpio/driver/Base.h"

namespace iqrf::gpio::driver {

	/**
	 * sysfs GPIO pin actions
	 */
	enum class SysfsActions {
		/// Export GPIO pin
		Export,
		/// Unexport GPIO pin
		Unexport,
		/// Get/Set GPIO pin direction
		Direction,
		/// Get/Set GPIO pin value
		Value,
		/// Export check
		CheckExport,
	};

	/**
	 * sysfs GPIO pin configuration
	 */
	struct SysfsConfig {
		/// GPIO pin number
		int64_t pin;
	};

	/**
	 * GPIO driver - sysfs
	 */
	class Sysfs: public Base {
	public:
		/**
		 * Constructor
		 * @param config
		 */
		explicit Sysfs(SysfsConfig config);

		/**
		 * Destructor
		 */
		~Sysfs() override;

		/**
		 * Initializes GPIO pin as an input
		 */
		void initInput() override;

		/**
		 * Initializes GPIO pin as an output
		 * @param initialValue Initial output value
		 */
		void initOutput(bool initialValue) override;

		/**
		 * Destroys GPIO pin
		 */
		void destroy() override;

		/**
		 * Sets GPIO pin direction
		 * @param direction GPIO pin direction
		 */
		void setDirection(iqrf::gpio::GpioDirection direction) override;

		/**
		 * Retrieves GPIO pin direction
		 * @return GPIO pin direction
		 */
		iqrf::gpio::GpioDirection getDirection() override;

		/**
		 * Sets GPIO pin output value
		 * @param value GPIO pin output value
		 */
		void setValue(bool value) override;

		/**
		 * Retrieves GPIO pin output value
		 * @return GPIO pin output value
		 */
		bool getValue() override;
	private:
#if BUILD_TESTS
		friend class SysfsTest;
		FRIEND_TEST(SysfsTest, createSysfsPath);
#endif
		/// GPIO pin configuration
		SysfsConfig config;

		/**
		 * Returns sysfs path for given action
		 * @param action GPIO action
		 * @return sysfs path
		 */
		boost::filesystem::path createSysfsPath(SysfsActions action) const;

		/**
		 * Checks if GPIO pin is exported
		 * @throws std::runtime_error if GPIO pin is not exported
		 */
		void checkExport();

		/**
		 * Exports GPIO pin
		 */
		void exportPin();

		/**
		 * Unexports GPIO pin
		 */
		void unexportPin();
	};

}


