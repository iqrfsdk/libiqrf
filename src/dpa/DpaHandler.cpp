/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: DpaHandler.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-12
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */
#include "iqrf/dpa/DpaHandler.h"

#include <cstddef>
#include <exception>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "iqrf/dpa/DpaConstants.h"
#include "iqrf/log/Logging.h"

namespace iqrf::dpa {

namespace log = ::iqrf::log;

DpaHandler::DpaHandler(iqrf::connector::IConnector &connector): connector(connector) {
    // initialize m_FrcTimingParams
    this->timingParams.bondedNodes = 1;
    this->timingParams.discoveredNodes = 1;
    this->timingParams.osBuild = 0x08B8;
    this->timingParams.dpaVersion = 0x0302;
    this->timingParams.frcResponseTime = FrcResponseTime::k40Ms;

    this->accessToken = this->connector.registerResponseHandler(
        [this](const std::vector<uint8_t> &message) { return this->onReceived(message); },
        iqrf::connector::AccessType::Normal
    );
    // the queue worker executes the transactions one by one
    this->transactionQueue = std::make_unique<TaskQueue<std::shared_ptr<DpaTransaction>>>(
        [this](const std::shared_ptr<DpaTransaction> &transaction) { this->processTransaction(transaction); }
    );
}

DpaHandler::~DpaHandler() {
    // No more messages are delivered to the handler, the connector waits for the handler in progress
    if (this->accessToken.has_value()) {
        this->connector.unregisterResponseHandler(*this->accessToken);
    }
    // kill DpaTransaction if any; the queue is stopped first, so the next transaction is not started after the
    // pending one is aborted
    this->transactionQueue->stop();
    std::shared_ptr<DpaTransaction> pending;
    {
        const std::scoped_lock lock(this->pendingMutex);
        pending = this->pendingTransaction;
    }
    if (pending) {
        pending->abort();
    }
    this->transactionQueue.reset();
}

std::shared_ptr<DpaTransaction> DpaHandler::executeDpaTransaction(const DpaMessage &request, const int32_t timeout,
    const int defaultError) {
    if (request.size() == 0) {
        throw std::invalid_argument("DPA request must not be empty.");
    }
    RfMode mode = RfMode::Std;
    TimingParams params;
    int32_t defaultTimeoutMs = DEFAULT_TIMEOUT;
    {
        const std::scoped_lock lock(this->settingsMutex);
        mode = this->rfMode;
        params = this->timingParams;
        defaultTimeoutMs = this->defaultTimeout;
    }
    auto transaction = std::make_shared<DpaTransaction>(request, mode, params, defaultTimeoutMs, timeout,
        [this](const DpaMessage &message) { this->sendRequest(message); }, defaultError);
    this->transactionQueue->push(transaction);
    return transaction;
}

int DpaHandler::getTimeout() const {
    const std::scoped_lock lock(this->settingsMutex);
    return this->defaultTimeout;
}

void DpaHandler::setTimeout(int timeout) {
    if (timeout < MINIMAL_TIMEOUT) {
        IQRF_LOG(log::Level::Warning) << "Timeout " << timeout << " ms is too low, it is set to " << MINIMAL_TIMEOUT
            << " ms";
        timeout = MINIMAL_TIMEOUT;
    }
    const std::scoped_lock lock(this->settingsMutex);
    this->defaultTimeout = timeout;
}

RfMode DpaHandler::getRfCommunicationMode() const {
    const std::scoped_lock lock(this->settingsMutex);
    return this->rfMode;
}

void DpaHandler::setRfCommunicationMode(const RfMode mode) {
    const std::scoped_lock lock(this->settingsMutex);
    this->rfMode = mode;
}

TimingParams DpaHandler::getTimingParams() const {
    const std::scoped_lock lock(this->settingsMutex);
    return this->timingParams;
}

void DpaHandler::setTimingParams(const TimingParams params) {
    const std::scoped_lock lock(this->settingsMutex);
    this->timingParams = params;
}

FrcResponseTime DpaHandler::getFrcResponseTime() const {
    const std::scoped_lock lock(this->settingsMutex);
    return this->timingParams.frcResponseTime;
}

void DpaHandler::setFrcResponseTime(const FrcResponseTime frcResponseTime) {
    const std::scoped_lock lock(this->settingsMutex);
    this->timingParams.frcResponseTime = frcResponseTime;
}

void DpaHandler::registerAsyncMessageHandler(const std::string &serviceId, MessageHandlerFunc fun) {
    // serviceId is not used, a single handler is kept as in the original implementation
    static_cast<void>(serviceId);
    const std::scoped_lock lock(this->asyncMutex);
    this->asyncMessageHandler = std::move(fun);
}

void DpaHandler::unregisterAsyncMessageHandler(const std::string &serviceId) {
    // serviceId is not used, see registerAsyncMessageHandler()
    static_cast<void>(serviceId);
    const std::scoped_lock lock(this->asyncMutex);
    this->asyncMessageHandler = nullptr;
}

void DpaHandler::registerAnyMessageHandler(const std::string &serviceId, MessageHandlerFunc fun) {
    const std::scoped_lock lock(this->anyMutex);
    if (!this->anyMessageHandlers.emplace(serviceId, std::move(fun)).second) {
        throw std::logic_error("Already registered: " + serviceId);
    }
}

void DpaHandler::unregisterAnyMessageHandler(const std::string &serviceId) {
    const std::scoped_lock lock(this->anyMutex);
    this->anyMessageHandlers.erase(serviceId);
}

int DpaHandler::getDpaQueueLen() const {
    return static_cast<int>(this->transactionQueue->size());
}

int DpaHandler::onReceived(const std::vector<uint8_t> &message) {
    // any received message from the connector
    if (message.empty()) {
        return 0;
    }
    // incoming message
    DpaMessage received;
    try {
        received = DpaMessage(message);
    } catch (const std::length_error &e) {
        IQRF_LOG(log::Level::Warning) << "Discarding received DPA message: " << e.what();
        return 0;
    }

    // process any message handler for special handling before transaction processing
    this->processAnyMessage(received);

    const DpaMessage::Direction direction = received.direction();
    // async msg: a request from the node or an asynchronous response
    // Always Async
    const bool asyncResponse = direction == DpaMessage::Direction::Response &&
        (received.responseCode() & static_cast<uint8_t>(DpaStatus::AsyncResponse)) != 0;
    if (direction == DpaMessage::Direction::Request || asyncResponse) {
        this->processAsyncMessage(received);
        return 0;
    }

    std::shared_ptr<DpaTransaction> pending;
    {
        const std::scoped_lock lock(this->pendingMutex);
        pending = this->pendingTransaction;
    }
    if (!pending) {
        return 0;
    }
    // confirmation or response of the pending transaction
    try {
        pending->processReceivedMessage(received);
    } catch (const std::logic_error &e) {
        IQRF_LOG(log::Level::Warning) << "Processing of received DPA message failed: " << e.what();
    }
    return 0;
}

void DpaHandler::processAsyncMessage(const DpaMessage &message) {
    MessageHandlerFunc handler;
    {
        const std::scoped_lock lock(this->asyncMutex);
        handler = this->asyncMessageHandler;
    }
    if (handler) {
        handler(message);
    }
}

void DpaHandler::processAnyMessage(const DpaMessage &message) {
    std::map<std::string, MessageHandlerFunc> handlers;
    {
        const std::scoped_lock lock(this->anyMutex);
        handlers = this->anyMessageHandlers;
    }
    for (const auto &[serviceId, handler] : handlers) {
        static_cast<void>(serviceId);
        handler(message);
    }
}

void DpaHandler::sendRequest(const DpaMessage &request) {
    try {
        this->connector.send(request.bytes(), *this->accessToken);
    } catch (const std::exception &e) {
        IQRF_LOG(log::Level::Warning) << "Cannot send DPA message to the coordinator: " << e.what();
        throw;
    }
}

void DpaHandler::processTransaction(const std::shared_ptr<DpaTransaction> &transaction) {
    // the transaction is pending until it is finished
    {
        const std::scoped_lock lock(this->pendingMutex);
        this->pendingTransaction = transaction;
    }
    // maximal queue length - can be prolonged if it make sense
    if (this->transactionQueue->size() < QUEUE_MAX_LEN) {
        transaction->execute();  // succesfully queued
    } else {
        // queue full transaction not handled, error reported
        IQRF_LOG(log::Level::Warning) << "Transaction queue overload: " << this->transactionQueue->size();
        transaction->execute(static_cast<int>(TransactionError::QueueFull));
    }
}

}  // namespace iqrf::dpa
