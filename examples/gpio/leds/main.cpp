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

#include <csignal>
#include <iostream>
#include <thread>

#include <boost/program_options.hpp>

#include "iqrf/gpio/Gpio.h"

namespace bpo = boost::program_options;

/// Green LED GPIO pin instance
std::unique_ptr<iqrf::gpio::Gpio> greenLed = nullptr;
/// Red LED GPIO pin instance
std::unique_ptr<iqrf::gpio::Gpio> redLed = nullptr;

/**
 * Cleans up GPIO resources
 * This function is called on exit to ensure that the GPIO pins are set to a safe state.
 */
void gpioCleanup() {
    if (greenLed != nullptr) {
        greenLed->setDirection(iqrf::gpio::GpioDirection::Input);
    }
    if (redLed != nullptr) {
        redLed->setDirection(iqrf::gpio::GpioDirection::Input);
    }
}

/**
 * Signal handler
 * @param signal Signal number
 */
void signalHandler(const int signal) {
    std::cout << "Signal " << signal << " received. Exiting..." << std::endl;
    gpioCleanup();
    exit(signal);
}

/**
 * Main function
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code
 */
int main(int argc, char *argv[]) {
    bpo::options_description general("General options");
    general.add_options()
        ("help,h", "display help message");
    bpo::options_description command("Command options");
    command.add_options()
        ("chip-green,cg,c", bpo::value<std::string>(), "GPIO chip name for the green LED")
        ("pin-green,pg,p", bpo::value<std::size_t>()->default_value(22), "GPIO line for the green LED")
        ("chip-red,cr", bpo::value<std::string>(), "GPIO chip name for the red LED")
        ("pin-red,pr", bpo::value<std::size_t>(), "GPIO line for the red LED");
    bpo::options_description desc("Available options");
    desc.add(general).add(command);
    bpo::variables_map vm;
    try {
        bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
        bpo::notify(vm);
        if (vm.count("help") || vm.empty()) {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << desc << std::endl;
            return EXIT_SUCCESS;
        }
        if (vm.count("chip-green") && vm["chip-green"].as<std::string>().empty()) {
            throw std::logic_error("GPIO chip name cannot be empty.");
        }
        if (vm.count("chip-red") && vm["chip-red"].as<std::string>().empty()) {
            throw std::logic_error("GPIO chip name cannot be empty.");
        }

        bool greenLedState = false;
        bool redLedState = true;

        /// Create GPIO configuration for the green LED
        std::unique_ptr<iqrf::gpio::GpioConfig> greenLedConfig;
        if (vm.count("chip-green")) {
            greenLedConfig = std::make_unique<iqrf::gpio::GpioConfig>(
                vm["chip-green"].as<std::string>(),
                vm["pin-green"].as<std::size_t>(),
                "green_led"
            );
        } else {
            greenLedConfig = std::make_unique<iqrf::gpio::GpioConfig>(
                vm["pin-green"].as<std::size_t>(),
                "green_led"
            );
        }
        greenLed = std::make_unique<iqrf::gpio::Gpio>(*greenLedConfig);
        greenLed->initOutput(greenLedState);

        /// Create GPIO configuration for the red LED
        if (vm.count("pin-red")) {
            std::unique_ptr<iqrf::gpio::GpioConfig> redLedConfig;
            if (vm.count("chip-red")) {
                redLedConfig = std::make_unique<iqrf::gpio::GpioConfig>(
                    vm["chip-red"].as<std::string>(),
                    vm["pin-red"].as<std::size_t>(),
                    "red_led"
                );
            } else {
                redLedConfig = std::make_unique<iqrf::gpio::GpioConfig>(
                    vm["pin-red"].as<std::size_t>(),
                    "red_led"
                );
            }
            redLed = std::make_unique<iqrf::gpio::Gpio>(*redLedConfig);
            redLed->initOutput(redLedState);
        }

        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        while (true) {
            greenLedState = !greenLedState;
            greenLed->setValue(greenLedState);
            if (redLed) {
                redLedState = !redLedState;
                redLed->setValue(redLedState);
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        gpioCleanup();
        return 1;
    }
}
