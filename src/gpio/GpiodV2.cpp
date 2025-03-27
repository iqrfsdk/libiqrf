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

#include "iqrf/gpio/GpiodV2.h"

namespace iqrf::gpio {

Gpiod::Gpiod(GpioConfig config)
    : chip(std::make_unique<::gpiod::chip>(::std::filesystem::path("/dev/" + config.chip))) {
    if (!chip.get()) {
        throw std::runtime_error("No GPIO chip '/dev/" + config.chip + "' found");
    }

    if (config.line_name.empty()) {
        line = config.line;
    } else {
        int offset;
        offset = chip->get_line_offset_from_name(config.line_name);
        if (offset >= 0) {
            line = offset;
        } else {
            throw std::runtime_error("No line '" + config.line_name + "' found at chip '" + config.chip + "'");
        }
    }
    name = config.consumer_name;
}

Gpiod::~Gpiod() {
    chip.reset();
}

void Gpiod::initInput() {
    // Initialize line request
    request = std::make_unique<::gpiod::line_request>(
        chip->prepare_request()
            .set_consumer(name)
            .add_line_settings(
                line,
                ::gpiod::line_settings()
                    .set_direction(::gpiod::line::direction::INPUT)
            )
            .do_request());
}

void Gpiod::initOutput(bool initialValue) {
    auto val = initialValue ? ::gpiod::line::value::ACTIVE : ::gpiod::line::value::INACTIVE;

    // Initialize line request
    request = std::make_unique<::gpiod::line_request>(
        chip->prepare_request()
            .set_consumer(name)
            .add_line_settings(
                line,
                ::gpiod::line_settings()
                    .set_direction(::gpiod::line::direction::OUTPUT)
                    .set_output_value(val)
            )
            .do_request());
}

void Gpiod::setDirection(iqrf::gpio::GpioDirection direction) {
    auto dir = (direction == iqrf::gpio::GpioDirection::Input) ?
        ::gpiod::line::direction::INPUT :
        ::gpiod::line::direction::OUTPUT;

    // Reconfigure line direction
    request->reconfigure_lines(
        ::gpiod::line_config()
            .add_line_settings(
                line,
                ::gpiod::line_settings()
                    .set_direction(dir)
            )
    );  // NOLINT(whitespace/parens)
}

iqrf::gpio::GpioDirection Gpiod::getDirection() {
    // Get line direction
    auto direction = chip->get_line_info(line).direction();
    switch (direction) {
        case ::gpiod::line::direction::OUTPUT:
            return iqrf::gpio::GpioDirection::Output;
        case ::gpiod::line::direction::INPUT:
            return iqrf::gpio::GpioDirection::Input;
        default:
            throw ::std::out_of_range("Unknown direction");
    }
}

void Gpiod::setValue(bool value) {
    auto val = value ? ::gpiod::line::value::ACTIVE : ::gpiod::line::value::INACTIVE;
    request->set_value(line, val);
}

bool Gpiod::getValue() {
    return request->get_value(line) == ::gpiod::line::value::ACTIVE;
}

}  // namespace iqrf::gpio
