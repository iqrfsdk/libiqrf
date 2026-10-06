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

#include <sys/ioctl.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>

#include "iqrf/log/Logging.h"

namespace iqrf::gpio {

GpioFreeBsd::GpioFreeBsd(const iqrf::gpio::GpioConfig &config):  line(config.line) {
    if (config.chip.empty()) {
        throw std::invalid_argument("GPIO chip name cannot be empty");
    }
    const std::string path = config.chip.substr(0, 5) == "/dev/" ? config.chip : "/dev/" + config.chip;
    this->fd = open(path.c_str(), O_RDONLY);  // NOLINT(cppcoreguidelines-pro-type-vararg)
    if (this->fd < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to open GPIO chip " + path);
    }
    if (config.consumer_name.empty()) {
        return;
    }
    // FreeBSD has no concept of GPIO line consumer, the pin name is used instead. The name is changed
    // system-wide, so the original name is restored in the destructor.
    try {
        const struct gpio_pin pin = this->getPinConfig();
        const char *name = static_cast<const char *>(pin.gp_name);
        this->originalName = std::string(name, strnlen(name, GPIOMAXNAME));
        this->setPinName(config.consumer_name);
    } catch (...) {
        close(this->fd);
        throw;
    }
}

GpioFreeBsd::~GpioFreeBsd() {
    if (this->fd < 0) {
        return;
    }
    if (this->originalName.has_value()) {
        try {
            this->setPinName(*this->originalName);
        } catch (const std::exception &e) {
            // Destructor must not throw, the pin keeps the consumer name
            IQRF_LOG(iqrf::log::Level::Warning) << "Failed to restore GPIO pin " << this->line
                << " name \"" << *this->originalName << "\": " << e.what();
        }
    }
    close(this->fd);
}

void GpioFreeBsd::setPinName(const std::string &name) const {
    struct gpio_pin pin{};
    pin.gp_pin = this->line;
    strlcpy(static_cast<char *>(pin.gp_name), name.c_str(), GPIOMAXNAME);
    if (this->ioctl(GPIOSETNAME, &pin) < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to set GPIO pin name");
    }
}

void GpioFreeBsd::initInput() {
    this->setDirection(GpioDirection::Input);
}

void GpioFreeBsd::initOutput(const bool initialValue) {
    // Preset the output latch before enabling the output to avoid glitches, drivers without the latch
    // support may reject setting value of an input pin, so the failure is ignored
    struct gpio_req rq{};
    rq.gp_pin = this->line;
    rq.gp_value = initialValue ? GPIO_PIN_HIGH : GPIO_PIN_LOW;
    static_cast<void>(this->ioctl(GPIOSET, &rq));

    this->configureDirection(GpioDirection::Output, initialValue ? GPIO_PIN_PRESET_HIGH : GPIO_PIN_PRESET_LOW);
    this->setValue(initialValue);
}

struct gpio_pin GpioFreeBsd::getPinConfig() const {
    struct gpio_pin pin{};
    pin.gp_pin = this->line;
    if (this->ioctl(GPIOGETCONFIG, &pin) < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to get GPIO pin configuration");
    }
    return pin;
}

void GpioFreeBsd::setDirection(const iqrf::gpio::GpioDirection direction) {
    this->configureDirection(direction, 0);
}

void GpioFreeBsd::configureDirection(const iqrf::gpio::GpioDirection direction, const uint32_t extraFlags) {
    struct gpio_pin pin = this->getPinConfig();
    // Keep other pin flags (pull-up/pull-down, inversion, ...), drop direction, presets and interrupt
    // configuration (output cannot be combined with interrupt flags)
    uint32_t flags = pin.gp_flags &
        ~(GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_PRESET_LOW | GPIO_PIN_PRESET_HIGH | GPIO_INTR_MASK);
    if (direction == GpioDirection::Input) {
        flags |= GPIO_PIN_INPUT;
    } else {
        flags |= GPIO_PIN_OUTPUT | extraFlags;
    }
    pin.gp_flags = flags;
    if (this->ioctl(GPIOSETCONFIG, &pin) < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to set GPIO direction");
    }
}

iqrf::gpio::GpioDirection GpioFreeBsd::getDirection() {
    const struct gpio_pin pin = this->getPinConfig();
    return (pin.gp_flags & GPIO_PIN_INPUT) != 0 ? GpioDirection::Input : GpioDirection::Output;
}

void GpioFreeBsd::setValue(const bool value) {
    struct gpio_req rq{};
    rq.gp_pin = this->line;
    rq.gp_value = value ? GPIO_PIN_HIGH : GPIO_PIN_LOW;

    if (this->ioctl(GPIOSET, &rq) < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to set GPIO value");
    }
}

bool GpioFreeBsd::getValue() {
    struct gpio_req rq{};
    rq.gp_pin = this->line;
    if (this->ioctl(GPIOGET, &rq) < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to get GPIO value");
    }
    return rq.gp_value != 0;
}

int GpioFreeBsd::ioctl(const unsigned long request, void *argument) const {  // NOLINT(runtime/int)
    return ::ioctl(this->fd, request, argument);  // NOLINT(cppcoreguidelines-pro-type-vararg)
}

}  // namespace iqrf::gpio
