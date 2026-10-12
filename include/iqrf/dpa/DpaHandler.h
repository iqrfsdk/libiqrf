/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: DpaHandler.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-12
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "iqrf/connector/IConnector.h"
#include "iqrf/dpa/DpaMessage.h"
#include "iqrf/dpa/DpaTransaction.h"
#include "iqrf/dpa/TaskQueue.h"

namespace iqrf::dpa {

/**
 * DPA handler: executes DPA transactions over the IQRF connector.
 *
 * The transactions are executed sequentially. Received messages are delivered to the handler by the response
 * handler registered in the connector, the connector must be listening (IConnector::listen()).
 *
 * The queue of transactions is processed by a worker thread, the transaction in progress blocks the next one.
 */
class DpaHandler {
 public:
    /// Asynchronous DPA message handler functional type, any DPA message handler functional type
    using MessageHandlerFunc = std::function<void(const DpaMessage &)>;
    /// Asynchronous DPA message handler functional type
    using AsyncMessageHandlerFunc = MessageHandlerFunc;
    /// Any DPA message handler functional type
    using AnyMessageHandlerFunc = MessageHandlerFunc;

    /**
     * Constructs the DPA handler and registers the response handler in the connector
     * @param connector IQRF connector, must outlive the handler
     */
    explicit DpaHandler(iqrf::connector::IConnector &connector);

    /**
     * Unregisters the response handler from the connector, aborts the pending transaction and stops the queue
     */
    ~DpaHandler();

    DpaHandler(const DpaHandler&) = delete;
    DpaHandler& operator=(const DpaHandler&) = delete;
    DpaHandler(DpaHandler&&) = delete;
    DpaHandler& operator=(DpaHandler&&) = delete;

    /**
     * Queues the DPA transaction and returns it immediately, the result is obtained by the transaction
     * 0 > timeout - use default, 0 == timeout - use infinit, 0 < timeout - user value
     * @param request Request to send, must not be empty
     * @param timeout Timeout required by the user in ms, negative value means the default timeout
     * @param defaultError Error reported instead of sending the request if not TRN_OK
     * @return The transaction
     * @throws std::invalid_argument if the request is empty
     */
    std::shared_ptr<DpaTransaction> executeDpaTransaction(const DpaMessage &request, int32_t timeout,
        int defaultError = static_cast<int>(TransactionError::Ok));

    /// Returns the default timeout in ms
    [[nodiscard]] int getTimeout() const;

    /// Sets the default timeout in ms, values lower than MINIMAL_TIMEOUT are raised to it
    void setTimeout(int timeout);

    /// Returns the communication mode of the RF
    [[nodiscard]] RfMode getRfCommunicationMode() const;

    /// Sets the communication mode of the RF
    void setRfCommunicationMode(RfMode rfMode);

    /// Returns the timing parameters
    [[nodiscard]] TimingParams getTimingParams() const;

    /// Sets the timing parameters
    void setTimingParams(TimingParams params);

    /// Returns the response time of the FRC
    [[nodiscard]] FrcResponseTime getFrcResponseTime() const;

    /// Sets the response time of the FRC
    void setFrcResponseTime(FrcResponseTime frcResponseTime);

    /**
     * Registers the handler of the asynchronous messages, replaces the previous one
     * @param serviceId Identifier of the service
     * @param fun Handler
     */
    void registerAsyncMessageHandler(const std::string &serviceId, MessageHandlerFunc fun);

    /// Unregisters the handler of the asynchronous messages
    void unregisterAsyncMessageHandler(const std::string &serviceId);

    /**
     * Registers the handler of all received messages
     * @param serviceId Identifier of the service, must be unique
     * @param fun Handler
     * @throws std::logic_error if the service is already registered
     */
    void registerAnyMessageHandler(const std::string &serviceId, MessageHandlerFunc fun);

    /// Unregisters the handler of all received messages
    void unregisterAnyMessageHandler(const std::string &serviceId);

    /// Returns the number of the transactions waiting in the queue
    [[nodiscard]] int getDpaQueueLen() const;

 private:
    /// maximal queue length - can be prolonged if it make sense
    static constexpr std::size_t QUEUE_MAX_LEN = 16;

    /**
     * Processes the message received from the connector
     * @param message Received bytes
     * @return 0 always
     */
    int onReceived(const std::vector<uint8_t> &message);

    /// Calls the handler of the asynchronous messages
    void processAsyncMessage(const DpaMessage &message);

    /// Calls the handlers of all messages
    void processAnyMessage(const DpaMessage &message);

    /// Sends the request, called by the transactions
    void sendRequest(const DpaMessage &request);

    /// Executes the transaction taken from the queue
    void processTransaction(const std::shared_ptr<DpaTransaction> &transaction);

    /// IQRF connector
    iqrf::connector::IConnector &connector;

    /// Access token of the response handler registered in the connector
    std::optional<iqrf::connector::AccessToken> accessToken;

    /// Guards the settings
    mutable std::mutex settingsMutex;
    /// Communication mode of the RF
    RfMode rfMode = RfMode::Std;
    /// Timing parameters
    TimingParams timingParams;
    /// Default timeout in ms
    int defaultTimeout = DEFAULT_TIMEOUT;

    /// Guards the asynchronous message handler
    std::mutex asyncMutex;
    /// Handler of the asynchronous messages
    MessageHandlerFunc asyncMessageHandler;

    /// Guards the handlers of all messages
    std::mutex anyMutex;
    /// Handlers of all messages by service
    std::map<std::string, MessageHandlerFunc> anyMessageHandlers;

    /// Guards the pending transaction
    std::mutex pendingMutex;
    /// Transaction being executed
    std::shared_ptr<DpaTransaction> pendingTransaction;

    /// Queue of the transactions, declared last so that it is destroyed first
    std::unique_ptr<TaskQueue<std::shared_ptr<DpaTransaction>>> transactionQueue;
};

}  // namespace iqrf::dpa
