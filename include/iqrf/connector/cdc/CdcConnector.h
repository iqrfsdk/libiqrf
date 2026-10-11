/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: CdcConnector.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-12
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */
#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "iqrf/connector/IConnector.h"
#include "iqrf/connector/TrInfo.h"
#include "iqrf/connector/cdc/CdcProtocol.h"
#include "iqrf/connector/cdc/ICdcPort.h"
#include "iqrf/log/Logging.h"

namespace iqrf::connector::cdc {

namespace log = ::iqrf::log;

/**
 * IQRF USB CDC connector (IQRF USB devices, e.g. GW-USB-04)
 *
 * The connector communicates with the TR module through the USB device, which implements
 * the CDC Implementation in IQRF USB devices protocol.
 */
class CdcConnector : public IConnector {
 public:
    /**
     * Constructs the IQRF USB CDC connector
     * @param device Name of the CDC device (e.g. /dev/ttyACM0 on Linux)
     */
    explicit CdcConnector(const std::string &device);

    /**
     * Constructs the IQRF USB CDC connector with the specified CDC port (e.g. for testing)
     * @param device Name of the CDC device, used only for logging if the port is specified
     * @param port CDC port, the port is opened according to the device name if null
     */
    CdcConnector(const std::string &device, std::unique_ptr<ICdcPort> port);

    /**
     * Destructs the IQRF USB CDC connector
     */
    ~CdcConnector() override;

    // Disable copying and moving
    CdcConnector(const CdcConnector&) = delete;
    CdcConnector& operator=(const CdcConnector&) = delete;
    CdcConnector(CdcConnector&&) = delete;
    CdcConnector& operator=(CdcConnector&&) = delete;

    // Basic state
    /**
     * Get the current state of the connector.
     */
    State getState() const override { return State::Ready; }

    // Basic communication
    /**
     * Send the data message via the connector.
     *
     * The data are sent to the TR module (DS command). Data of at most 255 B are accepted.
     *
     * @throws std::runtime_error if the device rejects the data or the TR module is busy
     */
    void send(const std::vector<uint8_t> &data) override;

    /**
     * Read the data synchronously from the connector.
     *
     * Returns the next asynchronous data message received from the TR module, waits up to the receive timeout
     * for its completion.
     *
     * @return Data of the received message, empty if no message has been received within the receive timeout
     */
    std::vector<uint8_t> receive() override;

    // Transceiver operations
    /**
     * Retrieve basic information about the TR module (IT command).
     */
    TrInfo readTrInfo() override;

    /**
     * Reset the TR module (RT command).
     *
     * Data received before the reset are discarded.
     */
    void resetTr() override;

    // USB device operations
    /**
     * Reset the USB device (R command).
     *
     * The device is reset 5 s after the command, the connector cannot be used afterwards.
     */
    void resetUsb();

    /**
     * Indicate the USB device acoustically or optically (B command).
     */
    void indicateConnectivity();

    /**
     * Retrieve the identification of the USB device (I command).
     * @return USB device type, firmware version and serial number
     */
    UsbDeviceInfo readUsbInfo();

    /**
     * Retrieve the SPI status of the TR module (S command).
     * @return SPI status value according to the table in the IQRF SPI User's guide (chapter SPI status)
     */
    uint8_t readSpiStatus();

    /**
     * Switch the USB class to Custom (U command).
     *
     * The device is reset 5 s after the command, the USB CDC is not available afterwards.
     */
    void switchToCustomClass();

    /**
     * Switch the USB device to the CDC-UART transparent mode (UU command).
     *
     * The device is reset 5 s after the command, the USB CDC is not available afterwards.
     */
    void switchToCdcUart();

    /**
     * Switch the USB device to the CDC-SPI transparent mode (US command).
     *
     * The device is reset 5 s after the command, the USB CDC is not available afterwards.
     */
    void switchToCdcSpi();

    // Programming mode
    /**
     * Switch the connected TR to programming mode (PE command).
     *
     * @throws std::runtime_error if the TR module does not enter the programming mode
     */
    void enterProgrammingMode() override;

