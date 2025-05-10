/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: Config.cpp
 * Authors: Karel Hanák <karel.hanak@iqrf.com>, Ondřej Hujňák <ondrej.hujnak@iqrf.com>
 * Date: 2024-09-29
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include "iqrf/gpio/Config.h"

namespace iqrf::gpio {

GpioConfig::GpioConfig(): line(0), pin(-1), consumer_name(default_consumer_name) {
    // Caution: Line is 0, which is a valid line number
}

GpioConfig::GpioConfig(const ::std::string& chip, const ::std::size_t line, const ::std::string& consumer_name)
    : chip(chip), line(line), pin(-1), consumer_name(consumer_name) {
}

GpioConfig::GpioConfig(const ::std::string& chip, const ::std::string& line_name, const ::std::string& consumer_name)
    : chip(chip), line(0), line_name(line_name), pin(-1), consumer_name(consumer_name) {
}

GpioConfig::GpioConfig(const int64_t pin, const ::std::string& consumer_name)
    : line(0), pin(pin), consumer_name(consumer_name) {

    auto *const gpioResolver = GpioResolver::GetResolver();
    gpioResolver->resolveGpioPin(pin, chip, line);
}

::std::string GpioConfig::to_string() const {
    ::std::ostringstream ss;
    ss << "Gpiod driver configuration: chip=" << chip << ", line=" << line;
    return ss.str();
}

}  // namespace iqrf::gpio
