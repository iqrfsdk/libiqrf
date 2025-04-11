/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: GpioResolver.cpp
 * Authors: Karel Hanák <karel.hanak@iqrf.com>, Ondřej Hujňák <ondrej.hujnak@iqrf.com>
 * Date: 2024-09-29
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <iostream>
#include <utility>

#include "iqrf/gpio/GpioResolver.h"

namespace iqrf::gpio {

GpioResolver* GpioResolver::gpioResolverInstance{nullptr};
std::mutex GpioResolver::gpioResolverMtx;

GpioResolver::GpioResolver(): gpioMap(getGpioMap()) {
}

GpioResolver::GpioResolver(GpioMap map): gpioMap(std::move(map)) {
}

GpioResolver::~GpioResolver() {
    delete gpioResolverInstance;
}

GpioResolver* GpioResolver::GetResolver() {
    std::lock_guard<std::mutex> lock(gpioResolverMtx);
    if (gpioResolverInstance == nullptr) {
        gpioResolverInstance = new GpioResolver();
    }
    return gpioResolverInstance;
}

GpioResolver* GpioResolver::GetResolver(const GpioMap& map) {
    std::lock_guard<std::mutex> lock(gpioResolverMtx);
    if (gpioResolverInstance == nullptr) {
        gpioResolverInstance = new GpioResolver(map);
    }
    return gpioResolverInstance;
}

void GpioResolver::resolveGpioPin(const int64_t pin, ::std::string& chip, ::std::size_t& line) {
    const auto record = gpioMap.find(pin);
    if (record == gpioMap.end()) {
        throw std::runtime_error("No chip and line found for pin no. " + std::to_string(pin));
    }
    auto pair = record->second;
    chip = *pair.first;
    line = pair.second;
}

void GpioResolver::dump() const {
    for (const auto& record : gpioMap) {
        std::cout << *record.second.first
                  << " - line "
                  << record.second.second
                  << " (pin " << record.first << ')'
                  << std::endl;
    }
}

}  // namespace iqrf::gpio
