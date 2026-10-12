/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: DpaMessage.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-12
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */
#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "iqrf/dpa/DpaConstants.h"

namespace iqrf::dpa {

/**
 * DPA message: request, confirmation or response of the DPA interface.
 *
 * Layout of the bytes: NADR (2 B, little endian), PNUM, PCMD, HWPID (2 B, little endian), then the payload.
 * The confirmation and the response have the response code and the DPA value after the header.
 *
 * The message is a value type with a fixed buffer, no dynamic memory is allocated.
 */
class DpaMessage {
 public:
    /// Size of the buffer for a message
    static constexpr std::size_t MAX_SIZE = 64;

    /// Values that represent message types
    enum class Direction {
        /// Request message
        Request,
        /// Confirmation message
        Confirmation,
        /// Response message
        Response
    };

    /**
     * Constructs an empty message
     */
    DpaMessage() = default;

    /**
     * Constructor from data (fills the message from the bytes received from the IQRF network)
     * Constructs the message from the bytes
     * @param data Bytes of the message, at most 64 B
     * @throws std::length_error if the data are longer than 64 B
     */
    explicit DpaMessage(const std::vector<uint8_t> &data) {
        if (data.size() > MAX_SIZE) {
            throw std::length_error("DPA message is too long");
        }
        std::copy(data.begin(), data.end(), this->buffer.begin());
        this->length = data.size();
    }

    /**
     * Constructs the request
     * @param nodeAddress Address of the node
     * @param peripheralType Peripheral number (PNUM)
     * @param peripheralCommand Command (PCMD)
     * @param hwpid Hardware profile ID
     * @param payload DPA message payload
     * @throws std::length_error if the message is longer than 64 B
     */
    DpaMessage(const uint16_t nodeAddress, const uint8_t peripheralType, const uint8_t peripheralCommand,
        const uint16_t hwpid, const std::vector<uint8_t> &payload)
        : DpaMessage(makeRequest(nodeAddress, peripheralType, peripheralCommand, hwpid, payload)) {}

    /**
     * Gets message type
     * @return Request, confirmation or response
     */
    [[nodiscard]] Direction direction() const {
        if (this->length <= COMMAND_INDEX) {
            return Direction::Request;
        }
        if ((this->buffer[COMMAND_INDEX] & RESPONSE_FLAG) != 0) {
            return Direction::Response;
        }
        const bool hasResponseCode = this->length > RESPONSE_CODE_INDEX;
        if (hasResponseCode && this->buffer[RESPONSE_CODE_INDEX] == static_cast<uint8_t>(DpaStatus::Confirmation)) {
            return Direction::Confirmation;
        }
        return Direction::Request;
    }

    /**
     * Gets destination or source address of sender/receiver
     * @return An address
     */
    [[nodiscard]] uint16_t nodeAddress() const {
        return static_cast<uint16_t>(this->buffer[0] | (this->buffer[1] << 8));
    }

    /**
     * Returns the peripheral number (PNUM)
     */
    [[nodiscard]] uint8_t peripheralType() const {
        return this->buffer[2];
    }

    /**
     * Gets command code (PCMD), including the response flag
     * @return A peripheral command
     */
    [[nodiscard]] uint8_t peripheralCommand() const {
        return this->buffer[COMMAND_INDEX];
    }

    /**
     * Gets response code from received message, valid for the confirmation and the response only
     * @return A response code
     */
    [[nodiscard]] uint8_t responseCode() const {
        return this->buffer[RESPONSE_CODE_INDEX];
    }

    /**
     * Gets length of data stored in message
     * @return Number of bytes in message
     */
    [[nodiscard]] std::size_t size() const {
        return this->length;
    }

    /**
     * Gets pointer to data stored in message (as a copy of the bytes)
     * @return Bytes of the message
     */
    [[nodiscard]] std::vector<uint8_t> bytes() const {
        return {this->buffer.begin(), this->buffer.begin() + static_cast<std::ptrdiff_t>(this->length)};
    }

 private:
    /// Index of the PCMD
    static constexpr std::size_t COMMAND_INDEX = 3;

    static std::vector<uint8_t> makeRequest(const uint16_t nodeAddress, const uint8_t peripheralType,
        const uint8_t peripheralCommand, const uint16_t hwpid, const std::vector<uint8_t> &payload) {
        std::vector<uint8_t> data = {
            static_cast<uint8_t>(nodeAddress & 0xFF),
            static_cast<uint8_t>(nodeAddress >> 8),
            peripheralType,
            peripheralCommand,
            static_cast<uint8_t>(hwpid & 0xFF),
            static_cast<uint8_t>(hwpid >> 8)
        };
        data.insert(data.end(), payload.begin(), payload.end());
        return data;
    }

    /// Bytes of the message
    std::array<uint8_t, MAX_SIZE> buffer{};
    /// Number of bytes of the message
    std::size_t length = 0;
};

}  // namespace iqrf::dpa
