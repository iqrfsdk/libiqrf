/**
 * Copyright 2023-2026 MICRORISC s.r.o.
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
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <boost/program_options/errors.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>

#include "iqrf/connector/ConnectorUtils.h"
#include "iqrf/connector/IConnector.h"
#include "iqrf/connector/cdc/CdcConnector.h"
#include "iqrf/connector/cdc/CdcProtocol.h"
#include "iqrf/log/Logging.h"

namespace bpo = boost::program_options;
namespace cdc = iqrf::connector::cdc;
using iqrf::connector::ConnectorUtils;

namespace {

/**
 * Response handler, prints the messages received from the TR module
 * @param response Received message
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
 * Parses the packet given as hexadecimal bytes separated by spaces (e.g. "00 00 06 01 ff ff")
 * @param text Packet in hexadecimal
 * @return Packet bytes
 * @throws std::invalid_argument if the text is not a list of hexadecimal bytes
 */
std::vector<uint8_t> parsePacket(const std::string &text) {
    std::vector<uint8_t> packet;
    std::istringstream stream(text);
    std::string token;
    while (stream >> token) {
        uint64_t value = 0;
        std::size_t consumed = 0;
        try {
            value = std::stoul(token, &consumed, 16);
        } catch (const std::logic_error &) {
            consumed = 0;
        }
        if (consumed == 0 || consumed != token.size() || value > 0xFF) {
            throw std::invalid_argument("Invalid packet byte: " + token);
        }
        packet.push_back(static_cast<uint8_t>(value));
    }
    if (packet.empty()) {
        throw std::invalid_argument("Packet is empty");
    }
    return packet;
}

/**
 * Executes the USB device command selected on the command line
 * @param connector CDC connector
 * @param command Name of the command
 * @return true if the command is known, false otherwise
 */
bool executeCommand(cdc::CdcConnector &connector, const std::string &command) {
    if (command == "info") {
        const cdc::UsbDeviceInfo info = connector.readUsbInfo();
        std::cout << "Type: " << info.type << "\nFirmware: " << info.version << "\nSerial: " << info.id << '\n';
    } else if (command == "status") {
        std::cout << "SPI status: 0x" << std::hex << static_cast<int>(connector.readSpiStatus()) << std::dec << '\n';
    } else if (command == "tr-info") {
        const iqrf::connector::TrInfo info = connector.readTrInfo();
        std::cout << info << '\n';
    } else if (command == "indicate") {
        connector.indicateConnectivity();
    } else if (command == "reset") {
        connector.resetUsb();
        std::cout << "USB device is reset in 5 s." << '\n';
    } else if (command == "custom") {
        connector.switchToCustomClass();
        std::cout << "USB class switches to Custom, the device is reset in 5 s." << '\n';
    } else if (command == "cdc-uart") {
        connector.switchToCdcUart();
        std::cout << "USB device switches to CDC-UART, it is reset in 5 s." << '\n';
    } else if (command == "cdc-spi") {
        connector.switchToCdcSpi();
        std::cout << "USB device switches to CDC-SPI, it is reset in 5 s." << '\n';
    } else {
        return false;
    }
    return true;
}

/**
 * Sends the packet to the TR module
 * @param connector CDC connector
 * @param packet Packet to send
 */
void sendPacket(cdc::CdcConnector &connector, const std::vector<uint8_t> &packet) {
    IQRF_LOG(iqrf::log::Level::Info) << "Sending: " << ConnectorUtils::vectorToHexString(packet);
    connector.send(packet);
}

}  // namespace

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

    bpo::options_description connection("Connection options");
    connection.add_options()
        ("device,d", bpo::value<std::string>()->required(), "CDC device name (e.g. /dev/ttyACM0)")
        ("command,c", bpo::value<std::string>(),
            "USB device command: info, status, indicate, reset, custom, cdc-uart, cdc-spi, tr-info");

    bpo::options_description packets("Packet options");
    packets.add_options()
        ("packet,p", bpo::value<std::string>(),
            "send the packet in hexadecimal (e.g. \"00 00 06 01 ff ff\") once and print the responses")
        ("send-led", "send the LED toggle packet every second, the same as the SPI example");

    bpo::options_description allOptions;
    allOptions.add(general).add(connection).add(packets);

    bpo::variables_map vm;
    try {
        bpo::store(bpo::parse_command_line(argc, argv, allOptions), vm);
        if (vm.count("help") > 0) {
            std::cout << allOptions << '\n';
            return 0;
        }
        bpo::notify(vm);
        if (vm.count("command") == 0 && vm.count("packet") == 0 && vm.count("send-led") == 0) {
            throw bpo::error("one of --command, --packet or --send-led is required");
        }
    } catch (const bpo::error &e) {
        std::cerr << "Error: " << e.what() << '\n' << allOptions << '\n';
        return 1;
    }

    try {
        // Parse the packet first, the device is not opened when the packet is invalid
        const std::vector<uint8_t> packet = vm.count("packet") > 0 ? parsePacket(vm["packet"].as<std::string>())
            : std::vector<uint8_t>{};
        cdc::CdcConnector connector(vm["device"].as<std::string>());
        if (vm.count("command") > 0) {
            if (!executeCommand(connector, vm["command"].as<std::string>())) {
                std::cerr << "Unknown command" << '\n' << allOptions << '\n';
                return 1;
            }
            return 0;
        }

        connector.registerResponseHandler(responseHandler, iqrf::connector::AccessType::Normal);
        connector.listen();
        if (vm.count("packet") > 0) {
            sendPacket(connector, packet);
            // Responses of the TR module are asynchronous, wait for them
            std::this_thread::sleep_for(std::chrono::seconds(5));
        } else {
            // The same packet as in the SPI example: LED toggled every second
            bool ledState = true;
            while (true) {
                sendPacket(connector, {0x00, 0x00, 0x06, static_cast<uint8_t>(ledState), 0xff, 0xff});
                ledState = !ledState;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        connector.stopListen();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
