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

	Gpio::Gpio(const GpioConfig& config) : impl(std::make_shared<iqrf::gpio::Gpiod>(config)) {
	}

	Gpio::Gpio(const Gpio& other) noexcept : impl(other.impl) {
	}

	Gpio::Gpio(Gpio&& other) noexcept : impl(std::move(other.impl)) {
	}

	Gpio::~Gpio() {
		impl.reset();
	}

	Gpio& Gpio::operator=(Gpio other) noexcept {
		// copy-and-swap idiom
		swap(*this, other);
		return *this;
	}

	void Gpio::initInput() {
		impl->initInput();
	}

	void Gpio::initOutput(bool initialValue) {
		impl->initOutput(initialValue);
	}

	void Gpio::setDirection(iqrf::gpio::GpioDirection direction) {
		impl->setDirection(direction);
	}

	iqrf::gpio::GpioDirection Gpio::getDirection() {
		return impl->getDirection();
	}

	void Gpio::setValue(bool value) {
		impl->setValue(value);
	}

	bool Gpio::getValue() {
		return impl->getValue();
	}

	void swap(Gpio& first, Gpio& second) {
		using std::swap;  // Enable ADL
		swap(first.impl, second.impl);
	}
}