    /**
     * Wait for TR to enter programming mode.
     *
     * Nothing to wait for, the PE command is answered only after the TR module has entered the programming mode.
     */
    void awaitProgrammingMode() override {}

    /**
     * Switch the connected TR back from programming mode (PT command).
     *
     * @throws std::runtime_error if the TR module does not leave the programming mode
     */
    void exitProgrammingMode() override;

    /**
     * Uploads the data to the TR module in programming mode (PM command).
     *
     * @param target specifies what the uploaded data contain.
     * @param data is the actual data to be uploaded.
     * @throws std::runtime_error if the upload fails
     */
    void upload(
        const ProgrammingTarget target,
        const std::vector<uint8_t> &data
    ) override;

    /**
     * Downloads data from the TR module in programming mode (PM command).
     *
     * @param target specifies which data shall be downloaded.
     * @throws std::runtime_error if the download fails
     */
    std::vector<uint8_t> download(const ProgrammingTarget target) override;

    /**
     * Downloads data from the TR module memory in programming mode (PM command).
     *
     * @param target specifies which data shall be downloaded.
     * @param address specifies the Flash or EEPROM address from which the data will be downloaded.
     * @throws std::runtime_error if the download fails
     */
    std::vector<uint8_t> download(
        const ProgrammingTarget target,
        const uint16_t address
    ) override;

 private:
    /**
     * Writes the command and waits for the response with the specified header.
     *
     * Asynchronous data received meanwhile are queued for receive(). The caller must hold receiveMutex.
     *
     * @param command Command to write
     * @param header Expected header of the response
     * @param downloadLength Length of the downloaded data if the response is a download response
     * @return The response frame
     * @throws std::runtime_error if the response is not received within the response timeout
     */
    CdcFrame transact(
        const std::vector<uint8_t> &command,
        const std::string &header,
        std::optional<std::size_t> downloadLength = std::nullopt
    );

    /**
     * Sends the download command and returns the downloaded data.
     *
     * The caller must hold receiveMutex.
     *
     * @param command Download command to write
     * @param dataLength Length of the downloaded data
     * @return Downloaded data
     * @throws std::runtime_error if the download fails
     */
    std::vector<uint8_t> downloadResponse(const std::vector<uint8_t> &command, std::size_t dataLength);

    /**
     * Reads the next complete frame, waits up to the specified deadline.
     *
     * Invalid frames are logged and discarded. The caller must hold receiveMutex.
     *
     * @param deadline Time point to stop waiting
     * @param downloadLength Length of the downloaded data if the frame may be a download response
     * @return The frame, nullopt if no complete frame has been received before the deadline
     * @throws std::runtime_error on read failure
     */
    std::optional<CdcFrame> readFrame(
        std::chrono::steady_clock::time_point deadline,
        std::optional<std::size_t> downloadLength
    );

    /**
     * Executes the command answered with "<HEADER:OK\r"
     *
     * The caller must hold receiveMutex.
     *
     * @param header Header of the command
     * @throws std::runtime_error if the device does not answer OK
     */
    void expectOk(const std::string &header);

    /**
     * Discards the received bytes and the queued messages
     */
    void discardReceived();

    /// Maximum time to wait for a message in receive()
    static constexpr std::chrono::milliseconds RECEIVE_TIMEOUT{100};
    /// Maximum time to wait for a response of a command
    static constexpr std::chrono::milliseconds RESPONSE_TIMEOUT{5000};
    /// Maximum number of bytes read from the CDC port at once
    static constexpr std::size_t READ_SIZE = 1024;
    /// Maximum time to write a command
    static constexpr std::chrono::milliseconds WRITE_TIMEOUT{1000};

    /// CDC port
    std::unique_ptr<ICdcPort> port;

    /// Guards receiving state and the transactions with the device
    std::mutex receiveMutex;

    /// Bytes received but not yet forming a complete frame
    std::vector<uint8_t> rxBuffer;

    /// Asynchronous data not yet returned by receive()
    std::deque<std::vector<uint8_t>> receivedData;
};

}  // namespace iqrf::connector::cdc
