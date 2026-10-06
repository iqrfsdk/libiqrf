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

#include "iqrf/gpio/GpioMock.h"

#include <mutex>
#include <stdexcept>
#include <utility>

namespace iqrf::gpio {

GpioMock::GpioMock(iqrf::gpio::GpioConfig config): config(std::move(config)) {
}

void GpioMock::checkInitialized() const {
    if (this->state == GpioMockState::Uninitialized) {
        throw std::runtime_error("GPIO line is not initialized");
    }
}

void GpioMock::initInput() {
    const std::scoped_lock lock(this->mutex);
    this->direction = GpioDirection::Input;
    this->state = GpioMockState::Initialized;
}

void GpioMock::initOutput(const bool initialValue) {
    const std::scoped_lock lock(this->mutex);
    this->direction = GpioDirection::Output;
    this->value = initialValue;
    this->state = GpioMockState::Initialized;
}

void GpioMock::setDirection(const iqrf::gpio::GpioDirection newDirection) {
    GpioDirectionCallback callback;
    GpioDirection oldDirection = GpioDirection::Input;
    {
        const std::scoped_lock lock(this->mutex);
        this->checkInitialized();
        oldDirection = this->direction;
        this->direction = newDirection;
        callback = this->directionCallback;
    }
    if (oldDirection != newDirection && callback) {
        callback(oldDirection, newDirection);
    }
}

iqrf::gpio::GpioDirection GpioMock::getDirection() {
    const std::scoped_lock lock(this->mutex);
    this->checkInitialized();
    return this->direction;
}

void GpioMock::setValue(const bool newValue) {
    GpioValueCallback callback;
    GpioWriteCallback onWrite;
    bool oldValue = false;
    {
        const std::scoped_lock lock(this->mutex);
        this->checkInitialized();
        if (this->direction != GpioDirection::Output) {
            throw std::runtime_error("Cannot set value on GPIO line that is not an output");
        }
        oldValue = this->value;
        this->value = newValue;
        callback = this->valueCallback;
        onWrite = this->writeCallback;
    }
    if (onWrite) {
        onWrite(newValue);
    }
    if (oldValue != newValue && callback) {
        callback(oldValue, newValue);
    }
}

bool GpioMock::getValue() {
    const std::scoped_lock lock(this->mutex);
    this->checkInitialized();
    return this->value;
}

void GpioMock::setInputValue(const bool newValue) {
    const std::scoped_lock lock(this->mutex);
    if (this->direction != GpioDirection::Input) {
        throw std::runtime_error("Cannot set input value on GPIO line that is not an input");
    }
    this->value = newValue;
}

GpioMockState GpioMock::getState() const {
    const std::scoped_lock lock(this->mutex);
    return this->state;
}

const GpioConfig& GpioMock::getConfig() const {
    return this->config;
}

void GpioMock::registerDirectionCallback(const GpioDirectionCallback& callback) {
    const std::scoped_lock lock(this->mutex);
    this->directionCallback = callback;
}

void GpioMock::registerValueCallback(const GpioValueCallback& callback) {
    const std::scoped_lock lock(this->mutex);
    this->valueCallback = callback;
}

void GpioMock::registerWriteCallback(const GpioWriteCallback& callback) {
    const std::scoped_lock lock(this->mutex);
    this->writeCallback = callback;
}

}  // namespace iqrf::gpio
