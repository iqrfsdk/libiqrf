/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: DpaTransaction.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-12
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */
#pragma once

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>

#include "iqrf/dpa/DpaMessage.h"
#include "iqrf/dpa/DpaTransactionResult.h"

namespace iqrf::dpa {

/// Communication mode of the RF
enum class RfMode {
    /// standard communication
    Std,
    /// low power communication
    Lp
};

/// Response time of the FRC (fast response command)
enum class FrcResponseTime : uint8_t {
    k40Ms = 0x00,
    k360Ms = 0x10,
    k680Ms = 0x20,
    k1320Ms = 0x30,
    k2600Ms = 0x40,
    k5160Ms = 0x50,
    k10280Ms = 0x60,
    k20620Ms = 0x70
};

/// Parameters used to estimate the timeouts of the transactions
struct TimingParams {
    /// Number of bonded nodes
    uint8_t bondedNodes = 1;
    /// Number of discovered nodes
    uint8_t discoveredNodes = 1;
    /// Response time of the FRC
    FrcResponseTime frcResponseTime = FrcResponseTime::k40Ms;
    /// IQRF OS build of the coordinator
    uint16_t osBuild = 0x08B8;
    /// DPA version of the coordinator
    uint16_t dpaVersion = 0x0302;
};

// Timing constants
/// Default timeout
constexpr int32_t DEFAULT_TIMEOUT = 500;
/// Minimal timeout used if required by user is too low
constexpr int32_t MINIMAL_TIMEOUT = 200;
/// Zero value used to indicate infinit timeout in special cases (discovery)
constexpr int32_t INFINITE_TIMEOUT = 0;
/// An extra timeout added to timeout from a confirmation packet.
constexpr int32_t SAFETY_TIMEOUT_MS = 40;
/// A special DISCOVERY timeout
constexpr int32_t BOND_TIMEOUT_MS = 11000;
/// Maximum FRC request timeout
constexpr uint32_t DEFAULT_FRC_TIMEOUT = 120000;

/**
 * DPA transaction: sends the request and waits for the confirmation and the response
 *
 * This is key timing class. Its methods execute(), get(), processReceivedMessage() runs concurently and has to be
 * synchronized.
 *
 * User calls:
 *   std::shared_ptr<IDpaTransaction2> dt = dpa->executeDpaTransaction(request);
 *   std::unique_ptr<IDpaTransactionResult2> dtr = dt->get();
 *
 * When user invokes dpa->executeDpaTransaction() this class is constructed and stored by the handler to the queue
 * of transactions. If there is no pending transaction the first one from the queue is executed by the DpaHandler
 * queue worker thread. If the function is not invoked in time required by user, the transaction timer expires and
 * this situation is reported via transaction result error code (coordinator busy or queue full).
 *
 * execute() gets to point waiting for confirmation or response from coordinator. These messages are handled by
 * processReceivedMessage(). The function computes expected time set appropriate transaction state and pass control
 * back to execute() to continue or finish the transaction. When transaction is finish the function execute() pass
 * control to get() and it returns transaction result to user.
 */
class DpaTransaction {
 public:
    /// type of functor to send the request message towards the coordinator
    using SendFunc = std::function<void(const DpaMessage &)>;

    /**
     * Constructs the transaction
     * @param request Request to send
     * @param mode Communication mode of the RF
     * @param params Timing parameters
     * @param defaultTimeout Default timeout in ms
     * @param userTimeout Timeout required by the user in ms, negative value means the default timeout
     * @param sender Function sending the request
     * @param defaultError Error reported instead of sending the request
     */
    DpaTransaction(const DpaMessage &request, RfMode mode, TimingParams params, int32_t defaultTimeout,
        int32_t userTimeout, SendFunc sender, int defaultError);

