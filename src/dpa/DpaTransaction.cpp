/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: DpaTransaction.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-12
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */
#include "iqrf/dpa/DpaTransaction.h"

#include <chrono>
#include <exception>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "iqrf/dpa/DpaConstants.h"

namespace iqrf::dpa {

namespace {

/// Index of the number of hops in the confirmation
constexpr std::size_t CONFIRMATION_HOPS_INDEX = 8;
/// Index of the timeslot length in the confirmation
constexpr std::size_t CONFIRMATION_TIMESLOT_INDEX = 9;
/// Index of the number of response hops in the confirmation
constexpr std::size_t CONFIRMATION_HOPS_RESPONSE_INDEX = 10;

/// Minimal size of the confirmation
constexpr std::size_t CONFIRMATION_MIN_SIZE = 11;

/// Size of the response before the payload
constexpr int RESPONSE_OVERHEAD = static_cast<int>(DPA_HEADER_LENGTH + 2);

/**
 * Returns true if the request is addressed to the coordinator (or to the broadcast of the coordinator)
 */
bool isCoordinatorRequest(const DpaMessage &request) {
    return (request.nodeAddress() & BROADCAST_ADDRESS) == COORDINATOR_ADDRESS;
}

/**
 * Returns true if the command is a long running coordinator command with unbounded response time
 */
bool isInfiniteCoordinatorCommand(const DpaMessage &request) {
    return request.peripheralType() == PNUM_COORDINATOR &&
        (request.peripheralCommand() == CMD_COORDINATOR_DISCOVERY ||
         request.peripheralCommand() == CMD_COORDINATOR_SMART_CONNECT ||
         request.peripheralCommand() == CMD_COORDINATOR_AUTHORIZE_BOND);
}

/**
 * Returns true if the command is the FRC send
 */
bool isFrcSendRequest(const DpaMessage &request) {
    return request.peripheralType() == PNUM_FRC &&
        (request.peripheralCommand() == CMD_FRC_SEND || request.peripheralCommand() == CMD_FRC_SEND_SELECTIVE);
}

}  // namespace

DpaTransaction::DpaTransaction(const DpaMessage &request, const RfMode mode, const TimingParams params,
    const int32_t defaultTimeout, const int32_t userTimeout, SendFunc sender, const int defaultError)
    : result(std::make_unique<DpaTransactionResult>(request)),
      currentMode(mode),
      timing(params),
      sender(std::move(sender)),
      defaultError(defaultError),
      defaultTimeoutMs(defaultTimeout),
      userTimeoutMs(defaultTimeout),
      expectedDurationMs(defaultTimeout) {
    // check and correct timeout here before blocking:
    const DpaMessage &message = this->result->getRequest();
    int32_t requiredTimeout = userTimeout;

    // user timeout is 0 (infinite) or negative (default); the default timeout is used first
    if (requiredTimeout <= INFINITE_TIMEOUT) {
        requiredTimeout = defaultTimeout;
        // Discovery or SmartConnect or Authorize or FRC command ?

        if (isCoordinatorRequest(message)) {
            // Set default (infinite) timeout for Discovery or SmartConnect or Authorize
            if (isInfiniteCoordinatorCommand(message)) {
                this->infiniteTimeout = true;
            } else if (isFrcSendRequest(message)) {
                // FRC send: maximum FRC request timeout
                requiredTimeout = DEFAULT_FRC_TIMEOUT;
            }
        }
    } else if (requiredTimeout < defaultTimeout) {
        // user value lower than the default timeout is raised to the default one
        requiredTimeout = defaultTimeout;
    }

    // init expected duration - no estimation yet, so use default timeout
    this->expectedDurationMs = this->defaultTimeoutMs;
    if (isCoordinatorRequest(message)) {
        if (requiredTimeout > defaultTimeout) {
            this->expectedDurationMs = requiredTimeout;
        }
        // bonding special timeout
        if (message.peripheralType() == PNUM_COORDINATOR && message.peripheralCommand() == CMD_COORDINATOR_BOND_NODE) {
            if (userTimeout < 0) {
                // user timeout is not applied, forced to BOND_TIMEOUT_MS
                requiredTimeout = BOND_TIMEOUT_MS;
                this->expectedDurationMs = requiredTimeout;
            }
        }
    }
    this->userTimeoutMs = requiredTimeout;
}

//-----------------------------------------------------
// abort the transaction immediately
void DpaTransaction::abort() {
    const std::scoped_lock lock(this->mutex);
    this->state = State::Aborted;
    this->condition.notify_all();
}

std::unique_ptr<DpaTransactionResult> DpaTransaction::get() {
    // wait for transaction end: the lock is released while blocked, the predicate is checked on each wakeup
    std::unique_lock lock(this->mutex);
    this->condition.wait(lock, [this] { return this->finished; });
    // return result and move ownership
    return std::move(this->result);
}

void DpaTransaction::execute() {
    this->execute(this->defaultError);
}

void DpaTransaction::execute(const int defaultError) {
    // lock this function except blocking in wait_for()
    std::unique_lock lock(this->mutex);
    this->defaultError = defaultError;
    const DpaMessage &message = this->result->getRequest();

    // init transaction state
    if (this->defaultError == static_cast<int>(TransactionError::Ok)) {
        this->state = isCoordinatorRequest(message) ? State::SentCoordinator : State::Sent;
        try {
            // send request toward coordinator via send functor
            this->sender(message);
            // now we can expect handling in processReceivedMessage()
        } catch (const std::exception &) {
            // we have final error state - just finish transaction
            this->expectedDurationMs = 0;
            this->state = State::InterfaceError;
        }
    } else {
        // transaction is not handled, we have final error state - just finish transaction
        this->state = State::DefaultError;
        this->expectedDurationMs = 0;
    }
    // 1st notification to get() - we started transaction
    this->condition.notify_one();

    int errorCode = static_cast<int>(TransactionError::Iface);
    bool finish = true;
    bool expired = false;
    do {
        finish = true;  // end this cycle
        expired = false;
        if (this->expectedDurationMs > 0) {
            // wait_for() unlock lck when blocking and lock it again when get out, processReceivedMessage() is able
            // to do its job as it can lock now
            const auto duration = std::chrono::milliseconds(this->expectedDurationMs);
            if (this->condition.wait_for(lock, duration) == std::cv_status::timeout) {
                // out of wait on timeout
                expired = true;
            }
            // out of wait on notify from processReceivedMessage()
        }
        // evaluate state
        switch (this->state) {
            case State::Sent:
            case State::SentCoordinator:
            case State::Confirmation:
                if (expired && !this->infiniteTimeout) {
                    this->state = State::Timeout;
                    errorCode = static_cast<int>(TransactionError::Timeout);
                } else {
                    finish = false;
                }
                break;
            case State::ConfirmationTempAddr:
            case State::ConfirmationBroadcast:
            case State::ReceivedResponse:
                if (expired) {
                    this->state = State::Processed;
                    errorCode = static_cast<int>(TransactionError::Ok);
                } else {
                    finish = false;
                }
                break;
            case State::Processed:
                errorCode = static_cast<int>(TransactionError::Ok);
                break;
            case State::Timeout:
                errorCode = static_cast<int>(TransactionError::Timeout);
                break;
            case State::Aborted:
                errorCode = static_cast<int>(TransactionError::Aborted);
                break;
            case State::InterfaceError:
                errorCode = static_cast<int>(TransactionError::Iface);
                break;
            case State::DefaultError:
                errorCode = this->defaultError;
                break;
            case State::Created:
                errorCode = static_cast<int>(TransactionError::Iface);
                break;
        }
    } while (!finish);

    // update error code in result
    this->result->setErrorCode(errorCode);
    // signalize final state
    this->finished = true;
    // 2nd notification to get()
    this->condition.notify_all();
}

void DpaTransaction::processReceivedMessage(const DpaMessage &receivedMessage) {
    // lock this function
    std::unique_lock lock(this->mutex);
    // check transaction state
    if (this->finished) {
        return;  // nothing to do, just double check
    }
    // check massage validity: no request is expected, the same request as sent is expected

    const DpaMessage::Direction direction = receivedMessage.direction();
    if (direction != DpaMessage::Direction::Response && direction != DpaMessage::Direction::Confirmation) {
        throw std::logic_error("Response is expected.");
    }

    const DpaMessage &request = this->result->getRequest();
    if (receivedMessage.nodeAddress() != request.nodeAddress()) {
        throw std::logic_error("Different node address than in sent message.");
    }
    if (receivedMessage.peripheralType() != request.peripheralType()) {
        throw std::logic_error("Different peripheral type than in sent message.");
    }
    if ((receivedMessage.peripheralCommand() & static_cast<uint8_t>(~RESPONSE_FLAG)) != request.peripheralCommand()) {
        throw std::logic_error("Different peripheral command than in sent message.");
    }

    int32_t estimatedTimeMs = 0;
    if (direction == DpaMessage::Direction::Confirmation) {
        // process confirmation
        const std::vector<uint8_t> bytes = receivedMessage.bytes();
        if (bytes.size() < CONFIRMATION_MIN_SIZE) {
            throw std::logic_error("Confirmation is too short.");
        }
        const uint8_t address = receivedMessage.nodeAddress() & BROADCAST_ADDRESS;
        if (address == BROADCAST_ADDRESS) {
            this->state = State::ConfirmationBroadcast;
        } else if (address == TEMPORARY_ADDRESS) {
            this->state = State::ConfirmationTempAddr;
        } else {
            this->state = State::Confirmation;
        }

        this->hops = bytes[CONFIRMATION_HOPS_INDEX];
        this->timeslotLength = bytes[CONFIRMATION_TIMESLOT_INDEX];
        this->hopsResponse = bytes[CONFIRMATION_HOPS_RESPONSE_INDEX];

        if (this->currentMode == RfMode::Lp) {
            estimatedTimeMs = this->estimateLpTimeout(this->hops, this->timeslotLength, this->hopsResponse);
        } else {
            estimatedTimeMs = this->estimateStdTimeout(this->hops, this->timeslotLength, this->hopsResponse);
        }
        // setting timeout based on the confirmation
        if (estimatedTimeMs > 0) {
            // user wants to wait more than estimated so keep the wish
            this->expectedDurationMs = estimatedTimeMs >= this->userTimeoutMs ? estimatedTimeMs : this->userTimeoutMs;
        }
        this->result->setConfirmation(receivedMessage);
    } else {
        // process response
        if (this->state == State::SentCoordinator) {
            // if there was a request to coordinator then after receiving response it is allowed to send another
            this->state = State::Processed;
        } else if (!this->infiniteTimeout) {
            // only if there is not infinite timeout: wait for the expected time of the response
            this->state = State::ReceivedResponse;
            const int responseDataLength = static_cast<int>(receivedMessage.size()) - RESPONSE_OVERHEAD;
            if (this->currentMode == RfMode::Lp) {
                estimatedTimeMs = this->estimateLpTimeout(this->hops, this->timeslotLength, this->hopsResponse,
                    responseDataLength);
            } else {
                estimatedTimeMs = this->estimateStdTimeout(this->hops, this->timeslotLength, this->hopsResponse,
                    responseDataLength);
            }
            this->expectedDurationMs = estimatedTimeMs;
            if (this->expectedDurationMs <= 0) {
                // done, next request gets ready
                this->state = State::Processed;
            }
        } else {
            // infinite timeout: done, next request gets ready
            this->state = State::Processed;
        }
        this->result->setResponse(receivedMessage);
    }
    // notification to execute() and get()
    this->condition.notify_all();
}

int32_t DpaTransaction::estimateStdTimeout(const uint8_t hopsRequest, const uint8_t timeslotReq,
    const uint8_t hopsResponse, const int responseDataLength) const {
    int32_t responseTimeSlotLengthMs = 0;
    // estimation from confirmation
    int32_t estimatedTimeoutMs = ((hopsRequest + 1) * timeslotReq) * 10;
    if (responseDataLength == -1) {
        // worst case
        responseTimeSlotLengthMs = timeslotReq == 20 ? 200 : 60;
    } else if (this->timing.osBuild >= 0x08C8) {
        // correction of the estimation from response, OS 4.03D
        if (responseDataLength < 17) {
            responseTimeSlotLengthMs = 40;
        } else if (responseDataLength < 41) {
            responseTimeSlotLengthMs = 50;
        } else {
            responseTimeSlotLengthMs = 60;
        }
    } else {
        // correction of the estimation from response, OS 4.02D (default)
        if (responseDataLength < 16) {
            responseTimeSlotLengthMs = 40;
        } else if (responseDataLength < 40) {
            responseTimeSlotLengthMs = 50;
        } else {
            responseTimeSlotLengthMs = 60;
        }
    }
    estimatedTimeoutMs += ((hopsResponse + 1) * responseTimeSlotLengthMs) + SAFETY_TIMEOUT_MS;
    return estimatedTimeoutMs;
}

int32_t DpaTransaction::estimateLpTimeout(const uint8_t hopsRequest, const uint8_t timeslotReq,
    const uint8_t hopsResponse, const int responseDataLength) const {
    int32_t responseTimeSlotLengthMs = 0;
    // estimation from confirmation
    int32_t estimatedTimeoutMs = ((hopsRequest + 1) * timeslotReq) * 10;
    if (responseDataLength == -1) {
        responseTimeSlotLengthMs = timeslotReq == 20 ? 200 : 110;
    } else if (this->timing.osBuild >= 0x08C8) {
        // correction of the estimation from response, OS 4.03D
        if (responseDataLength < 17) {
            responseTimeSlotLengthMs = 80;
        } else if (responseDataLength < 41) {
            responseTimeSlotLengthMs = 90;
        } else {
            responseTimeSlotLengthMs = 100;
        }
    } else {
        // correction of the estimation from response, OS 4.02D (default)
        if (responseDataLength < 11) {
            responseTimeSlotLengthMs = 80;
        } else if (responseDataLength < 34) {
            responseTimeSlotLengthMs = 90;
        } else if (responseDataLength < 57) {
            responseTimeSlotLengthMs = 100;
        } else {
            responseTimeSlotLengthMs = 110;
        }
    }
    estimatedTimeoutMs += ((hopsResponse + 1) * responseTimeSlotLengthMs) + SAFETY_TIMEOUT_MS;
    return estimatedTimeoutMs;
}

}  // namespace iqrf::dpa
