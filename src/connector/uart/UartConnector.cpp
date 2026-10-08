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

#include <array>
#include <chrono>
#include <optional>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
#include <thread>

namespace iqrf::connector::uart {

UartConnector::UartConnector(UartConfig config): busSwitcher(config.busSwitch()), config(std::move(config)) {
    // Destructor is not called if the constructor throws, so release the resources here
    // Open UART port first to fail before touching GPIOs
    try {
        this->openPort();
    } catch (...) {
        this->closePort();
        throw;
    }
    try {
        this->initGpio();
    } catch (...) {
        this->shutdownGpio();
        this->closePort();
        throw;
    }
}

void UartConnector::openPort() {
    IQRF_LOG(log::Level::Debug) << "Opening UART port: " << this->config.device;
    UartConnector::checkSerialResult(sp_get_port_by_name(this->config.device.c_str(), &this->port));
    const char *name = sp_get_port_name(this->port);
    const char *description = sp_get_port_description(this->port);
    IQRF_LOG(log::Level::Debug) << "UART port created: " << this->config.device
        << " (name: " << (name != nullptr ? name : "N/A")
        << ", description: " << (description != nullptr ? description : "N/A") << ")";
    if (sp_get_port_transport(this->port) == SP_TRANSPORT_USB) {
        std::stringstream usbInfo;
        int usbBus = 0;
        int usbAddress = 0;
        if (sp_get_port_usb_bus_address(this->port, &usbBus, &usbAddress) == SP_OK) {
            usbInfo << "USB bus: " << usbBus << ", USB address: " << usbAddress;
        } else {
            usbInfo << "USB bus and address not available";
        }
        int usbVid = 0;
        int usbPid = 0;
        if (sp_get_port_usb_vid_pid(this->port, &usbVid, &usbPid) == SP_OK) {
            usbInfo << ", USB VID: " << std::hex << usbVid << ", PID: " << usbPid;
        } else {
            usbInfo << ", USB VID and PID not available";
        }
        const char *manufacturer = sp_get_port_usb_manufacturer(this->port);
        usbInfo << ", Manufacturer: " << (manufacturer != nullptr ? manufacturer : "N/A");
        const char *product = sp_get_port_usb_product(this->port);
        usbInfo << ", Product: " << (product != nullptr ? product : "N/A");
        const char *serial = sp_get_port_usb_serial(this->port);
        usbInfo << ", Serial: " << (serial != nullptr ? serial : "N/A");

        IQRF_LOG(log::Level::Debug) << usbInfo.str();
    }

    // Open the port
    UartConnector::checkSerialResult(sp_open(this->port, SP_MODE_READ_WRITE));

    // Set up the port
    UartConnector::checkSerialResult(sp_set_baudrate(this->port, static_cast<int>(this->config.baudRate)));
    UartConnector::checkSerialResult(sp_set_bits(this->port, 8));
    UartConnector::checkSerialResult(sp_set_parity(this->port, SP_PARITY_NONE));
    UartConnector::checkSerialResult(sp_set_stopbits(this->port, 1));
    UartConnector::checkSerialResult(sp_set_flowcontrol(this->port, SP_FLOWCONTROL_NONE));
}

UartConnector::~UartConnector() {
    this->stopListen();
    this->shutdownGpio();
    this->closePort();
}

void UartConnector::closePort() noexcept {
    if (this->port != nullptr) {
        // Closing a port which is not open fails, the result is irrelevant here
        static_cast<void>(sp_close(this->port));
        sp_free_port(this->port);
        this->port = nullptr;
    }
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
    UartConnector::checkSerialResult(sp_flush(this->port, SP_BUF_INPUT));
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

int UartConnector::checkSerialResult(const sp_return result) {
    switch (result) {
        case SP_ERR_ARG:
            throw std::runtime_error("Invalid argument");
        case SP_ERR_FAIL: {
            char *message = sp_last_error_message();
            const std::string errorMessage = message != nullptr ? message : "unknown error";
            sp_free_error_message(message);
            throw std::runtime_error("Failed: " + errorMessage);
        }
        case SP_ERR_MEM:
            throw std::runtime_error("Memory allocation error");
        case SP_ERR_SUPP:
            throw std::runtime_error("Operation not supported");
        default:
            return result;
    }
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
        std::array<uint8_t, 64> buffer{};
        // Returns as soon as any data is available
        const int count = UartConnector::checkSerialResult(sp_blocking_read_next(
            this->port, buffer.data(), buffer.size(), static_cast<unsigned int>(remaining.count())));
        if (count == 0) {
            break;
        }
        this->decodeReceived(buffer.data(), static_cast<std::size_t>(count));
    }

    if (this->receivedFrames.empty()) {
        return {};
    }
    std::vector<uint8_t> frame = std::move(this->receivedFrames.front());
    this->receivedFrames.pop_front();
    return frame;
}

void UartConnector::decodeReceived(const uint8_t *bytes, const std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        try {
            std::optional<HdlcFrame> frame = this->decoder.decodeByte(bytes[i]);
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
    const std::vector<uint8_t> frame = HdlcFrame(data).encode();
    const int written = UartConnector::checkSerialResult(sp_blocking_write(
        this->port, frame.data(), frame.size(), static_cast<unsigned int>(WRITE_TIMEOUT.count())));
    if (static_cast<std::size_t>(written) != frame.size()) {
        throw std::runtime_error("Timeout while writing to UART port");
    }
}

}  // namespace iqrf::connector::uart
