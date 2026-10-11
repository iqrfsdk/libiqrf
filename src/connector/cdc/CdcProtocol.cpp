/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: CdcProtocol.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-12
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */
#include "iqrf/connector/cdc/CdcProtocol.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "iqrf/connector/IConnector.h"

namespace iqrf::connector::cdc::protocol {

namespace {

/// Command prefix
constexpr uint8_t COMMAND_START = '>';
/// Response prefix
constexpr uint8_t RESPONSE_START = '<';
/// Separator of the header and the body
constexpr uint8_t SEPARATOR = ':';

/// Length of the TR module information frame with the basic (16 B) body
constexpr std::size_t TR_INFO_STANDARD_SIZE = 21;
/// Length of the TR module information frame with the extended (32 B, including IBK) body
constexpr std::size_t TR_INFO_EXTENDED_SIZE = 37;

/**
 * Creates the command ">HEADER" followed by the payload and the terminator
 */
std::vector<uint8_t> makeCommand(const std::string &header, const std::vector<uint8_t> &payload) {
    std::vector<uint8_t> command;
    command.reserve(header.size() + payload.size() + 2);
    command.push_back(COMMAND_START);
    command.insert(command.end(), header.begin(), header.end());
    command.insert(command.end(), payload.begin(), payload.end());
    command.push_back(TERMINATOR);
    return command;
}

/// Communication failure of the asynchronous data, "<DR:ERR\r"
constexpr std::string_view ASYNC_ERROR = ":ERR\r";

/**
 * Checks whether the buffer starts with the asynchronous communication failure "<DR:ERR\r" (possibly incomplete)
 *
 * Asynchronous data frame has the length in the position of the separator if the length is 58 B.
 */
bool startsWithAsyncError(const std::vector<uint8_t> &buffer) {
    constexpr std::size_t POSITION = 3;
    constexpr std::size_t LENGTH = ASYNC_ERROR.size();
    if (buffer.size() <= POSITION || buffer[POSITION] != SEPARATOR) {
        return false;
    }
    const std::size_t available = std::min(buffer.size() - POSITION, LENGTH);
    return std::equal(buffer.begin() + POSITION, buffer.begin() + POSITION + static_cast<std::ptrdiff_t>(available),
        ASYNC_ERROR.begin());
}

/**
 * Checks whether the text is a status of the PM response
 */
bool isPmStatus(const std::string &text) {
    return text == "OK" || text == "ERR2" || text == "ERR3" || text == "ERR4" || text == "ERR5" ||
        text == "ERR6" || text == "ERR7" || text == "BUSY";
}

/**
 * Extracts the asynchronous data frame "<DRL:DATA\r", where L is the length of DATA
 *
 * DATA may contain the terminator, so the frame is delimited by the length.
 */
std::optional<CdcFrame> extractAsyncData(const std::vector<uint8_t> &buffer, std::size_t &consumed) {
    constexpr std::size_t LENGTH_POSITION = 3;
    constexpr std::size_t SEPARATOR_POSITION = 4;
    constexpr std::size_t DATA_POSITION = 5;
    // Frame overhead: '<', 'D', 'R', length, ':', '\r'
    constexpr std::size_t OVERHEAD = 6;
    if (buffer.size() <= LENGTH_POSITION) {
        return std::nullopt;
    }
    const std::size_t frameSize = OVERHEAD + buffer[LENGTH_POSITION];
    if (buffer.size() < frameSize) {
        return std::nullopt;
    }
    if (buffer[SEPARATOR_POSITION] != SEPARATOR || buffer[frameSize - 1] != TERMINATOR) {
        throw CdcFrameError("Invalid asynchronous data frame");
    }
    CdcFrame frame;
    frame.header = "DR";
    frame.body.assign(buffer.begin() + DATA_POSITION, buffer.begin() + static_cast<std::ptrdiff_t>(frameSize - 1));
    consumed = frameSize;
    return frame;
}

/**
 * Extracts the TR module information frame "<IT:BODY\r", where BODY is 16 B or 32 B (including IBK)
 *
 * BODY may contain the terminator, so the frame is delimited by its known length.
 */
std::optional<CdcFrame> extractTrInfo(const std::vector<uint8_t> &buffer, std::size_t &consumed) {
    constexpr std::size_t SEPARATOR_POSITION = 3;
    constexpr std::size_t BODY_POSITION = 4;
    std::size_t frameSize = 0;
    if (buffer.size() >= TR_INFO_EXTENDED_SIZE && buffer[TR_INFO_EXTENDED_SIZE - 1] == TERMINATOR) {
        frameSize = TR_INFO_EXTENDED_SIZE;
    } else if (buffer.size() >= TR_INFO_STANDARD_SIZE && buffer[TR_INFO_STANDARD_SIZE - 1] == TERMINATOR) {
        frameSize = TR_INFO_STANDARD_SIZE;
    } else if (buffer.size() >= TR_INFO_EXTENDED_SIZE) {
        throw CdcFrameError("Invalid TR module information frame");
    } else {
        return std::nullopt;
    }
    if (buffer[SEPARATOR_POSITION] != SEPARATOR) {
        throw CdcFrameError("Invalid TR module information frame");
    }
    CdcFrame frame;
    frame.header = "IT";
    frame.body.assign(buffer.begin() + BODY_POSITION, buffer.begin() + static_cast<std::ptrdiff_t>(frameSize - 1));
    consumed = frameSize;
    return frame;
}

/**
 * Extracts the SPI status frame "<S:STATUS\r", the status is 1 B and may contain the terminator
 */
std::optional<CdcFrame> extractSpiStatus(const std::vector<uint8_t> &buffer, std::size_t &consumed) {
    constexpr std::size_t STATUS_POSITION = 3;
    constexpr std::size_t FRAME_SIZE = 5;
    if (buffer.size() < FRAME_SIZE) {
        return std::nullopt;
    }
    if (buffer[FRAME_SIZE - 1] != TERMINATOR) {
        throw CdcFrameError("Invalid SPI status frame");
    }
    CdcFrame frame;
    frame.header = "S";
    frame.body = {buffer[STATUS_POSITION]};
    consumed = FRAME_SIZE;
    return frame;
}

/**
 * Extracts the frame "<HEADER:BODY\r" (or "<HEADER\r" without the body), delimited by the terminator
 */
std::optional<CdcFrame> extractTerminated(const std::vector<uint8_t> &buffer, std::size_t &consumed) {
    const auto end = std::find(buffer.begin(), buffer.end(), TERMINATOR);
    if (end == buffer.end()) {
        return std::nullopt;
    }
    const auto headerEnd = std::find(buffer.begin() + 1, end, SEPARATOR);
    CdcFrame frame;
    frame.header.assign(buffer.begin() + 1, headerEnd);
    if (headerEnd != end) {
        frame.body.assign(headerEnd + 1, end);
    }
    consumed = static_cast<std::size_t>(end - buffer.begin()) + 1;
    return frame;
}

}  // namespace

std::vector<uint8_t> buildCommand(const std::string &header) {
    return makeCommand(header, {});
}

std::vector<uint8_t> buildSendCommand(const std::vector<uint8_t> &data) {
    if (data.size() > MAX_SEND_LENGTH) {
        throw std::invalid_argument("Data to send are too long for the CDC device");
    }
    std::vector<uint8_t> payload = {static_cast<uint8_t>(data.size()), SEPARATOR};
    payload.insert(payload.end(), data.begin(), data.end());
    return makeCommand("DS", payload);
}

uint8_t uploadTarget(const ProgrammingTarget target) {
    switch (target) {
        case ProgrammingTarget::Config:         return 0x80;
        case ProgrammingTarget::Rfpgm:          return 0x81;
        case ProgrammingTarget::RfBand:         return 0x82;
        case ProgrammingTarget::AccessPassword: return 0x83;
        case ProgrammingTarget::UserKey:        return 0x84;
        case ProgrammingTarget::Flash:          return 0x85;
        case ProgrammingTarget::InternalEeprom: return 0x86;
        case ProgrammingTarget::ExternalEeprom: return 0x87;
        case ProgrammingTarget::Special:        return 0x88;  // Plugin code
    }
    throw std::invalid_argument("Unsupported upload target");
}

uint8_t downloadTarget(const ProgrammingTarget target) {
    switch (target) {
        case ProgrammingTarget::Config:         return 0x00;
        case ProgrammingTarget::Rfpgm:          return 0x01;
        case ProgrammingTarget::RfBand:         return 0x02;
        case ProgrammingTarget::Flash:          return 0x05;
        case ProgrammingTarget::InternalEeprom: return 0x06;
        case ProgrammingTarget::ExternalEeprom: return 0x07;
        case ProgrammingTarget::AccessPassword:
        case ProgrammingTarget::UserKey:
        case ProgrammingTarget::Special:
            break;
    }
    throw std::invalid_argument("Unsupported download target");
}

std::vector<uint8_t> buildUploadCommand(const ProgrammingTarget target, const std::vector<uint8_t> &data) {
    std::vector<uint8_t> payload = {uploadTarget(target)};
    payload.insert(payload.end(), data.begin(), data.end());
    return makeCommand("PM", payload);
}

std::vector<uint8_t> buildDownloadCommand(const ProgrammingTarget target, const std::optional<uint16_t> address) {
    std::vector<uint8_t> payload = {downloadTarget(target)};
    if (address.has_value()) {
        payload.push_back(static_cast<uint8_t>(*address & 0xFF));
        payload.push_back(static_cast<uint8_t>(*address >> 8));
    }
    return makeCommand("PM", payload);
}

std::size_t downloadLength(const ProgrammingTarget target) {
    downloadTarget(target);  // Validates the target
    switch (target) {
        case ProgrammingTarget::Config:
        case ProgrammingTarget::Flash:
        case ProgrammingTarget::InternalEeprom:
        case ProgrammingTarget::ExternalEeprom:
            return 32;
        case ProgrammingTarget::Rfpgm:
        case ProgrammingTarget::RfBand:
            return 1;
        case ProgrammingTarget::AccessPassword:
        case ProgrammingTarget::UserKey:
        case ProgrammingTarget::Special:
            break;
    }
    throw std::invalid_argument("Unsupported download target");
}

std::optional<CdcFrame> extractFrame(std::vector<uint8_t> &buffer) {
    // Discard the bytes preceding the start of the frame (the whole buffer if there is no start)
    buffer.erase(buffer.begin(), std::find(buffer.begin(), buffer.end(), RESPONSE_START));
    if (buffer.size() < 3) {
        return std::nullopt;
    }
    const std::string header(buffer.begin() + 1, buffer.begin() + 3);
    std::size_t consumed = 0;
    std::optional<CdcFrame> frame;
    if (header == "DR" && startsWithAsyncError(buffer)) {
        constexpr std::size_t ERROR_SIZE = 3 + ASYNC_ERROR.size();
        if (buffer.size() < ERROR_SIZE) {
            return std::nullopt;
        }
        buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(ERROR_SIZE));
        return CdcFrame{"ERR", {}};
    }
    if (header == "DR") {
        frame = extractAsyncData(buffer, consumed);
    } else if (header == "IT") {
        frame = extractTrInfo(buffer, consumed);
    } else if (header == "S:") {
        frame = extractSpiStatus(buffer, consumed);
    } else {
        frame = extractTerminated(buffer, consumed);
    }
    if (frame.has_value()) {
        buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(consumed));
    }
    return frame;
}

