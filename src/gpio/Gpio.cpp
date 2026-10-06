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

#include <memory>
#include <stdexcept>
#include <utility>

#ifdef __linux__
#include "iqrf/gpio/libgpiodVersion.h"
#if libgpiod_VERSION_MAJOR == 1
#include "iqrf/gpio/GpiodV1.h"
#else
#include "iqrf/gpio/GpiodV2.h"
#endif
#elif defined(__FreeBSD__)
#include "iqrf/gpio/GpioFreeBsd.h"
#endif

namespace iqrf::gpio {

Gpio::Gpio(const GpioConfig& config) {
#ifdef __linux__
    this->impl = std::make_shared<iqrf::gpio::Gpiod>(config);
#elif defined(__FreeBSD__)
    this->impl = std::make_shared<iqrf::gpio::GpioFreeBsd>(config);
#else
    throw std::runtime_error("GPIO is not supported on this platform: " + config.to_string());
#endif
}

Gpio::Gpio(std::shared_ptr<iqrf::gpio::Base> impl): impl(std::move(impl)) {
    if (!this->impl) {
        throw std::invalid_argument("GPIO driver cannot be null");
    }
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
}

}  // namespace iqrf::gpio
