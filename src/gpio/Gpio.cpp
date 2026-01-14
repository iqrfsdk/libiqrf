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

Gpio::Gpio(const GpioConfig& config) {
#if IQRF_TESTING_SUPPORT
    if (config.use_mock) {
        this->impl = std::make_shared<iqrf::gpio::GpioMock>(config);
        this->isMock = true;
        return;
    }
#endif
#if defined(__linux__)
    this->impl = std::make_shared<iqrf::gpio::Gpiod>(config);
#elif defined(__FreeBSD__)
    this->impl = std::make_shared<iqrf::gpio::GpioFreeBsd>(config);
#endif
}

Gpio::Gpio(const Gpio& other) noexcept : impl(other.impl), isMock(other.isMock) {
}

Gpio::Gpio(Gpio&& other) noexcept : impl(std::move(other.impl)), isMock(other.isMock) {
}

Gpio::~Gpio() {
    impl.reset();
}

Gpio& Gpio::operator=(Gpio other) noexcept {
    // copy-and-swap idiom
    swap(*this, other);
    return *this;
}

void Gpio::initInput() const {
    impl->initInput();
}

void Gpio::initOutput(const bool initialValue) const {
    impl->initOutput(initialValue);
}

void Gpio::setDirection(const iqrf::gpio::GpioDirection direction) const {
    impl->setDirection(direction);
}

iqrf::gpio::GpioDirection Gpio::getDirection() const {
    return impl->getDirection();
}

void Gpio::setValue(const bool value) const {
    impl->setValue(value);
}

bool Gpio::getValue() const {
    return impl->getValue();
}

void swap(Gpio& first, Gpio& second) noexcept {
    using std::swap;  // Enable ADL
    swap(first.impl, second.impl);
    swap(first.isMock, second.isMock);
}

#if IQRF_TESTING_SUPPORT
void Gpio::setInputValue(const bool value) const {
    if (!this->isMock) {
        throw std::logic_error("setInputValue is only available for mock GPIO");
    }
    std::dynamic_pointer_cast<GpioMock>(impl)->setInputValue(value);
}

void Gpio::registerDirectionCallback(const GpioDirectionCallback& callback) const {
    if (!this->isMock) {
        throw std::logic_error("registerDirectionCallback is only available for mock GPIO");
    }
    std::dynamic_pointer_cast<GpioMock>(this->impl)->registerDirectionCallback(callback);
}


void Gpio::registerValueCallback(const GpioValueCallback& callback) const {
    if (!this->isMock) {
        throw std::logic_error("registerValueCallback is only available for mock GPIO");
    }
    std::dynamic_pointer_cast<GpioMock>(this->impl)->registerValueCallback(callback);
}
#endif

}  // namespace iqrf::gpio
