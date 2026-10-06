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

#include "iqrf/gpio/GpioResolver.h"

#include <cstddef>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

namespace iqrf::gpio {

GpioResolver* GpioResolver::GetResolver() {
    static GpioResolver instance;
    return &instance;
}

GpioResolver* GpioResolver::GetResolver(const GpioMap& map) {
    GpioResolver *resolver = GetResolver();
    resolver->setGpioMap(map);
    return resolver;
}

void GpioResolver::setGpioMap(GpioMap map) {
    const std::scoped_lock lock(this->mutex);
    this->gpioMap = std::move(map);
}

const GpioMap& GpioResolver::getMap() const {
    if (!this->gpioMap.has_value()) {
        this->gpioMap = getGpioMap();
    }
    return *this->gpioMap;
}

void GpioResolver::resolveGpioPin(const int64_t pin, ::std::string& chip, ::std::size_t& line) {
    const std::scoped_lock lock(this->mutex);
    const GpioMap &map = this->getMap();
    const auto record = pin < 0 ? map.end() : map.find(static_cast<std::size_t>(pin));
    if (record == map.end()) {
        throw std::runtime_error("No chip and line found for pin no. " + std::to_string(pin));
    }
    chip = *record->second.first;
    line = record->second.second;
}

void GpioResolver::dump() const {
    const std::scoped_lock lock(this->mutex);
    for (const auto& record : this->getMap()) {
        std::cout << *record.second.first
                  << " - line "
                  << record.second.second
                  << " (pin " << record.first << ')'
                  << '\n';
    }
}

}  // namespace iqrf::gpio
