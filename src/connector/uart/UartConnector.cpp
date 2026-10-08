/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: UartConnector.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2025-05-10
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include "iqrf/connector/uart/UartConnector.h"

#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>
#include <utility>
#include <thread>

namespace iqrf::connector::uart {

UartConnector::UartConnector(UartConfig config): UartConnector(std::move(config), nullptr) {}

UartConnector::UartConnector(UartConfig config, std::unique_ptr<IUartPort> port):
    busSwitcher(config.busSwitch()),
    config(std::move(config)),
    port(std::move(port)) {
    // Open UART port first to fail before touching GPIOs
    if (!this->port) {
        this->port = createUartPort(UartPortConfig{this->config.device, this->config.baudRate});
    }
    try {
        this->initGpio();
    } catch (...) {
        // Destructor is not called if the constructor throws, so restore the GPIOs here
        this->shutdownGpio();
        throw;
    }
}

UartConnector::~UartConnector() {
    this->stopListen();
    this->shutdownGpio();
}

void UartConnector::shutdownGpio() noexcept {
    if (this->config.powerEnableGpio.has_value() && this->config.disablePowerOnShutdown) {
        ConnectorUtils::runSafely("disable TR power", [this] {
            this->config.powerEnableGpio->setValue(false);
        });
    }
    ConnectorUtils::runSafely("disable UART bus", [this] {
        this->busSwitcher.toggleUart(false);
    });
    if (this->config.pgmSwitchGpio.has_value()) {
        ConnectorUtils::runSafely("release PGM switch", [this] {
            this->config.pgmSwitchGpio->setValue(false);
        });
    }
}

void UartConnector::initGpio() {
    if (this->config.pgmSwitchGpio) {
        this->config.pgmSwitchGpio->initOutput(false);
    }
    if (this->config.powerEnableGpio) {
        this->config.powerEnableGpio->initOutput(true);
    }
    this->busSwitcher.init();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    if (this->config.trModuleReset && this->config.powerEnableGpio.has_value()) {
        this->powerCycleTr();
    }

    this->enableUart();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void UartConnector::powerCycleTr() {
    // Disconnect UART from TR module, otherwise TR module could be powered via UART lines
    this->busSwitcher.toggleUart(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    this->config.powerEnableGpio->setValue(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    this->config.powerEnableGpio->setValue(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void UartConnector::enableUart() {
    this->busSwitcher.toggleUart(true);
    // Discard data received before the UART was connected to TR module (including a partially received frame)
    const std::scoped_lock lock(this->receiveMutex);
    this->port->flushInput();
    this->decoder.reset();
    this->receivedFrames.clear();
}

void UartConnector::resetTr() {
    if (!this->config.powerEnableGpio.has_value()) {
        IQRF_LOG(log::Level::Warning) << "Unable to reset TR module: power enable GPIO is not configured";
        return;
    }
    this->powerCycleTr();
    this->enableUart();
}

std::vector<uint8_t> UartConnector::receive() {
    const std::scoped_lock lock(this->receiveMutex);
    const auto deadline = std::chrono::steady_clock::now() + RECEIVE_TIMEOUT;
    while (this->receivedFrames.empty()) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        if (remaining.count() <= 0) {
            break;
        }
        const std::vector<uint8_t> bytes = this->port->read(READ_SIZE, remaining);
        if (bytes.empty()) {
            break;
        }
        this->decodeReceived(bytes);
    }

    if (this->receivedFrames.empty()) {
        return {};
    }
    std::vector<uint8_t> frame = std::move(this->receivedFrames.front());
    this->receivedFrames.pop_front();
    return frame;
}

void UartConnector::decodeReceived(const std::vector<uint8_t> &bytes) {
    for (const uint8_t byte : bytes) {
        try {
            std::optional<HdlcFrame> frame = this->decoder.decodeByte(byte);
            if (frame.has_value()) {
                this->receivedFrames.push_back(frame->getData());
            }
        } catch (const HdlcFrameError &e) {
            IQRF_LOG(log::Level::Warning) << "Discarding invalid UART frame: " << e.what();
        }
    }
}

void UartConnector::send(const std::vector<uint8_t> &data) {
    if (data.empty()) {
        throw std::runtime_error("No data to send");
    }
    this->port->write(HdlcFrame(data).encode(), WRITE_TIMEOUT);
}

}  // namespace iqrf::connector::uart
