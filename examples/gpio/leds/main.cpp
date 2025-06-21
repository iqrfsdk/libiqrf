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

#include "iqrf/gpio/Gpio.h"

/// Green LED GPIO pin instance
iqrf::gpio::Gpio *greenLed = nullptr;
/// Red LED GPIO pin instance
iqrf::gpio::Gpio *redLed = nullptr;

void gpioCleanup() {
    if (greenLed != nullptr) {
        greenLed->setValue(false);
        delete greenLed;
        greenLed = nullptr;
    }
    if (redLed != nullptr) {
        redLed->setValue(false);
        delete redLed;
        redLed = nullptr;
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

int main() {
    try {
        /// Green LED configuration
        const iqrf::gpio::GpioConfig greenLedConfig(0, "green_led");
        greenLed = new iqrf::gpio::Gpio(greenLedConfig);
        /// Red LED configuration
        const iqrf::gpio::GpioConfig redLedConfig(1, "red_led");
        redLed = new iqrf::gpio::Gpio(redLedConfig);

        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        bool greenLedState = false;
        bool redLedState = true;

        greenLed->initOutput(greenLedState);
        redLed->initOutput(redLedState);

        while (true) {
            greenLedState = !greenLedState;
            redLedState = !redLedState;
            greenLed->setValue(greenLedState);
            redLed->setValue(redLedState);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        gpioCleanup();
        return 1;
    }
}
