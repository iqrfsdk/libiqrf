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

#include "iqrf/gpio/GpioFreeBsd.h"

namespace iqrf::gpio {

GpioFreeBsd::GpioFreeBsd(const iqrf::gpio::GpioConfig &config): chip(config.chip), line(config.line), name(config.consumer_name) {
    if (config.chip.empty()) {
        throw std::invalid_argument("GPIO chip name cannot be empty");
    }
    std::string path = config.chip.substr(0, 5) == "/dev/" ? config.chip : "/dev/" + config.chip;
    this->fd = open(path.c_str(), O_RDONLY);
    if (this->fd < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to open GPIO chip");
    }
    if (!config.consumer_name.empty()) {
        struct gpio_pin pin;
        pin.gp_pin = this->line;
        strlcpy(pin.gp_name, config.consumer_name.c_str(), GPIOMAXNAME);
        if (ioctl(this->fd, GPIOSETNAME, &pin) < 0) {
            close(this->fd);
            throw std::system_error(errno, std::generic_category(), "Failed to set GPIO consumer name");
        }
    }
}

GpioFreeBsd::~GpioFreeBsd() {
    if (this->fd >= 0) {
        close(this->fd);
    }
}

void GpioFreeBsd::initInput() {
    this->setDirection(GpioDirection::Input);
}

void GpioFreeBsd::initOutput(bool initialValue) {
    this->setDirection(GpioDirection::Output);
    this->setValue(initialValue);
}

struct gpio_pin GpioFreeBsd::getPinConfig() const {
    struct gpio_pin pin;
    pin.gp_pin = this->line;
    if (ioctl(this->fd, GPIOGETCONFIG, &pin) < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to get GPIO pin configuration");
    }
    return pin;
}

void GpioFreeBsd::setDirection(iqrf::gpio::GpioDirection direction) {
    struct gpio_pin pin = this->getPinConfig();
    pin.gp_flags = direction == GpioDirection::Input ? GPIO_PIN_INPUT : GPIO_PIN_OUTPUT;

    if (ioctl(this->fd, GPIOSETCONFIG, &pin) < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to set GPIO direction");
    }
}

iqrf::gpio::GpioDirection GpioFreeBsd::getDirection() {
    struct gpio_pin pin = this->getPinConfig();
    return (pin.gp_flags & GPIO_PIN_INPUT) ? GpioDirection::Input : GpioDirection::Output;
}

void GpioFreeBsd::setValue(bool value) {
    struct gpio_req rq;
    rq.gp_pin = this->line;
    rq.gp_value = value ? 1 : 0;

    if (ioctl(this->fd, GPIOSET, &rq) < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to set GPIO value");
    }
}

bool GpioFreeBsd::getValue() {
    struct gpio_req rq;
    rq.gp_pin = this->line;
    if (ioctl(this->fd, GPIOGET, &rq) < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to get GPIO value");
    }
    return rq.gp_value != 0;
}

}
