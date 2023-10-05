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

#include "iqrf/gpio/Gpio.h"

namespace iqrf::gpio {

	Gpio::Gpio(iqrf::gpio::GpioConfig config): config(config) {
		switch (this->config.driver) {
			case GpioDriver::gpiod:
				throw std::runtime_error("libgpiod driver is not implemented yet.");
			case GpioDriver::sysfs:
				this->driver = new iqrf::gpio::driver::Sysfs(this->config.config.sysfs);
				break;
		}
	}

	Gpio::~Gpio() {
		delete this->driver;
	}

	void Gpio::initInput() {
		this->driver->initInput();
	}

	void Gpio::initOutput(bool initialValue) {
		this->driver->initOutput(initialValue);
	}

	void Gpio::destroy() {
		this->driver->destroy();
	}

	void Gpio::setDirection(iqrf::gpio::GpioDirection direction) {
		this->driver->setDirection(direction);
	}

	iqrf::gpio::GpioDirection Gpio::getDirection() {
		return this->driver->getDirection();
	}

	void Gpio::setValue(bool value) {
		this->driver->setValue(value);
	}

	bool Gpio::getValue() {
		return this->driver->getValue();
	}

}
