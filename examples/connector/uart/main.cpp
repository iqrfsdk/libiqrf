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
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include "iqrf/connector/uart/UartConnector.h"
#include "iqrf/connector/ConnectorUtils.h"
#include "iqrf/log/Logging.h"

namespace bpo = boost::program_options;
using iqrf::connector::ConnectorUtils;

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
        ("power,p", bpo::value<int64_t>()->default_value(23))
        ("i2c,i", bpo::value<int64_t>()->default_value(18))
        ("spi,s", bpo::value<int64_t>()->default_value(7))
        ("uart,u", bpo::value<int64_t>()->default_value(6));
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

        iqrf::gpio::Gpio powerPin(iqrf::gpio::GpioConfig(vm["power"].as<int64_t>()));
        iqrf::gpio::Gpio i2cPin(iqrf::gpio::GpioConfig(vm["i2c"].as<int64_t>()));
        iqrf::gpio::Gpio spiPin(iqrf::gpio::GpioConfig(vm["spi"].as<int64_t>()));
        iqrf::gpio::Gpio uartPin(iqrf::gpio::GpioConfig(vm["uart"].as<int64_t>()));

        /// IQRF UART connector configuration
        const iqrf::connector::uart::UartConfig uartConfig(
            vm["device"].as<std::string>(),
            vm["baudrate"].as<uint32_t>(),
            powerPin,
            std::nullopt,
            std::nullopt,
            spiPin,
            uartPin,
            i2cPin,
            true,
            false
        );
        uartConnector = std::make_unique<iqrf::connector::uart::UartConnector>(uartConfig);
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
