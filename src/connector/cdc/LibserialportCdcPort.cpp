/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: LibserialportCdcPort.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-12
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */
#include "iqrf/connector/cdc/LibserialportCdcPort.h"

#include <libserialport.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "iqrf/connector/cdc/ICdcPort.h"
#include "iqrf/log/Logging.h"

namespace iqrf::connector::cdc {

namespace log = ::iqrf::log;

namespace {

/// Baud rate of the device (ignored by USB CDC, kept for compatibility with the TR communication settings)
constexpr int BAUD_RATE = 57600;

/// Time to wait after opening the port, the device needs it to settle
constexpr std::chrono::seconds SETTLE_TIME{2};

}  // namespace

std::unique_ptr<ICdcPort> createCdcPort(const std::string &device) {
    return std::make_unique<LibserialportCdcPort>(device);
}

LibserialportCdcPort::LibserialportCdcPort(const std::string &device) {
    try {
        this->open(device);
    } catch (...) {
        // Destructor is not called if the constructor throws, so release the port here
        this->close();
        throw;
    }
}

LibserialportCdcPort::~LibserialportCdcPort() {
    this->close();
}

void LibserialportCdcPort::open(const std::string &device) {
    IQRF_LOG(log::Level::Debug) << "Opening CDC port: " << device;
    try {
        LibserialportCdcPort::checkResult(sp_get_port_by_name(device.c_str(), &this->port));
    } catch (const std::runtime_error &e) {
        throw std::runtime_error("Cannot find CDC device " + device + ": " + e.what());
    }
    this->logPortInfo();
    LibserialportCdcPort::checkResult(sp_open(this->port, SP_MODE_READ_WRITE));
    LibserialportCdcPort::checkResult(sp_set_baudrate(this->port, BAUD_RATE));
    LibserialportCdcPort::checkResult(sp_set_bits(this->port, 8));
    LibserialportCdcPort::checkResult(sp_set_parity(this->port, SP_PARITY_NONE));
    LibserialportCdcPort::checkResult(sp_set_stopbits(this->port, 1));
    LibserialportCdcPort::checkResult(sp_set_flowcontrol(this->port, SP_FLOWCONTROL_NONE));
    std::this_thread::sleep_for(SETTLE_TIME);
    LibserialportCdcPort::checkResult(sp_flush(this->port, SP_BUF_INPUT));
}

void LibserialportCdcPort::close() noexcept {
    if (this->port != nullptr) {
        // Closing a port which is not open fails, the result is irrelevant here
        static_cast<void>(sp_close(this->port));
        sp_free_port(this->port);
        this->port = nullptr;
    }
}

void LibserialportCdcPort::logPortInfo() const {
    const char *name = sp_get_port_name(this->port);
    const char *description = sp_get_port_description(this->port);
    IQRF_LOG(log::Level::Debug) << "CDC port created: " << (name != nullptr ? name : "N/A")
        << " (description: " << (description != nullptr ? description : "N/A") << ")";
    if (sp_get_port_transport(this->port) != SP_TRANSPORT_USB) {
        return;
    }
    std::stringstream usbInfo;
    int usbVid = 0;
    int usbPid = 0;
    if (sp_get_port_usb_vid_pid(this->port, &usbVid, &usbPid) == SP_OK) {
        usbInfo << "USB VID: " << std::hex << usbVid << ", PID: " << usbPid;
    } else {
        usbInfo << "USB VID and PID not available";
    }
    const char *manufacturer = sp_get_port_usb_manufacturer(this->port);
    usbInfo << ", Manufacturer: " << (manufacturer != nullptr ? manufacturer : "N/A");
    const char *product = sp_get_port_usb_product(this->port);
    usbInfo << ", Product: " << (product != nullptr ? product : "N/A");
    const char *serial = sp_get_port_usb_serial(this->port);
    usbInfo << ", Serial: " << (serial != nullptr ? serial : "N/A");
    IQRF_LOG(log::Level::Debug) << usbInfo.str();
}

std::vector<uint8_t> LibserialportCdcPort::read(const std::size_t maxSize, const std::chrono::milliseconds timeout) {
    std::vector<uint8_t> buffer(maxSize);
    // Timeout 0 means waiting indefinitely in libserialport
    const auto timeoutMs = static_cast<unsigned int>(std::max<std::chrono::milliseconds::rep>(timeout.count(), 1));
    const int count = LibserialportCdcPort::checkResult(
        sp_blocking_read_next(this->port, buffer.data(), buffer.size(), timeoutMs));
    buffer.resize(static_cast<std::size_t>(count));
    return buffer;
}

void LibserialportCdcPort::write(const std::vector<uint8_t> &data, const std::chrono::milliseconds timeout) {
    const auto timeoutMs = static_cast<unsigned int>(std::max<std::chrono::milliseconds::rep>(timeout.count(), 1));
    const int written = LibserialportCdcPort::checkResult(
        sp_blocking_write(this->port, data.data(), data.size(), timeoutMs));
    if (static_cast<std::size_t>(written) != data.size()) {
        throw std::runtime_error("Timeout while writing to CDC port");
    }
}

void LibserialportCdcPort::flushInput() {
    LibserialportCdcPort::checkResult(sp_flush(this->port, SP_BUF_INPUT));
}

int LibserialportCdcPort::checkResult(const sp_return result) {
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

}  // namespace iqrf::connector::cdc
