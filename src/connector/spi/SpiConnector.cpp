/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: SpiConnector.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-05
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include "iqrf/connector/spi/SpiConnector.h"

#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "iqrf/connector/ConnectorUtils.h"
#include "iqrf/log/Logging.h"

namespace iqrf::connector::spi {

namespace log = ::iqrf::log;

namespace {

/// Timing of low speed communication mode
constexpr SpiTransferTiming LOW_SPEED_TIMING = {
    std::chrono::microseconds(10),
    std::chrono::microseconds(800),
    false,
};

/// Timing of high speed communication mode
constexpr SpiTransferTiming HIGH_SPEED_TIMING = {
    std::chrono::microseconds(5),
    std::chrono::microseconds(150),
    true,
};

/// Number of attempts to read a stable SPI status
constexpr int STATUS_READ_ATTEMPTS = 3;
/// Number of attempts to transfer a data packet before giving up
constexpr int PACKET_ATTEMPTS = 3;

/**
 * Formats SPI status byte for error messages
 * @param raw Raw SPI status byte
 * @return Formatted SPI status
 */
std::string formatStatus(const uint8_t raw) {
    std::stringstream ss;
    ss << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(raw);
    return ss.str();
}

/**
 * Checks whether SPI of TR module is active (TR module is able to communicate)
 * @param status SPI status
 * @return true if SPI is active, false otherwise
 */
bool isSpiActive(const SpiStatus &status) {
    return status.isDataReady() ||
        status.is(SpiStatusValue::ReadyCommunication) ||
        status.is(SpiStatusValue::ReadyProgramming) ||
        status.is(SpiStatusValue::ReadyDebug);
}

}  // namespace

SpiConnector::SpiConnector(SpiConfig config): SpiConnector(std::move(config), nullptr) {}

SpiConnector::SpiConnector(SpiConfig config, std::unique_ptr<ISpiDevice> device):
    busSwitcher(config.busSwitch()),
    config(std::move(config)),
    device(std::move(device)) {
    if (this->config.speed == 0 || this->config.speed > SpiConfig::MAX_SPEED) {
        throw std::invalid_argument(
            "SPI clock speed must be between 1 Hz and " + std::to_string(SpiConfig::MAX_SPEED) + " Hz"
        );
    }
    // Open SPI device first to fail before touching GPIOs
    if (!this->device) {
        this->device = createSpiDevice(SpiDeviceConfig{this->config.device, this->config.speed});
    }
    try {
        this->initGpio();
    } catch (...) {
        // Destructor is not called if the constructor throws, so restore the GPIOs here
        this->shutdownGpio();
        throw;
    }
}

SpiConnector::~SpiConnector() {
    this->stopListen();
    this->shutdownGpio();
}

void SpiConnector::shutdownGpio() noexcept {
    if (this->config.powerEnableGpio.has_value() && this->config.disablePowerOnShutdown) {
        ConnectorUtils::runSafely("disable TR power", [this] {
            this->config.powerEnableGpio->setValue(false);
        });
    }
    ConnectorUtils::runSafely("disable SPI bus", [this] {
        this->busSwitcher.toggleSpi(false);
    });
    if (this->config.pgmSwitchGpio.has_value()) {
        ConnectorUtils::runSafely("release PGM switch", [this] {
            this->config.pgmSwitchGpio->setValue(false);
        });
    }
}

void SpiConnector::initGpio() {
    if (this->config.pgmSwitchGpio.has_value()) {
        this->config.pgmSwitchGpio->initOutput(false);
    }
    if (this->config.powerEnableGpio.has_value()) {
        this->config.powerEnableGpio->initOutput(true);
    }
    this->busSwitcher.init();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    if (this->config.trModuleReset && this->config.powerEnableGpio.has_value()) {
        this->powerCycleTr(false);
    }

    this->busSwitcher.toggleSpi(true);

    if (this->config.powerEnableGpio.has_value()) {
        // TR module waits for the programming mode sequence after power up
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

SpiTransferTiming SpiConnector::timing() const {
    if (this->config.communicationMode == SpiCommunicationMode::LowSpeed) {
        return LOW_SPEED_TIMING;
    }
    return HIGH_SPEED_TIMING;
}

std::vector<uint8_t> SpiConnector::transfer(const std::vector<uint8_t> &tx) const {
    return this->device->transfer(tx, this->timing());
}

State SpiConnector::getState() const {
    try {
        if (isSpiActive(this->getSpiStatus())) {
            return State::Ready;
        }
    } catch (const std::exception &e) {
        IQRF_LOG(log::Level::Warning) << "Unable to read SPI status: " << e.what();
    }
    return State::NotReady;
}

SpiStatus SpiConnector::getSpiStatus() const {
    const std::scoped_lock lock(this->spiMutex);
    const std::vector<uint8_t> check = {SpiProtocol::CMD_CHECK};
    std::optional<uint8_t> raw;
    for (int attempt = 0; attempt < STATUS_READ_ATTEMPTS && !raw.has_value(); ++attempt) {
        // Dummy reads to get synchronized SPI status
        for (int i = 0; i < 2; ++i) {
            this->transfer(check);
        }
        const uint8_t first = this->transfer(check)[0];
        const uint8_t second = this->transfer(check)[0];
        if (first == second) {
            raw = first;
        }
    }
    if (!raw.has_value()) {
        throw std::runtime_error("Unable to read stable SPI status");
    }
    const std::optional<SpiStatus> status = SpiStatus::parse(*raw);
    if (!status.has_value()) {
        throw std::runtime_error("Invalid SPI status: " + formatStatus(*raw));
    }
    return *status;
}

SpiCommunicationMode SpiConnector::getCommunicationMode() const {
    const std::scoped_lock lock(this->spiMutex);
    return this->config.communicationMode;
}

void SpiConnector::setCommunicationMode(const SpiCommunicationMode mode) {
    const std::scoped_lock lock(this->spiMutex);
    this->config.communicationMode = mode;
}

std::optional<std::vector<uint8_t>> SpiConnector::readData(const std::size_t length) {
    const std::vector<uint8_t> packet = SpiProtocol::buildReadPacket(length);
    for (int attempt = 1; attempt <= PACKET_ATTEMPTS; ++attempt) {
        const std::vector<uint8_t> response = this->transfer(packet);
        auto data = SpiProtocol::parseReadResponse(length, response);
        // Only explicit CRCM error is considered as a failure, received data are protected by CRCS
        const bool crcmError = response.back() == static_cast<uint8_t>(SpiStatusValue::CrcmError);
        if (data.has_value() && !crcmError) {
            return data;
        }
        // Data remains valid in bufferCOM, so the reading can be repeated
        IQRF_LOG(log::Level::Warning) << "SPI read failed (" << (data.has_value() ? "CRCM" : "CRCS")
            << " mismatch), attempt " << attempt << "/" << PACKET_ATTEMPTS;
        std::this_thread::sleep_for(STATUS_POLL_INTERVAL);
    }
    return std::nullopt;
}

SpiStatus SpiConnector::awaitStatus(
    const std::function<bool(const SpiStatus&)> &predicate,
    const char *description
) const {
    const auto deadline = std::chrono::steady_clock::now() + STATUS_TIMEOUT;
    while (true) {
        const SpiStatus status = this->getSpiStatus();
        if (predicate(status)) {
            return status;
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            throw std::runtime_error(
                std::string("Timeout while waiting for ") + description + " (SPI status " +
                formatStatus(status.getRaw()) + ")"
            );
        }
        std::this_thread::sleep_for(STATUS_POLL_INTERVAL);
    }
}

void SpiConnector::send(const std::vector<uint8_t> &data) {
    const std::vector<uint8_t> packet = SpiProtocol::buildWritePacket(data);
    const std::scoped_lock lock(this->spiMutex);
    IQRF_LOG(log::Level::Trace) << "SPI send: " << ConnectorUtils::vectorToHexString(data);
    for (int attempt = 1; attempt <= PACKET_ATTEMPTS; ++attempt) {
        this->awaitStatus([this](const SpiStatus &status) {
            if (status.isDataReady()) {
                // TR module does not accept new data until the data it has ready are read
                const auto received = this->readData(status.getDataReadyLength());
                if (received.has_value()) {
                    this->pendingMessages.push_back(*received);
                } else {
                    IQRF_LOG(log::Level::Warning) << "CRCS mismatch of data received before send";
                }
                return false;
            }
            return status.is(SpiStatusValue::ReadyCommunication);
        }, "TR module to be ready to receive data");
        const std::vector<uint8_t> response = this->transfer(packet);
        if (SpiProtocol::isCrcmConfirmed(response)) {
            return;
        }
        if (response.back() != static_cast<uint8_t>(SpiStatusValue::CrcmError)) {
            // Unknown result, the packet must not be repeated as TR module could have accepted it
            throw std::runtime_error(
                "SPI write was not confirmed by TR module (SPI status " + formatStatus(response.back()) + ")"
            );
        }
        // Packet with wrong CRCM is discarded by TR module, so it can be sent again
        IQRF_LOG(log::Level::Warning) << "SPI write not confirmed (CRCM mismatch), attempt "
            << attempt << "/" << PACKET_ATTEMPTS;
    }
    throw std::runtime_error("SPI write was not confirmed by TR module (CRCM mismatch)");
}

std::vector<uint8_t> SpiConnector::receive() {
    const std::scoped_lock lock(this->spiMutex);
    if (!this->pendingMessages.empty()) {
        std::vector<uint8_t> data = std::move(this->pendingMessages.front());
        this->pendingMessages.pop_front();
        return data;
    }
    const SpiStatus status = this->getSpiStatus();
    if (!status.isDataReady()) {
        return {};
    }
    const auto data = this->readData(status.getDataReadyLength());
    if (!data.has_value()) {
        IQRF_LOG(log::Level::Warning) << "CRCS mismatch of received data";
        return {};
    }
    IQRF_LOG(log::Level::Trace) << "SPI receive: " << ConnectorUtils::vectorToHexString(*data);
    return *data;
}

TrInfo SpiConnector::readTrInfo() {
    const std::scoped_lock lock(this->spiMutex);
    // TR module does not provide the information until its SPI is active (e.g. during TR module startup)
    this->awaitStatus(isSpiActive, "TR module SPI to be active");
    TrInfo info = TrInfo::parse(this->readModuleInfo(TrInfo::BASIC_LENGTH));
    if (info.supportsIbk()) {
        // IQRF OS v4.03 or higher provides IBK in the extended Module Info block
        info = TrInfo::parse(this->readModuleInfo(TrInfo::EXTENDED_LENGTH));
    }
    return info;
}

std::vector<uint8_t> SpiConnector::readModuleInfo(const std::size_t length) {
    const std::vector<uint8_t> packet = SpiProtocol::buildTrInfoPacket(length);
    for (int attempt = 1; attempt <= PACKET_ATTEMPTS; ++attempt) {
        const std::vector<uint8_t> response = this->transfer(packet);
        auto data = SpiProtocol::parseReadResponse(length, response);
        if (data.has_value()) {
            return *data;
        }
        IQRF_LOG(log::Level::Warning) << "Reading TR module information failed (CRCS mismatch, response: "
            << ConnectorUtils::vectorToHexString(response) << "), attempt " << attempt << "/" << PACKET_ATTEMPTS;
        std::this_thread::sleep_for(STATUS_POLL_INTERVAL);
    }
    throw std::runtime_error("CRCS mismatch of TR module information");
}

void SpiConnector::powerCycleTr(const bool enableBus) {
    // Disconnect SPI master from TR module
    this->busSwitcher.toggleSpi(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    this->config.powerEnableGpio->setValue(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    this->config.powerEnableGpio->setValue(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    if (enableBus) {
        this->busSwitcher.toggleSpi(true);
    }
}

void SpiConnector::resetTr() {
    const std::scoped_lock lock(this->spiMutex);
    if (!this->config.powerEnableGpio.has_value()) {
        IQRF_LOG(log::Level::Warning) << "Unable to reset TR module: power enable GPIO is not configured";
        return;
    }
    this->powerCycleTr(true);
}

void SpiConnector::enterProgrammingMode() {
    const std::scoped_lock lock(this->spiMutex);
    if (this->getSpiStatus().is(SpiStatusValue::ReadyProgramming)) {
        return;
    }
    if (!this->config.pgmSwitchGpio.has_value()) {
        throw std::runtime_error("Unable to enter programming mode: PGM switch GPIO is not configured");
    }
    if (!this->config.powerEnableGpio.has_value()) {
        throw std::runtime_error("Unable to enter programming mode: power enable GPIO is not configured");
    }

    // TR module enters programming mode if PGM pin is active during power up
    this->busSwitcher.toggleSpi(false);
    this->config.pgmSwitchGpio->setValue(true);
    this->powerCycleTr(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    this->config.pgmSwitchGpio->setValue(false);
    this->busSwitcher.toggleSpi(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    this->awaitProgrammingMode();
}

void SpiConnector::awaitProgrammingMode() {
    const std::scoped_lock lock(this->spiMutex);
    this->awaitStatus([](const SpiStatus &status) {
        return status.is(SpiStatusValue::ReadyProgramming);
    }, "programming mode");
}

void SpiConnector::exitProgrammingMode() {
    const std::scoped_lock lock(this->spiMutex);
    if (this->getSpiStatus().is(SpiStatusValue::ReadyCommunication)) {
        return;
    }
    if (!this->config.powerEnableGpio.has_value()) {
        throw std::runtime_error("Unable to exit programming mode: power enable GPIO is not configured");
    }
    // Wait for TR module to finish pending programming operation
    this->awaitProgrammingMode();
    this->powerCycleTr(true);
}

void SpiConnector::upload(const ProgrammingTarget target, const std::vector<uint8_t> &data) {
    const std::vector<uint8_t> packet = SpiProtocol::buildUploadPacket(target, data);
    const std::scoped_lock lock(this->spiMutex);
    this->awaitProgrammingMode();
    const std::vector<uint8_t> response = this->transfer(packet);
    if (!SpiProtocol::isCrcmConfirmed(response)) {
        throw std::runtime_error("Upload was not confirmed by TR module (CRCM mismatch)");
    }
    this->awaitStatus([](const SpiStatus &status) {
        return status.is(SpiStatusValue::ReadyProgramming);
    }, "upload completion");
}

std::vector<uint8_t> SpiConnector::download(const ProgrammingTarget target) {
    if (target != ProgrammingTarget::Rfpgm && target != ProgrammingTarget::RfBand) {
        throw std::invalid_argument("Download of the specified target requires an address");
    }
    return this->download(target, 0);
}

std::vector<uint8_t> SpiConnector::download(const ProgrammingTarget target, const uint16_t address) {
    const std::vector<uint8_t> packet = SpiProtocol::buildDownloadPacket(target, address);
    const std::scoped_lock lock(this->spiMutex);
    this->awaitProgrammingMode();
    const std::vector<uint8_t> response = this->transfer(packet);
    if (!SpiProtocol::isCrcmConfirmed(response)) {
        throw std::runtime_error("Download was not confirmed by TR module (CRCM mismatch)");
    }
    this->awaitStatus([](const SpiStatus &status) {
        return status.isDataReady() && status.getDataReadyLength() == SpiProtocol::DOWNLOAD_LENGTH;
    }, "downloaded data");
    const auto block = this->readData(SpiProtocol::DOWNLOAD_LENGTH);
    if (!block.has_value()) {
        throw std::runtime_error("CRCS mismatch of downloaded data");
    }
    return SpiProtocol::parseDownloadResponse(target, *block);
}

}  // namespace iqrf::connector::spi
