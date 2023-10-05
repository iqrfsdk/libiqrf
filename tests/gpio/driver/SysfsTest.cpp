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

#include <stdexcept>

#include <gtest/gtest.h>

#include "iqrf/gpio/driver/Sysfs.h"

namespace iqrf::gpio::driver {

	/**
	 * GPIO driver test - sysfs
	 */
	class SysfsTest : public ::testing::Test {
	protected:
		/**
		 * Sets up the test fixture.
		 */
		void SetUp() override {
			this->driver = new Sysfs(this->config);
		}

		/**
		 * Tears down the test fixture.
		 */
		void TearDown() override {
			delete this->driver;
		}

		/// GPIO driver instance
		Sysfs *driver = nullptr;

		/// GPIO driver config
		SysfsConfigStruct config = {
				.pin = 1,
		};
	};

	/**
	 * Tests the method to create sysfs paths
	 */
	TEST_F(SysfsTest, createSysfsPath) {
		ASSERT_STREQ("/sys/class/gpio/export", this->driver->createSysfsPath(SysfsActions::Export).c_str());
		ASSERT_STREQ("/sys/class/gpio/unexport", this->driver->createSysfsPath(SysfsActions::Unexport).c_str());
		ASSERT_STREQ("/sys/class/gpio/gpio1/direction", this->driver->createSysfsPath(SysfsActions::Direction).c_str());
		ASSERT_STREQ("/sys/class/gpio/gpio1/value", this->driver->createSysfsPath(SysfsActions::Value).c_str());
		ASSERT_STREQ("/sys/class/gpio/gpio1", this->driver->createSysfsPath(SysfsActions::CheckExport).c_str());
		try {
			this->driver->createSysfsPath(static_cast<SysfsActions>(-1));
			FAIL();
		} catch (const std::runtime_error &e) {
			ASSERT_STREQ("Unknown sysfs action.", e.what());
		} catch (...) {
			FAIL();
		}
	}

}
