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
#include <optional>
#include <string>
#include <vector>

#include "iqrf/connector/spi/SpiConnector.h"
#include "iqrf/connector/ConnectorUtils.h"
#include "iqrf/gpio/Gpio.h"
#include "iqrf/log/Logging.h"

using iqrf::connector::ConnectorUtils;

/// IQRF SPI connector configuration
const iqrf::connector::spi::SpiConfig spiConfig(
    "/dev/spidev0.0",
    // Power enable GPIO
    iqrf::gpio::Gpio(iqrf::gpio::GpioConfig(23, "iqrf_spi_power_enable")),
    // Bus enable GPIO
    std::nullopt,
    // SPI enable GPIO
    std::nullopt,
    // PGM switch GPIO
    std::nullopt,
    // UART enable GPIO
    std::nullopt,
    // I2C enable GPIO
    std::nullopt,
    // TR module reset during initialization
    true
);
/// IQRF SPI connector instance
iqrf::connector::spi::SpiConnector *spiConnector = nullptr;

/**
 * Signal handler
 * @param signal Signal number
 */
void signalHandler(const int signal) {
    std::cout << "Signal " << signal << " received. Exiting..." << std::endl;

    delete spiConnector;

    exit(signal);
}

/**
 * Identifies connected TR module
 */
void identifyConnectedTrModule() {
    try {
        IQRF_LOG(iqrf::log::Level::Info) << "Identifying connected TR module...";
        IQRF_LOG(iqrf::log::Level::Debug) << "Entering programming mode...";
        spiConnector->enterProgrammingMode();
        IQRF_LOG(iqrf::log::Level::Debug) << "Reading TR module identification...";
        spiConnector->readTrInfo();
        IQRF_LOG(iqrf::log::Level::Debug) << "Terminating programming mode...";
    } catch (std::exception &e) {
        IQRF_LOG(iqrf::log::Level::Error) << "Failed to identify TR module: " << e.what();
        return;
    }
}

/**
 * DPA response handler
 * @param response Response data vector
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
 * @return Exit code
 */
int main() {
    iqrf::log::Logger::logLevel = iqrf::log::Level::Trace;
    iqrf::log::Logger logger;
    IQRF_LOG(iqrf::log::Level::Info) << "IQRF SPI Connector Example";
    spiConnector = new iqrf::connector::spi::SpiConnector(spiConfig);
    identifyConnectedTrModule();
    spiConnector->registerResponseHandler(responseHandler, iqrf::connector::AccessType::Normal);
    spiConnector->listen();

    bool ledState = true;
    while (true) {
        std::vector<uint8_t> request = {0x00, 0x00, 0x06, static_cast<uint8_t>(ledState), 0xff, 0xff};
        ledState = !ledState;
        IQRF_LOG(iqrf::log::Level::Info) << "Sending: " << ConnectorUtils::vectorToHexString(request);
        spiConnector->send(request);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    return 0;
}
