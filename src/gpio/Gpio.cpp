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

#include "iqrf/gpio/Gpio.h"

namespace iqrf::gpio {

	Gpio::Gpio(GpioConfig config) {
		this->impl = new iqrf::gpio::Gpiod(config);		
	}

	Gpio::~Gpio() {
		delete this->impl;
	}

	void Gpio::initInput() {
		this->impl->initInput();
	}

	void Gpio::initOutput(bool initialValue) {
		this->impl->initOutput(initialValue);
	}

	void Gpio::setDirection(iqrf::gpio::GpioDirection direction) {
		this->impl->setDirection(direction);
	}

	iqrf::gpio::GpioDirection Gpio::getDirection() {
		return this->impl->getDirection();
	}

	void Gpio::setValue(bool value) {
		this->impl->setValue(value);
	}

	bool Gpio::getValue() {
		return this->impl->getValue();
	}

}
