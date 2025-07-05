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

/// Button GPIO pin instance
std::unique_ptr<iqrf::gpio::Gpio> button;

/**
 * Signal handler
 * @param signal Signal number
 */
void signalHandler(const int signal) {
    std::cout << "Signal " << signal << " received. Exiting..." << std::endl;
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
        ("chip,c", bpo::value<std::string>(), "GPIO chip name for the button")
        ("pin,p", bpo::value<std::size_t>()->default_value(2), "GPIO line for the button")
        ("inverted,i", bpo::bool_switch()->default_value(false), "Invert button state (pressed = 1, released = 0)");
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
        if (vm.count("chip") && vm["chip"].as<std::string>().empty()) {
            throw std::logic_error("GPIO chip name cannot be empty.");
        }

        // Create GPIO configuration for the button
        size_t buttonPin = vm["pin"].as<std::size_t>();
        std::unique_ptr<iqrf::gpio::GpioConfig> buttonConfig;
        if (vm.count("chip")) {
            buttonConfig = std::make_unique<iqrf::gpio::GpioConfig>(vm["chip"].as<std::string>(), buttonPin);
        } else {
            buttonConfig = std::make_unique<iqrf::gpio::GpioConfig>(buttonPin);
        }
        button = std::make_unique<iqrf::gpio::Gpio>(*buttonConfig);
        button->initInput();
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        uint64_t counter = 0;
        bool inverted = vm["inverted"].as<bool>();

        while (true) {
            if (
                (button->getValue() && !inverted) ||
                (!button->getValue() && inverted)
            ) {
                if (counter > 0) {
                    std::cout << "Button has been released. Button was pressed for "
                              << std::to_string(static_cast<double>(counter) / 100.0)
                              << " s."
                              << std::endl;
                }
                counter = 0;
            } else {
                if (counter == 0) {
                    std::cout << "Button has been pressed." << std::endl;
                }
                counter++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return EXIT_SUCCESS;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
