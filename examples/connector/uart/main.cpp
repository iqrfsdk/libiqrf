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

#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include "iqrf/connector/uart/UartConnector.h"
#include "iqrf/connector/ConnectorUtils.h"
#include "iqrf/gpio/Gpio.h"
#include "iqrf/log/Logging.h"

namespace bpo = boost::program_options;
using iqrf::connector::ConnectorUtils;
using iqrf::gpio::Gpio;
using iqrf::gpio::GpioConfig;

/// IQRF UART connector instance
std::unique_ptr<iqrf::connector::uart::UartConnector> uartConnector = nullptr;

/**
 * Signal handler
 * @param signal Signal number
 */
void signalHandler(const int signal) {
    std::cout << "Signal " << signal << " received. Exiting..." << std::endl;
    exit(signal);
}

/**
 * Response handler for the IQRF UART connector
 * @param response Response received from the IQRF module
 * @return 0 on success, -1 on error
 */
int responseHandler(const std::vector<uint8_t> &response) {
    if (response.empty()) {
        IQRF_LOG(iqrf::log::Level::Error) << "Empty response received.";
        return -1;
    }
    IQRF_LOG(iqrf::log::Level::Info) << "Response: " << ConnectorUtils::vectorToHexString(response);
    return 0;
}

/**
 * Creates optional GPIO from command line option
 * @param vm Parsed command line options
 * @param name Option name
 * @return GPIO if the option is set
 */
std::optional<Gpio> gpioOption(const bpo::variables_map &vm, const std::string &name) {
    if (vm.count(name) == 0) {
        return std::nullopt;
    }
    return Gpio(GpioConfig(vm[name].as<int64_t>()));
}

/**
 * Checks whether the message is DPA startup notification
 *
 * DPA coordinator sends asynchronous peripheral enumeration (PNUM 0xFF, PCMD 0x3F without the response bit)
 * when it is ready after the startup.
 *
 * @param message Received message
 * @return true if the message is DPA startup notification, false otherwise
 */
bool isStartupNotification(const std::vector<uint8_t> &message) {
    // NADR (2 B), PNUM, PCMD, HWPID (2 B), ErrN, DpaValue
    return message.size() >= 8 && message[2] == 0xFF && message[3] == 0x3F;
}

/**
 * Waits for DPA startup notification, other received messages are logged
 * @param connector IQRF UART connector
 * @param timeout Maximum time to wait
 * @return true if the notification has been received, false on timeout
 */
bool awaitStartupNotification(iqrf::connector::uart::UartConnector &connector, const std::chrono::seconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        const std::vector<uint8_t> message = connector.receive();
        if (message.empty()) {
            continue;
        }
        IQRF_LOG(iqrf::log::Level::Info) << "Received: " << ConnectorUtils::vectorToHexString(message);
        if (isStartupNotification(message)) {
            return true;
        }
    }
    return false;
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
        ("device,d", bpo::value<std::string>(), "UART device name")
        ("baudrate,b", bpo::value<uint32_t>()->default_value(57600), "UART baud rate (default: 57600)")
        ("power-gpio", bpo::value<int64_t>(), "TR power enable GPIO pin number")
        ("bus-gpio", bpo::value<int64_t>(), "bus enable GPIO pin number")
        ("pgm-gpio", bpo::value<int64_t>(), "PGM switch GPIO pin number")
        ("spi-gpio", bpo::value<int64_t>(), "SPI enable GPIO pin number")
        ("uart-gpio", bpo::value<int64_t>(), "UART enable GPIO pin number")
        ("i2c-gpio", bpo::value<int64_t>(), "I2C enable GPIO pin number");
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

        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        iqrf::log::Logger::logLevel = iqrf::log::Level::Trace;
        iqrf::log::Logger logger;
        IQRF_LOG(iqrf::log::Level::Info) << "IQRF UART Connector Example";

        if (!vm.count("device")) {
            throw std::logic_error("UART device name is required.");
        }

        /// IQRF UART connector configuration
        const iqrf::connector::uart::UartConfig uartConfig(
            vm["device"].as<std::string>(),
            vm["baudrate"].as<uint32_t>(),
            gpioOption(vm, "power-gpio"),
            gpioOption(vm, "bus-gpio"),
            gpioOption(vm, "pgm-gpio"),
            gpioOption(vm, "spi-gpio"),
            gpioOption(vm, "uart-gpio"),
            gpioOption(vm, "i2c-gpio"),
            true,
            true
        );
        uartConnector = std::make_unique<iqrf::connector::uart::UartConnector>(uartConfig);
        // TR module is reset only if its power can be controlled, DPA is not ready until it sends the notification
        if (uartConfig.powerEnableGpio.has_value()) {
            IQRF_LOG(iqrf::log::Level::Info) << "Waiting for DPA startup notification...";
            if (awaitStartupNotification(*uartConnector, std::chrono::seconds(5))) {
                IQRF_LOG(iqrf::log::Level::Info) << "DPA is ready.";
            } else {
                IQRF_LOG(iqrf::log::Level::Warning) << "DPA startup notification has not been received.";
            }
        }
        uartConnector->registerResponseHandler(responseHandler, iqrf::connector::AccessType::Normal);
        uartConnector->listen();

        bool ledState = true;
        while (true) {
            std::vector<uint8_t> request = {0x00, 0x00, 0x06, static_cast<uint8_t>(ledState), 0xff, 0xff};
            ledState = !ledState;
            IQRF_LOG(iqrf::log::Level::Info) << "Sending: " << ConnectorUtils::vectorToHexString(request);
            uartConnector->send(request);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        return EXIT_SUCCESS;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
