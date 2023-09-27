/**
 * Copyright 2023 MICRORISC s.r.o.
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
#include <thread>

#include "iqrf/gpio/Gpio.h"

/// Button configuration
const iqrf::gpio::GpioConfig buttonConfig = {
		.driver = iqrf::gpio::GpioDriver::sysfs,
		.config = {
				.sysfs = {
						.pin = 2,
				},
		},
};

/// Button GPIO pin instance
auto *button = new iqrf::gpio::Gpio(buttonConfig);

/**
 * Signal handler
 * @param signal Signal number
 */
void signalHandler(int signal) {
    std::cout << "Signal " << signal << " received. Exiting..." << std::endl;

    button->destroy();
    delete button;

    exit(signal);
}

int main() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
	button->initInput();
	bool buttonState;
	uint64_t counter = 0;

	while (true) {
		buttonState = button->getValue();

		if (buttonState) {
            if (counter > 0) {
                std::cout << "Button has been released. Button was pressed for " << std::to_string(static_cast<double>(counter) / 10.0) << " s." << std::endl;
            }
			counter = 0;
		} else {
			if (counter == 0) {
				std::cout << "Button has been pressed." << std::endl;
			}
			counter++;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

    return 0;
}