    DpaTransaction(const DpaTransaction&) = delete;
    DpaTransaction& operator=(const DpaTransaction&) = delete;
    DpaTransaction(DpaTransaction&&) = delete;
    DpaTransaction& operator=(DpaTransaction&&) = delete;

    ~DpaTransaction() = default;

    /**
     * Aborts the transaction immediately
     */
    void abort();

    /**
     * Waits for the end of the transaction, wait for result
     * @return Result of the transaction
     */
    std::unique_ptr<DpaTransactionResult> get();

    /**
     * Sends the request and waits for the end of the transaction, called by the queue of the handler
     */
    void execute();

    /**
     * Sends the request and waits for the end of the transaction
     * @param defaultError Error reported instead of sending the request if not TRN_OK
     */
    void execute(int defaultError);

    /**
     * Processes the message received from the interface
     * @param receivedMessage Confirmation or response of the request
     * @throws std::logic_error if the message does not belong to the request
     */
    void processReceivedMessage(const DpaMessage &receivedMessage);

 private:
    /// Values that represent transaction state.
    enum class State {
        /// init state
        Created,
        /// request was sent.
        Sent,
        /// request with coordinator address was sent.
        SentCoordinator,
        /// confirmation was received.
        Confirmation,
        /// confirmation temporary was received.
        ConfirmationTempAddr,
        /// confirmation broadcast was received.
        ConfirmationBroadcast,
        /// response was received wait for expected time
        ReceivedResponse,
        /// transaction was processed.
        Processed,
        /// timeout expired.
        Timeout,
        /// transaction forced to abort.
        Aborted,
        /// error state during sending via iqrf interface.
        InterfaceError,
        /// error state enforced at the beginning of the transaction
        DefaultError
    };

    /**
     * Estimates the timeout of the standard RF communication
     * @param hopsRequest Number of hops of the request
     * @param timeslotReq Timeslot length of the request in 10 ms
     * @param hopsResponse Number of hops of the response
     * @param responseDataLength Length of the response data, -1 if no response
     * @return Estimated timeout in ms
     */
    [[nodiscard]] int32_t estimateStdTimeout(uint8_t hopsRequest, uint8_t timeslotReq, uint8_t hopsResponse,
        int responseDataLength = -1) const;

    /**
     * Estimates the timeout of the low power RF communication
     * @param hopsRequest Number of hops of the request
     * @param timeslotReq Timeslot length of the request in 10 ms
     * @param hopsResponse Number of hops of the response
     * @param responseDataLength Length of the response data, -1 if no response
     * @return Estimated timeout in ms
     */
    [[nodiscard]] int32_t estimateLpTimeout(uint8_t hopsRequest, uint8_t timeslotReq, uint8_t hopsResponse,
        int responseDataLength = -1) const;

    /// Result object tobe returned when the transaction finishes
    std::unique_ptr<DpaTransactionResult> result;
    /// transaction state
    State state = State::Created;
    /// signalize final state
    bool finished = false;
    /// actual communication mode
    RfMode currentMode;
    /// actual timing params
    TimingParams timing;
    /// functor to send the request message towards the coordinator
    SendFunc sender;
    /// Error reported instead of sending the request
    int defaultError;
    /// set form configuration
    int32_t defaultTimeoutMs;
    /// required by user
    int32_t userTimeoutMs;
    /// expected duration of the next state (estimated from the confirmation and response)
    int32_t expectedDurationMs;
    /// True for requests waiting for the response of the coordinator
    bool infiniteTimeout = false;
    /// iqrf structure info to estimate transaction processing time (number of hops of the confirmation)
    uint8_t hops = 0;
    /// timeslot length of the confirmation in 10 ms
    uint8_t timeslotLength = 0;
    /// number of hops of the response
    uint8_t hopsResponse = 0;
    /// mutex to protect shared variables controlled by condition variable
    std::mutex mutex;
    /// condition used to wait for confirmation and response messages from coordinator
    std::condition_variable condition;
};

}  // namespace iqrf::dpa
