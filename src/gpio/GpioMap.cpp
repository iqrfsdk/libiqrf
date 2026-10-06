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

#include "iqrf/gpio/GpioMap.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "iqrf/log/Logging.h"

namespace iqrf::gpio {

namespace {

#if defined(__FreeBSD__) || (defined(__linux__) && libgpiod_VERSION_MAJOR != 1)
/// Directory with GPIO chip devices
constexpr const char *GPIO_DIRECTORY = "/dev";
#endif
#if defined(__linux__) && libgpiod_VERSION_MAJOR != 1
/// GPIO chip device name prefix
constexpr std::string_view GPIO_CHIP_PREFIX = "gpiochip";
#elif defined(__FreeBSD__)
/// GPIO chip device name prefix
constexpr std::string_view GPIO_CHIP_PREFIX = "gpioc";
#endif

}  // namespace

GpioMap getGpioMap() {
    GpioMap map;
    std::size_t npin = 0;

#ifdef __linux__
#if libgpiod_VERSION_MAJOR == 1
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
                        std::stoul(path_str.substr(GPIO_CHIP_PREFIX.size())),
                        entry.path());
                } catch (const std::exception &e) {
                    IQRF_LOG(iqrf::log::Level::Debug) << "Ignoring GPIO chip " << path_str << ": " << e.what();
                }
            }
        }
    }

    // Sort the chips in numerical order by their ordinal number
    std::sort(chips.begin(), chips.end(), [](const auto &a, const auto &b) { return a.first < b.first; });

    // Create the mapping according to the sorted chips
    for (auto const &[_, chip_path] : chips) {
        try {
            auto chip_info = ::gpiod::chip(chip_path).get_info();
            const std::shared_ptr<std::string> chip_name = std::make_shared<std::string>(chip_info.name());

            for (std::size_t line = 0; line < chip_info.num_lines(); ++line) {
                map.insert_or_assign(npin++, std::make_pair(chip_name, line));
            }
        } catch (const std::exception &e) {
            IQRF_LOG(iqrf::log::Level::Debug) << "Ignoring GPIO chip " << chip_path << ": " << e.what();
        }
    }
#endif
#elif defined(__FreeBSD__)
    std::vector<std::pair<unsigned long, std::filesystem::path>> chips;  // NOLINT(runtime/int)

    // Load all GPIO chips, we care only about /dev/gpioc* entries
    for (const auto &entry : std::filesystem::directory_iterator{GPIO_DIRECTORY}) {
        const auto &filename = entry.path().filename().string();
        if (filename.find(GPIO_CHIP_PREFIX) == 0) {
            try {
                chips.emplace_back(std::stoul(filename.substr(GPIO_CHIP_PREFIX.size())), entry.path());
            } catch (const std::exception &e) {
                IQRF_LOG(iqrf::log::Level::Debug) << "Ignoring GPIO chip " << filename << ": " << e.what();
            }
        }
    }

    // Sort the chips in numerical order by their ordinal number, directory iteration order is unspecified
    std::sort(chips.begin(), chips.end(), [](const auto &a, const auto &b) { return a.first < b.first; });

    // Create the mapping according to the sorted chips
    for (const auto &[_, chip_path] : chips) {
        const int fd = open(chip_path.c_str(), O_RDONLY);  // NOLINT(cppcoreguidelines-pro-type-vararg)
        if (fd < 0) {
            // Failed to open the chip file, skip it
            continue;
        }
        // GPIOMAXPIN returns the highest pin number, not the number of pins
        int max_pin = -1;
        const int result = ioctl(fd, GPIOMAXPIN, &max_pin);  // NOLINT(cppcoreguidelines-pro-type-vararg)
        close(fd);
        if (result < 0) {
            continue;
        }
        const std::shared_ptr<std::string> chip_name = std::make_shared<std::string>(chip_path.filename().string());
        for (int line = 0; line <= max_pin; ++line) {
            map.insert_or_assign(npin++, std::make_pair(chip_name, static_cast<std::size_t>(line)));
        }
    }
#endif

    return map;
}

}  // namespace iqrf::gpio
