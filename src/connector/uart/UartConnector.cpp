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

namespace iqrf::connector::uart {

UartConnector::UartConnector(UartConfig config): IConnector(), config(std::move(config)) {
    this->initGpio();
    IQRF_LOG(log::Level::Debug) << "Opening UART port: " << this->config.device;
    UartConnector::checkSerialResult(sp_get_port_by_name(this->config.device.c_str(), &this->port));
    IQRF_LOG(log::Level::Debug) << "UART port created: " << this->config.device
        << " (name: " << sp_get_port_name(this->port) << ", description: "
        << sp_get_port_description(this->port) << ")";
    if (sp_get_port_transport(this->port) == SP_TRANSPORT_USB) {
        std::stringstream usbInfo;
        int usbBus, usbAddress;
        if (sp_get_port_usb_bus_address(this->port, &usbBus, &usbAddress) == SP_OK) {
            usbInfo << "USB bus: " << usbBus << ", USB address: " << usbAddress;
        } else {
            usbInfo << "USB bus and address not available";
        }
        int usbVid, usbPid;
        if (sp_get_port_usb_vid_pid(this->port, &usbVid, &usbPid) == SP_OK) {
            usbInfo << ", USB VID: " << std::hex << usbVid << ", PID: " << usbPid;
        } else {
            usbInfo << ", USB VID and PID not available";
        }
        char *manufacturer = sp_get_port_usb_manufacturer(this->port);
        usbInfo << ", Manufacturer: " << (manufacturer != nullptr ? manufacturer : "N/A");
        char *product = sp_get_port_usb_product(this->port);
        usbInfo << ", Product: " << (product != nullptr ? product : "N/A");
        char *serial = sp_get_port_usb_serial(this->port);
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

    if (this->config.powerEnableGpio) {
        this->config.powerEnableGpio->setValue(false);
    }

    this->toggleBus(false);

    if (this->config.pgmSwitchGpio) {
        this->config.pgmSwitchGpio->setValue(false);
    }

    if (this->port) {
        sp_close(this->port);
        sp_free_port(this->port);
    }
}

void UartConnector::initGpio() {
    if (this->config.pgmSwitchGpio) {
        this->config.pgmSwitchGpio->initOutput(false);
    }
    if (this->config.powerEnableGpio) {
        this->config.powerEnableGpio->initOutput(true);
    }
    if (this->config.busEnableGpio) {
        this->config.busEnableGpio->initOutput(false);
    } else {
        if (this->config.uartEnableGpio) {
            this->config.uartEnableGpio->initOutput(false);
        }
        if (this->config.spiEnableGpio) {
            this->config.spiEnableGpio->initOutput(false);
        }
        if (this->config.i2cEnableGpio) {
            this->config.i2cEnableGpio->initOutput(false);
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    if (this->config.trModuleReset) {
        this->resetTr();
    }

    if (this->config.busEnableGpio) {
        this->config.busEnableGpio->setValue(true);
    } else {
        if (this->config.uartEnableGpio) {
            this->config.uartEnableGpio->setValue(true);
        }
        if (this->config.spiEnableGpio) {
            this->config.spiEnableGpio->setValue(false);
        }
        if (this->config.i2cEnableGpio) {
            this->config.i2cEnableGpio->setValue(true);
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void UartConnector::resetTr() {
    if (!this->config.powerEnableGpio.has_value()) {
        return;
    }
    this->config.powerEnableGpio->setValue(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    this->config.powerEnableGpio->setValue(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void UartConnector::toggleBus(bool enable) {
    if (this->config.busEnableGpio) {
        this->config.busEnableGpio->setValue(enable);
    } else {
        if (this->config.i2cEnableGpio) {
            this->config.i2cEnableGpio->setValue(false);
        }
        if (this->config.spiEnableGpio) {
            this->config.spiEnableGpio->setValue(false);
        }
        if (this->config.uartEnableGpio) {
            this->config.uartEnableGpio->setValue(enable);
        }
    }
}

int UartConnector::checkSerialResult(sp_return result) {
    switch (result) {
        case SP_ERR_ARG:
            throw std::runtime_error("Invalid argument");
        case SP_ERR_FAIL: {
            const std::string errorMessage = sp_last_error_message();
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
    int bytesRead = 0;
    uint8_t byte;
    HdlcFrame frame;
    while ((bytesRead = sp_blocking_read(this->port, &byte, 1, 100)) > 0) {
        frame.decodeByte(byte);
    }

    if (bytesRead < 0) {
        throw std::runtime_error("Failed to read from UART port");
    }

    return frame.getData();
}

void UartConnector::send(const std::vector<uint8_t> &data) {
    if (data.empty()) {
        throw std::runtime_error("No data to send");
    }
    HdlcFrame hdlcFrame(data);
    std::vector<uint8_t> frame = hdlcFrame.encode();
    UartConnector::checkSerialResult(sp_blocking_write(this->port, frame.data(), frame.size(), 1000));
}

}  // namespace iqrf::connector::uart
