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

#include "iqrf/connector/uart/UartConnector.h"
#include "iqrf/log/Logging.h"

/// IQRF UART connector configuration
const iqrf::connector::uart::UartConfig uartConfig("/dev/ttyACM0", 57600);
/// IQRF UART connector instance
iqrf::connector::uart::UartConnector *uartConnector = nullptr;

/**
 * Signal handler
 * @param signal Signal number
 */
void signalHandler(const int signal) {
    std::cout << "Signal " << signal << " received. Exiting..." << std::endl;

    delete uartConnector;

    exit(signal);
}

std::string vectorToHex(const std::vector<uint8_t>& input) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');

    for (size_t i = 0; i < input.size(); ++i) {
        ss << std::setw(2) << static_cast<int>(input[i]);
        if (i + 1 < input.size()) {
            ss << ".";
        }
    }

    return ss.str();
}

int main() {
    iqrf::log::Logger::logLevel = iqrf::log::Level::Trace;
    iqrf::log::Logger logger;
    IQRF_LOG(iqrf::log::Level::Info) << "IQRF UART Connector Example";
    uartConnector = new iqrf::connector::uart::UartConnector(uartConfig);

    bool ledState = true;
    while (true) {
        std::vector<uint8_t> request = {0x00, 0x00, 0x06, static_cast<uint8_t>(ledState), 0xff, 0xff};
        ledState = !ledState;
        IQRF_LOG(iqrf::log::Level::Info) << "Sending: " << vectorToHex(request);
        uartConnector->send(request);
        std::vector<uint8_t> receivedData = uartConnector->receive();
        if (receivedData.empty()) {
            IQRF_LOG(iqrf::log::Level::Warning) << "No data received.";
            continue;
        }
        IQRF_LOG(iqrf::log::Level::Info) << "Received: " << vectorToHex(receivedData);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    return 0;
}