std::optional<CdcFrame> extractDownloadFrame(std::vector<uint8_t> &buffer, const std::size_t dataLength) {
    // Discard the bytes preceding the start of the frame (the whole buffer if there is no start)
    buffer.erase(buffer.begin(), std::find(buffer.begin(), buffer.end(), RESPONSE_START));
    if (buffer.size() < 3 || buffer[1] != 'P' || (buffer[2] != 'M' && buffer[2] != 'C')) {
        return extractFrame(buffer);
    }
    constexpr std::size_t STATUS_SEARCH_SIZE = 9;  // "<PM:" + the longest status + '\r'
    constexpr std::size_t BODY_POSITION = 4;
    if (buffer.size() > 3 && buffer[3] == SEPARATOR) {
        const std::size_t searchSize = std::min(buffer.size(), STATUS_SEARCH_SIZE);
        const auto end = std::find(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(searchSize),
            TERMINATOR);
        if (end != buffer.begin() + static_cast<std::ptrdiff_t>(searchSize)) {
            const std::string status(buffer.begin() + BODY_POSITION, end);
            if (isPmStatus(status)) {
                buffer.erase(buffer.begin(), end + 1);
                return CdcFrame{"PM", std::vector<uint8_t>(status.begin(), status.end())};
            }
        } else if (buffer.size() < STATUS_SEARCH_SIZE) {
            // The status may be not complete yet
            return std::nullopt;
        }
    }
    // Downloaded data "<PM" + data + '\r', the data may contain the terminator
    const std::size_t frameSize = 3 + dataLength + 1;
    if (buffer.size() < frameSize) {
        return std::nullopt;
    }
    if (buffer[frameSize - 1] != TERMINATOR) {
        throw CdcFrameError("Invalid download frame");
    }
    CdcFrame frame;
    frame.header = "PM";
    frame.body.assign(buffer.begin() + 3, buffer.begin() + static_cast<std::ptrdiff_t>(frameSize - 1));
    buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(frameSize));
    return frame;
}

UsbDeviceInfo parseUsbDeviceInfo(const std::vector<uint8_t> &body) {
    const std::string text(body.begin(), body.end());
    const std::size_t first = text.find('#');
    const std::size_t second = first == std::string::npos ? std::string::npos : text.find('#', first + 1);
    if (second == std::string::npos || text.find('#', second + 1) != std::string::npos) {
        throw std::runtime_error("Invalid USB device information: " + text);
    }
    UsbDeviceInfo info;
    info.type = text.substr(0, first);
    info.version = text.substr(first + 1, second - first - 1);
    info.id = text.substr(second + 1);
    return info;
}

std::string statusOf(const CdcFrame &frame) {
    return {frame.body.begin(), frame.body.end()};
}

}  // namespace iqrf::connector::cdc::protocol
