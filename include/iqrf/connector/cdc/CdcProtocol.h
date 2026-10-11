/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: CdcProtocol.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-12
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "iqrf/connector/IConnector.h"

namespace iqrf::connector::cdc {

/**
 * Error of the CDC frame, the received bytes do not form a valid frame
 */
class CdcFrameError : public std::runtime_error {
 public:
    using std::runtime_error::runtime_error;
};

/**
 * Frame received from the USB CDC device
 *
 * Frame has the form "<HH:BODY\r" (HH is the header), the asynchronous data frame "<DR" has the form "<DRL:DATA\r"
 * where L is the length of DATA.
 */
struct CdcFrame {
    /// Header of the frame (e.g. "DS", "PM", "DR")
    std::string header;
    /// Body of the frame (data without the header, the separator and the terminator)
    std::vector<uint8_t> body;
};

/**
 * USB device identification, response of the I command
 */
struct UsbDeviceInfo {
    /// Device type (e.g. GW-USB-03)
    std::string type;
    /// Firmware version (e.g. 02.01)
    std::string version;
    /// Serial number (e.g. 03010000)
    std::string id;
};

/**
 * Command and response encoding of the USB CDC device (CDC Implementation in IQRF USB devices User Guide)
 */
namespace protocol {

/// Terminator of the commands and responses
constexpr uint8_t TERMINATOR = 0x0D;

/// Maximum length of the data sent to the TR module (DS command), 41 B on older devices, 64 B on GW-USB-05 and CK-USB-04A
constexpr std::size_t MAX_SEND_LENGTH = 64;

/**
 * Builds the command without any parameters (e.g. IT, RT, PE, PT)
 * @param header Header of the command
 * @return Command bytes
 */
std::vector<uint8_t> buildCommand(const std::string &header);

/**
 * Builds the DS command (send data to the TR module)
 * @param data Data to send, at most 64 B
 * @return Command bytes
 * @throws std::invalid_argument if the data is too long
 */
std::vector<uint8_t> buildSendCommand(const std::vector<uint8_t> &data);

/**
 * Returns the PM command target byte for the upload operation
 * @param target Programming target
 * @return Target byte with the write flag set
 * @throws std::invalid_argument if the target cannot be uploaded
 */
uint8_t uploadTarget(ProgrammingTarget target);

/**
 * Returns the PM command target byte for the download operation
 * @param target Programming target
 * @return Target byte with the read flag cleared
 * @throws std::invalid_argument if the target cannot be downloaded
 */
uint8_t downloadTarget(ProgrammingTarget target);

/**
 * Builds the PM command (upload data to the TR module in programming mode)
 * @param target Programming target
 * @param data Data to upload
 * @return Command bytes
 * @throws std::invalid_argument if the target cannot be uploaded
 */
std::vector<uint8_t> buildUploadCommand(ProgrammingTarget target, const std::vector<uint8_t> &data);

/**
 * Builds the PM command (download data from the TR module in programming mode)
 * @param target Programming target
 * @param address Optional address of the Flash or EEPROM memory, sent as little endian
 * @return Command bytes
 * @throws std::invalid_argument if the target cannot be downloaded
 */
std::vector<uint8_t> buildDownloadCommand(ProgrammingTarget target, std::optional<uint16_t> address);

/**
 * Extracts the next complete frame from the received bytes, the bytes of the frame are removed from the buffer.
 *
 * Bytes preceding the start of the frame are discarded.
 *
 * @param buffer Received bytes
 * @return The frame, nullopt if the buffer does not contain the complete frame yet
 * @throws CdcFrameError if the buffer contains invalid frame, the buffer is left unchanged
 */
std::optional<CdcFrame> extractFrame(std::vector<uint8_t> &buffer);

/**
 * Returns the length of the data returned by the PM download command
 * @param target Programming target
 * @return Length of the downloaded data
 * @throws std::invalid_argument if the target cannot be downloaded
 */
std::size_t downloadLength(ProgrammingTarget target);

/**
 * Extracts the next complete frame of the PM download response, the bytes of the frame are removed from the buffer.
 *
 * The downloaded data are returned without any header, their length is known from the request. The status
 * response "<PM:STATUS\r" (or "<PC:BUSY\r") is recognized as well. Other frames are extracted by extractFrame().
 *
 * @param buffer Received bytes
 * @param dataLength Length of the downloaded data
 * @return The frame, nullopt if the buffer does not contain the complete frame yet
 * @throws CdcFrameError if the buffer contains invalid frame, the buffer is left unchanged
 */
std::optional<CdcFrame> extractDownloadFrame(std::vector<uint8_t> &buffer, std::size_t dataLength);

/**
 * Parses the body of the I response "<I:TYPE#VERSION#ID\r"
 * @param body Body of the response
 * @return USB device identification
 * @throws std::runtime_error if the body has invalid format
 */
UsbDeviceInfo parseUsbDeviceInfo(const std::vector<uint8_t> &body);

/**
 * Checks the status of the DS, PE, PT, PM and RT responses
 * @param frame Response frame
 * @return The status text of the response (e.g. "OK", "ERR", "BUSY")
 */
std::string statusOf(const CdcFrame &frame);

}  // namespace protocol

}  // namespace iqrf::connector::cdc
