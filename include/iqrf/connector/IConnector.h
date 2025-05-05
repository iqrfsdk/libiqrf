/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: IConnector.h
 * Authors: Ondřej Hujňák <ondrej.hujnak@iqrf.org>
 * Date: 2025-04-23
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#pragma once

#include <atomic>
#include <functional>
#include <thread>
#include <vector>

namespace iqrf::connector {

/**
 * Represents a state of the connector.
 */
enum class State {
    Ready,
    NotReady,
    Exclusive
};

/**
 * Represents the mode of access to the connector.
 */
enum class AccessType {
    Normal,
    Exclusive,
    Sniffer
};

/**
 * Represents the target for upload.
 */
enum class ProgrammingTarget {
    Config,
    Rfpgm,
    RfBand,
    AccessPassword,
    UserKey,
    Flash,
    InternalEeprom,
    ExternalEeprom,
    Special
};

typedef std::function<int(const std::vector<uint8_t>&)> ResponseHandler;

/**
 * Transceiver information as returned by IQRF OS moduleInfo()
 */
#pragma pack(push, 1)
struct TrInfo {
    uint32_t mid;
    uint8_t osVersion;
    uint8_t trType;
    uint16_t osBuild;

    uint8_t osVersionMajor() const { return osVersion >> 4; }

    uint8_t osVersionMinor() const { return osVersion & 0x0F; }
};
#pragma pack(pop)


class IConnector {
 public:
    /**
     * Constructs the connector and initializes the necessary resources (GPIO lines).
     */
    IConnector() {}

    /**
     * Destructor to cleanly release the connection resources.
     */
    virtual ~IConnector() {}

    // Basic state

    /**
     * Get the current state of the connector.
     *
     * TODO: Used in Daemon(IqrfCdc, IqrfSpi, IqrfUart)
     * 
     * FIXME: clibspi - very different states with more detail
     */
    virtual State getState() const = 0;

    // Basic communication

    /**
     * Send the data message via the connector.
     *
     * TODO: Used in Daemon(IqrfCdc, IqrfSpi, IqrfUart), clibspi, clibuart
     */
    virtual void send(const std::vector<uint8_t> data) = 0;

    /**
     * Read the data synchronously from the connector.
     *
     * TODO: Used in Uploader, clibspi, clibuart
     */
    virtual std::vector<uint8_t> receive() = 0;

    /**
     * Register the responseHandler for received messages.
     *
     * TODO: Used in Daemon(IqrfCdc, IqrfSpi, IqrfUart)
     */
    virtual void registerResponseHandler(ResponseHandler responseHandler, AccessType access) = 0;

    /**
     * Start the listening loop in a separate thread.
     *
     * TODO: Used in Daemon(IqrfCdc, IqrfSpi, IqrfUart)
     */
    virtual void listen() {
        this->listening = true;
        this->listeningThread = std::thread(&IConnector::listeningLoop, this);
    }

    // Exclusive access

    /**
     * Check whether the connector is in exclusive access mode.
     *
     * TODO: Used in Daemon(IqrfCdc, IqrfSpi, IqrfUart)
     */
    virtual bool hasExclusiveAccess() const = 0;

    // Transceiver operations

    /**
     * Retrieve basic information about the TR module.
     *
     * TODO: Used in Daemon(IqrfCdc, IqrfSpi), Uploader, clibcdc
     * 
     * FIXME: In clibcdc somehow 8b reserved and 16b IBK is included
     */
    virtual TrInfo readTrInfo() = 0;

    /**
     * Reset the TR module.
     *
     * TODO: Used in Daemon(IqrfCdc), clibspi, clibcdc
     */
    virtual void resetTr() = 0;

    // Programming mode

    /**
     * Switch the connected TR to programming mode.
     *
     * TODO: Used in Daemon(IqrfCdc, IqrfSpi), Uploader
     */
    virtual void enterProgrammingMode() = 0;

    /**
     * Wait for TR to enter programming mode.
     *
     * TODO: Used in Daemon(IqrfSpi), Uploader
     */
    virtual void awaitProgrammingMode() = 0;

    /**
     * Switch the connected TR back from programming mode.
     *
     * TODO: Used in Daemon(IqrfCdc, IqrfSpi), Uploader
     */
    virtual void exitProgrammingMode() = 0;

    /**
     * Uploads the data to the TR module in programming mode.
     *
     * @param target specifies what the uploaded data contain.
     * @param data is the actual data to be uploaded.
     *
     * TODO: Used in Daemon(IqrfCdc, IqrfSpi), Uploader, clibspi (for cutils, clibtr)
     */
    virtual void upload(
        const ProgrammingTarget target,
        const std::vector<uint8_t> data
    ) = 0;

    /**
     * Uploads the data to the TR module memory.
     *
     * Just an utility function which prepends the address to the data so it is properly packed for the upload.
     *
     * @param target specifies what the uploaded data contain.
     * @param data is the actual data to be uploaded.
     * @param address specifies the Flash or EEPROM address at which the data will be stored.
     *
     * TODO: Used in Daemon(IqrfSpi), clibspi (for cutils, clibtr)
     */
    virtual void upload(
        const ProgrammingTarget target,
        const std::vector<uint8_t> data,
        const uint16_t address
    ) {
        std::vector<uint8_t> fullData = data;
        fullData.insert(fullData.begin(), (address >> 8) & 0xFF);
        fullData.insert(fullData.begin(), address & 0xFF);

        this->upload(target, fullData);
    }

    /**
     * Downloads data from the TR module in programming mode.
     *
     * @param target specifies which data shall be downloaded.
     *
     * TODO: Used in clibspi (for cutils, clibtr)
     */
    virtual std::vector<uint8_t> download(const ProgrammingTarget target) = 0;

    /**
     * Downloads data from the TR module memory in programming mode.
     *
     * @param target specifies which data shall be downloaded.
     * @param address specifies the Flash or EEPROM address from which the data will be downloaded.
     *
     * TODO: Used in clibspi (for cutils, clibtr)
     */
    virtual std::vector<uint8_t> download(
        const ProgrammingTarget target,
        const uint16_t address
    ) = 0;

 protected:
    /**
     * Listen to the connector and asychronously call responseHandler for
     * received messages.
     *
     * TODO: Used in Daemon(IqrfSpi)
     */
    virtual void listeningLoop() = 0;

 private:
    std::atomic_bool listening = false;
    std::thread listeningThread;
};

}  // namespace iqrf::connector
