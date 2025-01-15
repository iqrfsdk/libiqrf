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

#include "iqrf/gpio/Gpio.h"

#include <csignal>
#include <iostream>
#include <thread>

/// Green LED configuration
const iqrf::gpio::GpioConfig greenLedConfig(0);

/// Red LED configuration
const iqrf::gpio::GpioConfig redLedConfig(1);

/// Green LED GPIO pin instance
auto *greenLed = new iqrf::gpio::Gpio(greenLedConfig);
/// Red LED GPIO pin instance
auto *redLed = new iqrf::gpio::Gpio(redLedConfig);

/**
 * Signal handler
 * @param signal Signal number
 */
void signalHandler(int signal) {
	std::cout << "Signal " << signal << " received. Exiting..." << std::endl;

	greenLed->setValue(false);
	redLed->setValue(false);

	delete greenLed;
	delete redLed;

	exit(signal);
}

int main() {
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

	return 0;
}
