/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: DpaTransactionResult.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-12
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */
#pragma once

#include <chrono>
#include <string>
#include <utility>

#include "iqrf/dpa/DpaConstants.h"
#include "iqrf/dpa/DpaMessage.h"

namespace iqrf::dpa {

/**
 * Error codes of the DPA transaction. The values between USER_FROM and USER_TO are user codes,
 * the values 0 - 0x3F are the DPA response codes.
 */
enum class TransactionError : int {
    // transaction handling
    ExclusiveAccess = -8,
    BadResponse = -7,
    BadRequest = -6,
    IfaceBusy = -5,
    Iface = -4,
    Aborted = -3,
    QueueFull = -2,
    Timeout = -1,
    Ok = 0,
    // DPA response errors
    Fail = 1,
    Pcmd = 2,
    Pnum = 3,
    Addr = 4,
    DataLen = 5,
    Data = 6,
    Hwpid = 7,
    Nadr = 8,
    IfaceCustomHandler = 9,
    MissingCustomDpaHandler = 10,
    UserFrom = 0x20,
    UserTo = 0x3F,
    StatusConfirmation = 0xFF
};

/**
 * Returns the name of the error code
 * @param errorCode Error code
 * @return Name of the error code, the hexadecimal value for user codes
 */
inline std::string transactionErrorName(const int errorCode) {
    switch (static_cast<TransactionError>(errorCode)) {
        case TransactionError::ExclusiveAccess: return "ERROR_IFACE_EXCLUSIVE_ACCESS";
        case TransactionError::BadResponse: return "BAD_RESPONSE";
        case TransactionError::BadRequest: return "BAD_REQUEST";
        case TransactionError::IfaceBusy: return "ERROR_IFACE_BUSY";
        case TransactionError::Iface: return "ERROR_IFACE";
        case TransactionError::Aborted: return "ERROR_ABORTED";
        case TransactionError::QueueFull: return "ERROR_IFACE_QUEUE_FULL";
        case TransactionError::Timeout: return "ERROR_TIMEOUT";
        case TransactionError::Ok: return "ok";
        case TransactionError::Fail: return "ERROR_FAIL";
        case TransactionError::Pcmd: return "ERROR_PCMD";
        case TransactionError::Pnum: return "ERROR_PNUM";
        case TransactionError::Addr: return "ERROR_ADDR";
        case TransactionError::DataLen: return "ERROR_DATA_LEN";
        case TransactionError::Data: return "ERROR_DATA";
        case TransactionError::Hwpid: return "ERROR_HWPID";
        case TransactionError::Nadr: return "ERROR_NADR";
        case TransactionError::IfaceCustomHandler: return "ERROR_IFACE_CUSTOM_HANDLER";
        case TransactionError::MissingCustomDpaHandler: return "ERROR_MISSING_CUSTOM_DPA_HANDLER";
        case TransactionError::UserTo: return "ERROR_USER_TO";
        case TransactionError::StatusConfirmation: return "STATUS_CONFIRMATION";
        default: break;
    }
    return "TRN_ERROR_USER_" + std::to_string(errorCode);
}

/**
 * Result of the DPA transaction: the request, its confirmation and response with the timestamps
 */
class DpaTransactionResult {
 public:
    using TimePoint = std::chrono::system_clock::time_point;

    /**
     * Constructs the result of the request, the request timestamp is set to the current time
     * @param request Request of the transaction
     */
    explicit DpaTransactionResult(DpaMessage request): request(request) {
        this->requestTs = std::chrono::system_clock::now();
    }

    /**
     * Returns the error code, the response code is returned if the transaction succeeded
     * (the transaction error code is not TRN_OK)
     */
    [[nodiscard]] int errorCode() const {
        if (this->error != static_cast<int>(TransactionError::Ok)) {
            return this->error;
        }
        return this->responseCodeValue;
    }

    /**
     * Overrides the error code
     * @param errorCode Error code
     */
    void overrideErrorCode(const int errorCode) {
        this->error = errorCode;
    }

    /**
     * Sets the error code, the response code is used if the transaction succeeded but the response failed
     * @param errorCode Error code
     */
    void setErrorCode(int errorCode) {
        if (errorCode == static_cast<int>(TransactionError::Ok) &&
            this->responseCodeValue != static_cast<int>(TransactionError::Ok)) {
            errorCode = this->responseCodeValue;
        }
        this->error = errorCode;
    }

    /**
     * Returns the name of the error code
     */
    [[nodiscard]] std::string errorName() const {
        return transactionErrorName(this->error);
    }

    /// original request creating the transaction
    [[nodiscard]] const DpaMessage& getRequest() const { return this->request; }
    [[nodiscard]] const DpaMessage& getConfirmation() const { return this->confirmation; }
    [[nodiscard]] const DpaMessage& getResponse() const { return this->response; }
    [[nodiscard]] const TimePoint& getRequestTs() const { return this->requestTs; }
    [[nodiscard]] const TimePoint& getConfirmationTs() const { return this->confirmationTs; }
    [[nodiscard]] const TimePoint& getResponseTs() const { return this->responseTs; }
    [[nodiscard]] bool isConfirmed() const { return this->confirmed; }
    [[nodiscard]] bool isResponded() const { return this->responded; }

    /**
     * Sets the confirmation of the request
     * @param confirmationMessage Confirmation
     */
    void setConfirmation(const DpaMessage &confirmationMessage) {
        this->confirmationTs = std::chrono::system_clock::now();
        this->confirmation = confirmationMessage;
        this->confirmed = true;
    }

    /**
     * Sets the response of the request
     * @param responseMessage Response
     */
    void setResponse(const DpaMessage &responseMessage) {
        this->responseTs = std::chrono::system_clock::now();
        this->response = responseMessage;
        if (responseMessage.size() > 0) {
            this->responseCodeValue = responseMessage.responseCode();
            this->responded = true;
        } else {
            this->responded = false;
        }
    }

 private:
    /// original request creating the transaction
    DpaMessage request;
    /// received confirmation
    DpaMessage confirmation;
    /// received response
    DpaMessage response;
    /// request timestamp
    TimePoint requestTs;
    /// confirmation timestamp
    TimePoint confirmationTs;
    /// response timestamp
    TimePoint responseTs;
    /// overall error code
    int error = static_cast<int>(TransactionError::Aborted);
    /// response code
    int responseCodeValue = static_cast<int>(TransactionError::Ok);
    /// received and set response flag
    bool responded = false;
    /// received and set confirmation flag
    bool confirmed = false;
};

}  // namespace iqrf::dpa
