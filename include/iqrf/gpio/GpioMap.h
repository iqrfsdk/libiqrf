/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: GpioMap.h
 * Authors: Karel Hanák <karel.hanak@iqrf.com>
 * Date: 2024-09-29
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#pragma once

#include <map>
#include <memory>
#include <utility>
#include <string>

#if defined(__linux__)
#include <gpiod.hpp>
#elif defined(__FreeBSD__)
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/gpio.h>
#endif

#include "iqrf/gpio/version.h"

namespace iqrf::gpio {

    typedef std::map<std::size_t, std::pair<std::shared_ptr<std::string>, size_t>> GpioMap;

    /**
     * Get map of GPIO pins and chips names / line offsets
     * @return Map of GPIO pins and chips / line offsets
     */
    GpioMap getGpioMap();
}
