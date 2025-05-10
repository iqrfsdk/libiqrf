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

UartConnector::UartConnector(const iqrf::connector::uart::UartConfig &config): IConnector(), config(config) {
    IQRF_LOG(log::Level::Debug) << "Opening UART port: " << this->config.device;
    UartConnector::checkSerialResult(sp_get_port_by_name(this->config.device.c_str(), &this->port));
    IQRF_LOG(log::Level::Debug) << "UART port created: " << this->config.device
        << " (name: " << sp_get_port_name(this->port) << ", description: "
        << sp_get_port_description(this->port) << ")";
    if (sp_get_port_transport(this->port) == SP_TRANSPORT_USB) {
        int usbBus, usbAddress;
        UartConnector::checkSerialResult(sp_get_port_usb_bus_address(this->port, &usbBus, &usbAddress));
        IQRF_LOG(log::Level::Debug) << "UART port is on USB bus: " << usbBus << ", USB address: " << usbAddress
            << ", Manufacturer: " << sp_get_port_usb_manufacturer(this->port)
            << ", Product: " << sp_get_port_usb_product(this->port);
    }

    // Open the port
    UartConnector::checkSerialResult(sp_open(this->port, SP_MODE_READ_WRITE));

    // Set up the port
    UartConnector::checkSerialResult(sp_set_baudrate(this->port, this->config.baudRate));
    UartConnector::checkSerialResult(sp_set_bits(this->port, 8));
    UartConnector::checkSerialResult(sp_set_parity(this->port, SP_PARITY_NONE));
    UartConnector::checkSerialResult(sp_set_stopbits(this->port, 1));
    UartConnector::checkSerialResult(sp_set_flowcontrol(this->port, SP_FLOWCONTROL_NONE));

}

UartConnector::~UartConnector() {
    if (this->port) {
        sp_close(this->port);
        sp_free_port(this->port);
    }
}

int UartConnector::checkSerialResult(enum sp_return result) {
    switch (result) {
        case SP_ERR_ARG:
            throw new std::runtime_error("Invalid argument");
        case SP_ERR_FAIL: {
            ::std::string errorMessage = sp_last_error_message();
            throw new std::runtime_error("Failed: " + errorMessage);
        }
        case SP_ERR_MEM:
            throw new std::runtime_error("Memory allocation error");
        case SP_ERR_SUPP:
            throw new std::runtime_error("Operation not supported");
        default:
            return result;
    }
}

::std::vector<uint8_t> UartConnector::receive() {
    ::std::vector<uint8_t> data;
    int bytesRead = 0;
    uint8_t byte;

    while ((bytesRead = sp_blocking_read(this->port, &byte, 1, 100)) > 0) {
        data.push_back(byte);
        if (byte == '\n' || byte == '\r' || byte == '\0') {
            break;
        }
    }

    if (bytesRead < 0) {
        throw new std::runtime_error("Failed to read from UART port");
    }

    return data;
}

}
