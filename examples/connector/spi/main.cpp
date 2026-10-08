/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: main.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-05
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <boost/program_options.hpp>

#include "iqrf/connector/spi/SpiConnector.h"
#include "iqrf/connector/ConnectorUtils.h"
#include "iqrf/gpio/Gpio.h"
#include "iqrf/log/Logging.h"

namespace bpo = boost::program_options;
using iqrf::connector::ConnectorUtils;
using iqrf::gpio::Gpio;
using iqrf::gpio::GpioConfig;

namespace {

/// IQRF SPI connector instance
std::unique_ptr<iqrf::connector::spi::SpiConnector> spiConnector = nullptr;

/**
 * Signal handler
 * @param signal Signal number
 */
void signalHandler(const int signal) {
    std::cout << "Signal " << signal << " received. Exiting..." << '\n';
    exit(signal);
}

/**
 * Response handler for the IQRF SPI connector
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
 * Prints TR module information
 * @param trInfo TR module information
 */
void printTrInfo(const iqrf::connector::TrInfo &trInfo) {
    std::cout << "TR module information:" << '\n'
        << "  Module type:      " << trInfo.trTypeString() << '\n'
        << "  MCU type:         " << trInfo.mcuTypeString() << '\n'
        << "  Module ID:        " << trInfo.midString() << '\n'
        << "  IQRF OS version:  " << trInfo.osVersionString() << '\n'
        << "  IQRF OS build:    " << trInfo.osBuildString() << '\n'
        << "  FCC certified:    " << (trInfo.isFccCertified() ? "yes" : "no") << '\n'
        << "  Shared MCU pins:  " << (trInfo.hasSharedMcuPins() ? "yes" : "no") << '\n'
        << "  IBK:              " << (trInfo.ibk.has_value() ? trInfo.ibkString() : "not available") << '\n';
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
    bpo::options_description command("Command options");
    command.add_options()
        ("device,d", bpo::value<std::string>(), "SPI device name (e.g. /dev/spidev0.0)")
        ("low-speed", "use low speed communication mode")
        ("power-gpio", bpo::value<int64_t>(), "TR power enable GPIO pin number (required for TR module identification)")
        ("bus-gpio", bpo::value<int64_t>(), "bus enable GPIO pin number")
        ("pgm-gpio", bpo::value<int64_t>(), "PGM switch GPIO pin number (required for TR module identification)")
        ("spi-gpio", bpo::value<int64_t>(), "SPI enable GPIO pin number")
        ("uart-gpio", bpo::value<int64_t>(), "UART enable GPIO pin number")
        ("i2c-gpio", bpo::value<int64_t>(), "I2C enable GPIO pin number");
    bpo::options_description desc("Available options");
    desc.add(general).add(command);
    bpo::variables_map vm;
    try {
        bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
        bpo::notify(vm);
        if (vm.count("help") > 0 || vm.empty()) {
            std::cout << "Usage: " << argv[0] << " [options]" << '\n';
            std::cout << desc << '\n';
            return EXIT_SUCCESS;
        }

        static_cast<void>(signal(SIGINT, signalHandler));
        static_cast<void>(signal(SIGTERM, signalHandler));

        iqrf::log::Logger::logLevel = iqrf::log::Level::Trace;
        const iqrf::log::Logger logger;
        IQRF_LOG(iqrf::log::Level::Info) << "IQRF SPI Connector Example";

        if (vm.count("device") == 0) {
            throw std::logic_error("SPI device name is required.");
        }

        /// IQRF SPI connector configuration
        iqrf::connector::spi::SpiConfig spiConfig(
            vm["device"].as<std::string>(),
            gpioOption(vm, "power-gpio"),
            gpioOption(vm, "bus-gpio"),
            gpioOption(vm, "pgm-gpio"),
            gpioOption(vm, "spi-gpio"),
            gpioOption(vm, "uart-gpio"),
            gpioOption(vm, "i2c-gpio"),
            true,
            true
        );
        if (vm.count("low-speed") > 0) {
            spiConfig.communicationMode = iqrf::connector::spi::SpiCommunicationMode::LowSpeed;
        }
        spiConnector = std::make_unique<iqrf::connector::spi::SpiConnector>(spiConfig);

        // TR module information is available regardless of the TR module application in programming mode,
        // TR module is switched to programming mode by PGM pin active during power up
        if (spiConfig.powerEnableGpio.has_value() && spiConfig.pgmSwitchGpio.has_value()) {
            spiConnector->enterProgrammingMode();
            try {
                const iqrf::connector::TrInfo trInfo = spiConnector->readTrInfo();
                printTrInfo(trInfo);
            } catch (...) {
                spiConnector->exitProgrammingMode();
                throw;
            }
            spiConnector->exitProgrammingMode();
        } else {
            IQRF_LOG(iqrf::log::Level::Info)
                << "TR module identification skipped (power enable and PGM switch GPIOs are not specified)";
        }

        spiConnector->registerResponseHandler(responseHandler, iqrf::connector::AccessType::Normal);
        spiConnector->listen();

        bool ledState = true;
        while (true) {
            const std::vector<uint8_t> request = {0x00, 0x00, 0x06, static_cast<uint8_t>(ledState), 0xff, 0xff};
            ledState = !ledState;
            IQRF_LOG(iqrf::log::Level::Info) << "Sending: " << ConnectorUtils::vectorToHexString(request);
            spiConnector->send(request);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        return EXIT_SUCCESS;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
