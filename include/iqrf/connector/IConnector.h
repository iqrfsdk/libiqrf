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
#include <mutex>
#include <stdexcept>
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

    [[nodiscard]] uint8_t osVersionMajor() const { return osVersion >> 4; }

    [[nodiscard]] uint8_t osVersionMinor() const { return osVersion & 0x0F; }
};
#pragma pack(pop)

/**
 * Access Token to guard exclusive access to the connector.
 */
class AccessToken {
 public:
  // Disable copying
  AccessToken(const AccessToken&) = delete;
  AccessToken& operator=(const AccessToken&) = delete;

  // Allow move (transfer of the access)
  AccessToken(AccessToken&&) = default;
  AccessToken& operator=(AccessToken&&) = default;

  /**
   * Get the level of access for this token.
   */
  [[nodiscard]] AccessType getAccessType() const {
    return accessType;
  }


 private:
  // Make the class instantiable only by the IConnector
  friend class IConnector;
  explicit AccessToken(const AccessType accessType): accessType(accessType) {}

  /**
   * The level of access this token bears.
   */
  AccessType accessType;
};


/**
 * Interface for IQRF Transceiver module connectors.
 */
class IConnector {
 public:
    /**
     * Constructs the connector and initializes the necessary resources (GPIO lines).
     */
    IConnector() {
      this->normalResponseHandler = ResponseHandler();
      this->exclusiveResponseHandler = ResponseHandler();
      this->snifferResponseHandler = ResponseHandler();
    }

    /**
     * Destructor to cleanly release the connection resources.
     */
    virtual ~IConnector() = default;

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
     * In order to send data, a ResponseHandler needs to be registered first.
     * Registering the handler yields the AccessToken used as an authenticator in this function.
     *
     * TODO: Used in Daemon(IqrfCdc, IqrfSpi, IqrfUart), clibspi, clibuart
     */
    void send(const std::vector<uint8_t>& data, const AccessToken token) {
      std::lock_guard<std::recursive_mutex> lock(this->guard);

      switch (token.getAccessType()) {
        case AccessType::Normal:
          if (this->hasExclusiveAccess()) {
            // TODO: Custom exceptions
            throw std::runtime_error("Cannot send: Exclusive access is active");
          }
          this->send(data);
          break;
        case AccessType::Exclusive:
          this->send(data);
          break;
        case AccessType::Sniffer:
          // TODO: Custom exceptions
          throw std::runtime_error("Cannot send: Sniffer token does not allow sending");
        default:
          break;
      }
    }

    /**
     * Read the data synchronously from the connector.
     *
     * TODO: Used in Uploader, clibspi, clibuart
     */
    virtual std::vector<uint8_t> receive() = 0;

    /**
     * Register the responseHandler for received messages.
     *
     * Upon successful register yields an AccessToken which can be used to access this channel.
     *
     * TODO: Used in Daemon(IqrfCdc, IqrfSpi, IqrfUart)
     */
    AccessToken registerResponseHandler(const ResponseHandler &responseHandler, const AccessType access) {
      std::lock_guard<std::recursive_mutex> lock(this->guard);

      switch (access) {
        case AccessType::Normal:
          if (this->normalResponseHandler) {
            // TODO: Send EOF to the previous handler
          }
          this->normalResponseHandler = responseHandler;
          break;
        case AccessType::Exclusive:
          if (this->hasExclusiveAccess()) {
            // TODO: Custom exceptions
            throw std::runtime_error("Exclusive access already assigned");
          }
          this->exclusiveResponseHandler = responseHandler;
          break;
        case AccessType::Sniffer:
          this->snifferResponseHandler = responseHandler;
          break;
        default:
            // TODO: Custom exceptions
            throw std::runtime_error("Invalid access type for response handler registration");
      }

      return AccessToken(access);
    }

    /**
     * Unregister the previously registered responseHandler.
     */
    void unregisterResponseHandler(const AccessToken token) {
      std::lock_guard<std::recursive_mutex> lock(this->guard);

      switch (token.getAccessType()) {
        case AccessType::Normal:
          this->normalResponseHandler = ResponseHandler();
          break;
        case AccessType::Exclusive:
          this->exclusiveResponseHandler = ResponseHandler();
          break;
        case AccessType::Sniffer:
          this->snifferResponseHandler = ResponseHandler();
          break;
        default:
          break;
      }
    }

    /**
     * Start the listening loop in a separate thread.
     *
     * TODO: Used in Daemon(IqrfCdc, IqrfSpi, IqrfUart)
     */
    void listen() {
      // TODO: Add some access control
      if (this->listening) {
        // TODO: Custom exceptions
        throw std::runtime_error("Listening loop already active");
      }

      this->listening = true;
      this->listeningThread = std::thread(&IConnector::listeningLoop, this);
    }

    /**
     * Checks whether the listening loop is running
     */
    bool isListening() const {
        return this->listening;
    }

    /**
     * Stops listening loop and joins thread
     */
    void stopListen() {
        this->listening = false;
        if (this->listeningThread.joinable()) {
            this->listeningThread.join();
        }
    }

    // Exclusive access

    /**
     * Check whether the connector is in exclusive access mode.
     *
     * TODO: Used in Daemon(IqrfCdc, IqrfSpi, IqrfUart)
     */
    bool hasExclusiveAccess() const {
      std::lock_guard<std::recursive_mutex> lock(this->guard);
      return static_cast<bool>(this->exclusiveResponseHandler);
    }

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
        const std::vector<uint8_t> &data
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
    void upload(
        const ProgrammingTarget target,
        const std::vector<uint8_t> &data,
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
     * Listen to the connector and asynchronously call responseHandler for
     * received messages.
     *
     * TODO: Used in Daemon(IqrfSpi)
     */
    void listeningLoop() {
        try {
            std::vector<uint8_t> recvBuffer;

            while (this->listening) {
                recvBuffer = this->receive();

                if (recvBuffer.empty()) {
                    // No data received, continue listening
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    continue;
                }

                std::lock_guard<std::recursive_mutex> lock(this->guard);

                if (this->hasExclusiveAccess()) {
                    this->exclusiveResponseHandler(recvBuffer);
                } else {
                   this->normalResponseHandler(recvBuffer);
                }

                if (this->snifferResponseHandler) {
                   this->snifferResponseHandler(recvBuffer);
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        } catch (...) {
            // TODO: Report error
            this->listening = false;
        }
    }

    /**
     * Send the data message directly via the connector.
     *
     * Unlike the two-parameter public version, this method does not perform access control.
     */
    virtual void send(const std::vector<uint8_t>& data) = 0;


 private:
  // Response handlers for managing the replies from Transceiver modules asynchronously
  // NOTE: Could be written as a map<AccessToken,ResponseHandler> to enable multiple access
  ResponseHandler normalResponseHandler;
  ResponseHandler exclusiveResponseHandler;
  ResponseHandler snifferResponseHandler;

  // Control variables for the listening loop
  std::atomic_bool listening = false;
  std::thread listeningThread;

  // Guards atomicity of connector operations
  mutable std::recursive_mutex guard;
};

}  // namespace iqrf::connector
