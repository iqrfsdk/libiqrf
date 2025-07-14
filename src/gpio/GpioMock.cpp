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

#include <utility>

#include "iqrf/gpio/GpioMock.h"

namespace iqrf::gpio {

GpioMock::GpioMock(iqrf::gpio::GpioConfig config): config(std::move(config)) {
    // No actual GPIO implementation, just a mock
}

void GpioMock::initInput() {
    this->direction = GpioDirection::Input;
    this->state = GpioMockState::Initialized;
}

void GpioMock::initOutput(bool initialValue) {
    this->direction = GpioDirection::Output;
    this->value = initialValue;
    this->state = GpioMockState::Initialized;
}

void GpioMock::setDirection(iqrf::gpio::GpioDirection direction) {
    if (this->state == GpioMockState::Uninitialized) {
        throw std::runtime_error("GPIO line is not initialized");
    }
    if (this->direction != direction && this->directionCallback) {
        this->directionCallback(this->direction, direction);
    }
    this->direction = direction;
}

iqrf::gpio::GpioDirection GpioMock::getDirection() {
    if (this->state == GpioMockState::Uninitialized) {
        throw std::runtime_error("GPIO line is not initialized");
    }
    return this->direction;
}

void GpioMock::setValue(bool value) {
    if (this->state == GpioMockState::Uninitialized) {
        throw std::runtime_error("GPIO line is not initialized");
    }
    if (this->direction != GpioDirection::Output) {
        throw std::runtime_error("Cannot set value on GPIO line that is not an output");
    }
    if (this->value != value && this->valueCallback) {
        this->valueCallback(this->value, value);
    }
    this->value = value;
}

void GpioMock::setInputValue(bool value) {
    if (this->direction != GpioDirection::Input) {
        throw std::runtime_error("Cannot set input value on GPIO line that is not an input");
    }
    this->value = value;
}

bool GpioMock::getValue() {
    if (this->state == GpioMockState::Uninitialized) {
        throw std::runtime_error("GPIO line is not initialized");
    }
    return this->value;
}

}  // namespace iqrf::gpio
