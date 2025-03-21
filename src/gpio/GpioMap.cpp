/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: GpioMap.cpp
 * Authors: Karel Hanák <karel.hanak@iqrf.com>, Ondřej Hujňák <ondrej.hujnak@iqrf.com>
 * Date: 2024-09-29
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <algorithm>
#include <string>
#include <vector>

#include "iqrf/gpio/GpioMap.h"

#define GPIO_DIRECTORY "/dev"
#define GPIO_CHIP_PREFIX "gpiochip"
#define GPIO_CHIP_PREFIX_LEN (sizeof(GPIO_CHIP_PREFIX) - 1)  // Excluding the null terminator

namespace iqrf::gpio {

GpioMap getGpioMap() {
    GpioMap map;
    std::size_t npin = 0;

#if LIBGPIOD_VERSION_MAJOR == 1
    for (auto &chip : ::gpiod::make_chip_iter()) {
        std::shared_ptr<std::string> chip_name = std::make_shared<std::string>(chip.name());

        for (auto &line : ::gpiod::line_iter(chip)) {
            map.insert_or_assign(npin++, std::make_pair(chip_name, line.offset()));
        }
    }
#else
    std::vector< std::pair<unsigned long, std::filesystem::path> > chips;  // NOLINT(runtime/int)

    // Load all GPIO chips
    for (auto const &entry : std::filesystem::directory_iterator{GPIO_DIRECTORY}) {
        auto const &path_str = entry.path().filename().string();
        if (path_str.find(GPIO_CHIP_PREFIX) == 0) {  // We care only about /dev/gpiochip* entries
            if (gpiod::is_gpiochip_device(entry.path())) {  // Make sure it is GPIO
                try {
                    chips.emplace_back(
                        std::stoul(path_str.substr(GPIO_CHIP_PREFIX_LEN)),
                        entry.path());
                } catch (const std::exception &e) {}  // Ignore invalid chip names
            }
        }
    }

    // Sort the chips in numerical order by their ordinal number
    std::sort(chips.begin(), chips.end(), [](const auto &a, const auto &b) { return a.first < b.first; });

    // Create the mapping according to the sorted chips
    for (auto const &[_, chip_path] : chips) {
        try {
            auto chip_info = ::gpiod::chip(chip_path).get_info();
            std::shared_ptr<std::string> chip_name = std::make_shared<std::string>(chip_info.name());

            for (std::size_t line = 0; line < chip_info.num_lines(); ++line) {
                map.insert_or_assign(npin++, std::make_pair(chip_name, line));
            }
        } catch (const std::exception &e) {}  // Ignore the failed chips
    }
#endif

    return map;
}

}  // namespace iqrf::gpio
