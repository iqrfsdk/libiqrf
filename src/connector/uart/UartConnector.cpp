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
    IQRF_LOG(log::Level::Debug) << "Opening UART port: " << this->config.device;
    UartConnector::checkSerialResult(sp_get_port_by_name(this->config.device.c_str(), &this->port));
    IQRF_LOG(log::Level::Debug) << "UART port created: " << this->config.device
        << " (name: " << sp_get_port_name(this->port) << ", description: "
        << sp_get_port_description(this->port) << ")\n";
    if (sp_get_port_transport(this->port) == SP_TRANSPORT_USB) {
        int usbBus, usbAddress;
        UartConnector::checkSerialResult(sp_get_port_usb_bus_address(this->port, &usbBus, &usbAddress));
        IQRF_LOG(log::Level::Debug) << "UART port is on USB bus: " << usbBus << ", USB address: " << usbAddress
            << ", Manufacturer: " << sp_get_port_usb_manufacturer(this->port)
            << ", Product: " << sp_get_port_usb_product(this->port) << "\n";
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
    if (this->port) {
        sp_close(this->port);
        sp_free_port(this->port);
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
    std::vector<uint8_t> data;
    int bytesRead = 0;
    uint8_t byte;
    bool running = false;
    bool escape = false;

    while ((bytesRead = sp_blocking_read(this->port, &byte, 1, 1000)) > 0) {
        if (!running && byte == 0x7E) {
            running = true;
            continue;
        }
        if (running && byte == 0x7D) {
            escape = true;
            continue;
        }
        if (running && escape) {
            escape = false;
            if (byte == 0x7E) {
                byte = 0x7F;
            } else if (byte == 0x7D) {
                byte = 0x7E;
            }
        }
        if (running && byte == 0x7E) {
            break;
        }
        data.push_back(byte);
    }

    if (bytesRead < 0) {
        throw std::runtime_error("Failed to read from UART port");
    }

    if (data.empty()) {
        return data;
    }

    const uint8_t crc = data.back();
    data.pop_back();
    if (crc != calculateCrc(data)) {
        throw std::runtime_error("CRC check failed");
    }

    return data;
}

void UartConnector::send(const std::vector<uint8_t> &data) {
    if (data.empty()) {
        throw std::runtime_error("No data to send");
    }
    std::vector<uint8_t> frame;
    frame.push_back(0x7E);
    for (const auto byte : data) {
        if (byte == 0x7E) {
            frame.push_back(0x7D);
            frame.push_back(0x7E);
        } else if (byte == 0x7D) {
            frame.push_back(0x7D);
            frame.push_back(0x7D);
        } else {
            frame.push_back(byte);
        }
    }
    frame.push_back(calculateCrc(data));
    frame.push_back(0x7E);
    UartConnector::checkSerialResult(sp_blocking_write(this->port, frame.data(), frame.size(), 1000));
}

uint8_t UartConnector::calculateCrc(const std::vector<uint8_t> &data) {
    uint8_t crc = 0xFF;
    for (const uint8_t byte : data) {
        crc ^= byte;
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x01) {
                crc = (crc >> 1) ^ 0x8C;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

}  // namespace iqrf::connector::uart
