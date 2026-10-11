/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: CdcConnector.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-12
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */
#include "iqrf/connector/cdc/CdcConnector.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "iqrf/connector/IConnector.h"
#include "iqrf/connector/cdc/CdcProtocol.h"
#include "iqrf/connector/cdc/ICdcPort.h"
#include "iqrf/connector/TrInfo.h"
#include "iqrf/log/Logging.h"

namespace iqrf::connector::cdc {

using Clock = std::chrono::steady_clock;

CdcConnector::CdcConnector(const std::string &device): CdcConnector(device, nullptr) {}

CdcConnector::CdcConnector(const std::string &device, std::unique_ptr<ICdcPort> port): port(std::move(port)) {
    if (!this->port) {
        this->port = createCdcPort(device);
    }
}

CdcConnector::~CdcConnector() {
    this->stopListen();
}

void CdcConnector::send(const std::vector<uint8_t> &data) {
    if (data.empty()) {
        throw std::runtime_error("No data to send");
    }
    const std::scoped_lock lock(this->receiveMutex);
    const CdcFrame response = this->transact(protocol::buildSendCommand(data), "DS");
    const std::string status = protocol::statusOf(response);
    if (status == "OK") {
        return;
    }
    if (status == "BUSY") {
        throw std::runtime_error("Cannot send data: TR module is busy");
    }
    throw std::runtime_error("Cannot send data: CDC device responded with " + status);
}

std::vector<uint8_t> CdcConnector::receive() {
    const std::scoped_lock lock(this->receiveMutex);
    const auto deadline = Clock::now() + RECEIVE_TIMEOUT;
    while (this->receivedData.empty()) {
        const std::optional<CdcFrame> frame = this->readFrame(deadline, std::nullopt);
        if (!frame.has_value()) {
            break;
        }
        if (frame->header == "DR") {
            this->receivedData.push_back(frame->body);
        } else {
            IQRF_LOG(log::Level::Warning) << "Discarding unexpected CDC frame: " << frame->header;
        }
    }
    if (this->receivedData.empty()) {
        return {};
    }
    std::vector<uint8_t> data = std::move(this->receivedData.front());
    this->receivedData.pop_front();
    return data;
}

TrInfo CdcConnector::readTrInfo() {
    const std::scoped_lock lock(this->receiveMutex);
    const CdcFrame response = this->transact(protocol::buildCommand("IT"), "IT");
    return TrInfo::parse(response.body);
}

void CdcConnector::resetTr() {
    const std::scoped_lock lock(this->receiveMutex);
    const CdcFrame response = this->transact(protocol::buildCommand("RT"), "RT");
    const std::string status = protocol::statusOf(response);
    if (status != "OK") {
        throw std::runtime_error("Cannot reset TR module: CDC device responded with " + status);
    }
    // Data received before the reset are discarded
    this->discardReceived();
}

void CdcConnector::resetUsb() {
    const std::scoped_lock lock(this->receiveMutex);
    this->expectOk("R");
}

void CdcConnector::indicateConnectivity() {
    const std::scoped_lock lock(this->receiveMutex);
    this->expectOk("B");
}

UsbDeviceInfo CdcConnector::readUsbInfo() {
    const std::scoped_lock lock(this->receiveMutex);
    const CdcFrame response = this->transact(protocol::buildCommand("I"), "I");
    return protocol::parseUsbDeviceInfo(response.body);
}

uint8_t CdcConnector::readSpiStatus() {
    const std::scoped_lock lock(this->receiveMutex);
    const CdcFrame response = this->transact(protocol::buildCommand("S"), "S");
    if (response.body.size() != 1) {
        throw std::runtime_error("Invalid SPI status received from CDC device");
    }
    return response.body[0];
}

void CdcConnector::switchToCustomClass() {
    const std::scoped_lock lock(this->receiveMutex);
    this->expectOk("U");
}

void CdcConnector::switchToCdcUart() {
    const std::scoped_lock lock(this->receiveMutex);
    this->expectOk("UU");
}

void CdcConnector::switchToCdcSpi() {
    const std::scoped_lock lock(this->receiveMutex);
    this->expectOk("US");
}

void CdcConnector::expectOk(const std::string &header) {
    const CdcFrame response = this->transact(protocol::buildCommand(header), header);
    const std::string status = protocol::statusOf(response);
    if (status != "OK") {
        throw std::runtime_error("Command " + header + " failed: CDC device responded with " + status);
    }
}

void CdcConnector::enterProgrammingMode() {
    const std::scoped_lock lock(this->receiveMutex);
    const CdcFrame response = this->transact(protocol::buildCommand("PE"), "PE");
    const std::string status = protocol::statusOf(response);
    if (status != "OK") {
        throw std::runtime_error("Cannot enter programming mode: CDC device responded with " + status);
    }
}

void CdcConnector::exitProgrammingMode() {
    const std::scoped_lock lock(this->receiveMutex);
    const CdcFrame response = this->transact(protocol::buildCommand("PT"), "PT");
    const std::string status = protocol::statusOf(response);
    if (status != "OK") {
        throw std::runtime_error("Cannot exit programming mode: CDC device responded with " + status);
    }
}

void CdcConnector::upload(const ProgrammingTarget target, const std::vector<uint8_t> &data) {
    const std::scoped_lock lock(this->receiveMutex);
    const CdcFrame response = this->transact(protocol::buildUploadCommand(target, data), "PM");
    const std::string status = protocol::statusOf(response);
    if (status != "OK") {
        throw std::runtime_error("Upload failed: CDC device responded with " + status);
    }
}

std::vector<uint8_t> CdcConnector::download(const ProgrammingTarget target) {
    const std::scoped_lock lock(this->receiveMutex);
    return this->downloadResponse(
        protocol::buildDownloadCommand(target, std::nullopt),
        protocol::downloadLength(target)
    );
}

std::vector<uint8_t> CdcConnector::download(const ProgrammingTarget target, const uint16_t address) {
    const std::scoped_lock lock(this->receiveMutex);
    return this->downloadResponse(
        protocol::buildDownloadCommand(target, address),
        protocol::downloadLength(target)
    );
}

std::vector<uint8_t> CdcConnector::downloadResponse(const std::vector<uint8_t> &command, const std::size_t dataLength) {
    const CdcFrame response = this->transact(command, "PM", dataLength);
    // Failure is reported by a status text only, its length never matches the downloaded data
    const std::string status = protocol::statusOf(response);
    if (response.body.size() != dataLength) {
        throw std::runtime_error("Download failed: CDC device responded with " + status);
    }
    return response.body;
}

CdcFrame CdcConnector::transact(
    const std::vector<uint8_t> &command,
    const std::string &header,
    const std::optional<std::size_t> downloadLength
) {
    this->port->write(command, WRITE_TIMEOUT);
    const auto deadline = Clock::now() + RESPONSE_TIMEOUT;
    while (true) {
        const std::optional<CdcFrame> frame = this->readFrame(deadline, downloadLength);
        if (!frame.has_value()) {
            throw std::runtime_error("Timeout while waiting for CDC response " + header);
        }
        // The TR module busy status is answered with "<PC:BUSY" in the guide
        if (frame->header == header || (header == "PM" && frame->header == "PC")) {
            return *frame;
        }
        if (frame->header == "DR") {
            this->receivedData.push_back(frame->body);
        } else if (frame->header == "ERR") {
            throw std::runtime_error("CDC device reported an error while waiting for response " + header);
        } else {
            IQRF_LOG(log::Level::Warning) << "Discarding unexpected CDC frame " << frame->header
                << " while waiting for " << header;
        }
    }
}

std::optional<CdcFrame> CdcConnector::readFrame(
    const Clock::time_point deadline,
    const std::optional<std::size_t> downloadLength
) {
    while (true) {
        try {
            std::optional<CdcFrame> frame = downloadLength.has_value()
                ? protocol::extractDownloadFrame(this->rxBuffer, *downloadLength)
                : protocol::extractFrame(this->rxBuffer);
            if (frame.has_value()) {
                return frame;
            }
        } catch (const CdcFrameError &e) {
            IQRF_LOG(log::Level::Warning) << "Discarding invalid CDC frame: " << e.what();
            this->rxBuffer.clear();
        }
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - Clock::now());
        if (remaining.count() <= 0) {
            return std::nullopt;
        }
        const std::vector<uint8_t> bytes = this->port->read(READ_SIZE, remaining);
        this->rxBuffer.insert(this->rxBuffer.end(), bytes.begin(), bytes.end());
    }
}

void CdcConnector::discardReceived() {
    this->rxBuffer.clear();
    this->receivedData.clear();
}

}  // namespace iqrf::connector::cdc
