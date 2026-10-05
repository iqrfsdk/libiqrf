/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: SpiProtocol.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-05
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include "iqrf/connector/spi/SpiProtocol.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace iqrf::connector::spi {

namespace {

/// Initial value of CRCM and CRCS
constexpr uint8_t CRC_INIT = 0x5F;
/// CTYPE bit of PTYPE - master writes data to TR module
constexpr uint8_t PTYPE_CTYPE_BUFFER_CHANGED = 0x80;
/// Lowest SPI status value signaling ready data
constexpr uint8_t STATUS_DATA_READY_MIN = 0x40;
/// Highest SPI status value signaling ready data
constexpr uint8_t STATUS_DATA_READY_MAX = 0x7F;
/// Size of internal EEPROM
constexpr std::size_t EEPROM_SIZE = 256;
/// Alignment of Flash write address [words]
constexpr uint16_t FLASH_WRITE_ALIGNMENT = 16;
/// Alignment of Flash read (verify) address [words]
constexpr uint16_t FLASH_READ_ALIGNMENT = 32;
/// Offset of external EEPROM virtual addresses in .HEX file
constexpr uint16_t EEEPROM_VIRTUAL_OFFSET = 0x0200;
/// Highest external EEPROM write index address
constexpr uint16_t EEEPROM_WRITE_INDEX_MAX = 0x01FF;
/// Lowest external EEPROM read index address
constexpr uint16_t EEEPROM_READ_INDEX_MIN = 0x0400;
/// Highest external EEPROM read index address
constexpr uint16_t EEEPROM_READ_INDEX_MAX = 0x05FF;

/**
 * SPI command and its payload (data part of SPI packet)
 */
struct SpiCommand {
    /// SPI command
    uint8_t cmd = 0;
    /// Payload
    std::vector<uint8_t> payload;
};

/**
 * Decodes 2 B little-endian address from the beginning of upload data
 * @param data Upload data
 * @return Address
 */
uint16_t decodeAddress(const std::vector<uint8_t> &data) {
    return static_cast<uint16_t>(data[0] | (data[1] << 8));
}

/**
 * Builds command for writing TR configuration parameter (RFPGM, RF band, access password, user key)
 * @param parameter Parameter address (DM1)
 * @param data Parameter value
 * @param length Required length of the parameter value
 * @param error Error message for invalid length
 * @return SPI command
 */
SpiCommand configParameterUpload(
    const uint8_t parameter,
    const std::vector<uint8_t> &data,
    const std::size_t length,
    const char *error
) {
    if (data.size() != length) {
        throw std::invalid_argument(error);
    }
    SpiCommand command{SpiProtocol::CMD_WRITE_EEPROM, {parameter, static_cast<uint8_t>(length)}};
    command.payload.insert(command.payload.end(), data.begin(), data.end());
    return command;
}

/**
 * Builds command for writing data to Flash
 * @param data Virtual address (modulo 16) followed by 32 B of data
 * @return SPI command
 */
SpiCommand flashUpload(const std::vector<uint8_t> &data) {
    if (data.size() != 2 + SpiProtocol::MEMORY_BLOCK_LENGTH) {
        throw std::invalid_argument("Flash upload requires address and exactly 32 B of data");
    }
    if (decodeAddress(data) % FLASH_WRITE_ALIGNMENT != 0) {
        throw std::invalid_argument("Flash upload address must be modulo 16");
    }
    return {SpiProtocol::CMD_FLASH_EEEPROM, data};
}

/**
 * Builds command for writing data to internal EEPROM
 * @param data Address (only the lower byte is used) followed by 1 - 32 B of data
 * @return SPI command
 */
SpiCommand internalEepromUpload(const std::vector<uint8_t> &data) {
    if (data.size() < 3 || data.size() > 2 + SpiProtocol::MEMORY_BLOCK_LENGTH) {
        throw std::invalid_argument("Internal EEPROM upload requires address and 1 - 32 B of data");
    }
    // Only the low byte of address is used for addressing internal EEPROM
    const uint8_t address = data[0];
    const std::size_t length = data.size() - 2;
    if (address + length > EEPROM_SIZE) {
        throw std::invalid_argument("Internal EEPROM upload exceeds the EEPROM size");
    }
    SpiCommand command{SpiProtocol::CMD_WRITE_EEPROM, {address, static_cast<uint8_t>(length)}};
    command.payload.insert(command.payload.end(), data.begin() + 2, data.end());
    return command;
}

/**
 * Builds command for writing data to external EEPROM
 * @param data Virtual address from .HEX file (0x0200 - 0x41FF, modulo 32) followed by 32 B of data
 * @return SPI command
 */
SpiCommand externalEepromUpload(const std::vector<uint8_t> &data) {
    if (data.size() != 2 + SpiProtocol::MEMORY_BLOCK_LENGTH) {
        throw std::invalid_argument("External EEPROM upload requires address and exactly 32 B of data");
    }
    const uint16_t address = decodeAddress(data);
    const bool aligned = (address - EEEPROM_VIRTUAL_OFFSET) % SpiProtocol::MEMORY_BLOCK_LENGTH == 0;
    if (address < EEEPROM_VIRTUAL_OFFSET || !aligned) {
        throw std::invalid_argument("External EEPROM upload address must be at least 0x0200 and modulo 32");
    }
    // External EEPROM is written by 32 B blocks addressed by index
    const auto block = static_cast<uint16_t>((address - EEEPROM_VIRTUAL_OFFSET) / SpiProtocol::MEMORY_BLOCK_LENGTH);
    if (block > EEEPROM_WRITE_INDEX_MAX) {
        throw std::invalid_argument("External EEPROM upload address is out of range");
    }
    SpiCommand command{
        SpiProtocol::CMD_FLASH_EEEPROM,
        {static_cast<uint8_t>(block & 0xFF), static_cast<uint8_t>(block >> 8)},
    };
    command.payload.insert(command.payload.end(), data.begin() + 2, data.end());
    return command;
}

}  // namespace

std::optional<SpiStatus> SpiStatus::parse(const uint8_t raw) {
    if (raw >= STATUS_DATA_READY_MIN && raw <= STATUS_DATA_READY_MAX) {
        // 0x40 means 64 B of data, 0x41 - 0x7F means 1 - 63 B of data
        const uint8_t length = raw == STATUS_DATA_READY_MIN ? 64 : raw - STATUS_DATA_READY_MIN;
        return SpiStatus(raw, length);
    }
    switch (static_cast<SpiStatusValue>(raw)) {
        case SpiStatusValue::Disabled:
        case SpiStatusValue::Suspended:
        case SpiStatusValue::BufferProtect:
        case SpiStatusValue::CrcmError:
        case SpiStatusValue::ReadyCommunication:
        case SpiStatusValue::ReadyProgramming:
        case SpiStatusValue::ReadyDebug:
        case SpiStatusValue::SlowMode:
        case SpiStatusValue::HwError:
            return SpiStatus(raw, 0);
        default:
            return std::nullopt;
    }
}

uint8_t SpiProtocol::ptype(const std::size_t length, const bool bufferChanged) {
    auto result = static_cast<uint8_t>(length & 0x7F);
    if (bufferChanged) {
        result |= PTYPE_CTYPE_BUFFER_CHANGED;
    }
    return result;
}

uint8_t SpiProtocol::crcm(const uint8_t cmd, const uint8_t ptype, const std::vector<uint8_t> &data) {
    uint8_t crc = CRC_INIT ^ cmd ^ ptype;
    for (const uint8_t byte : data) {
        crc ^= byte;
    }
    return crc;
}

uint8_t SpiProtocol::crcs(const uint8_t ptype, const std::vector<uint8_t> &data) {
    uint8_t crc = CRC_INIT ^ ptype;
    for (const uint8_t byte : data) {
        crc ^= byte;
    }
    return crc;
}

void SpiProtocol::checkDataLength(const std::size_t length) {
    if (length == 0 || length > MAX_DATA_LENGTH) {
        throw std::invalid_argument(
            "Invalid SPI data length: " + std::to_string(length) + " (allowed 1 - " +
            std::to_string(MAX_DATA_LENGTH) + ")"
        );
    }
}

std::vector<uint8_t> SpiProtocol::buildPacket(
    const uint8_t cmd,
    const std::vector<uint8_t> &data,
    const bool bufferChanged,
    const bool trailingByte
) {
    SpiProtocol::checkDataLength(data.size());
    const uint8_t packetType = SpiProtocol::ptype(data.size(), bufferChanged);
    std::vector<uint8_t> packet;
    packet.reserve(data.size() + 4);
    packet.push_back(cmd);
    packet.push_back(packetType);
    packet.insert(packet.end(), data.begin(), data.end());
    packet.push_back(SpiProtocol::crcm(cmd, packetType, data));
    if (trailingByte) {
        packet.push_back(0x00);
    }
    return packet;
}

std::vector<uint8_t> SpiProtocol::buildWritePacket(const std::vector<uint8_t> &data) {
    return SpiProtocol::buildPacket(CMD_DATA, data, true, true);
}

std::vector<uint8_t> SpiProtocol::buildReadPacket(const std::size_t length) {
    SpiProtocol::checkDataLength(length);
    return SpiProtocol::buildPacket(CMD_DATA, std::vector<uint8_t>(length, 0), false, true);
}

std::optional<std::vector<uint8_t>> SpiProtocol::parseReadResponse(
    const std::size_t length,
    const std::vector<uint8_t> &response
) {
    // CMD + PTYPE + DATA + CRCS
    if (response.size() < length + 3) {
        return std::nullopt;
    }
    const auto first = response.begin() + 2;
    std::vector<uint8_t> data(first, first + static_cast<std::ptrdiff_t>(length));
    if (response[length + 2] != SpiProtocol::crcs(SpiProtocol::ptype(length, false), data)) {
        return std::nullopt;
    }
    return data;
}

std::vector<uint8_t> SpiProtocol::buildTrInfoPacket(const std::size_t length) {
    if (length != TrInfo::BASIC_LENGTH && length != TrInfo::EXTENDED_LENGTH) {
        throw std::invalid_argument("Module Info length must be 16 or 32 B");
    }
    return SpiProtocol::buildPacket(CMD_TR_MODULE_INFO, std::vector<uint8_t>(length, 0), false, true);
}

std::vector<uint8_t> SpiProtocol::buildUploadPacket(
    const ProgrammingTarget target,
    const std::vector<uint8_t> &data
) {
    SpiCommand command;
    switch (target) {
        case ProgrammingTarget::Rfpgm:
            command = configParameterUpload(0xC1, data, 1, "RFPGM upload requires exactly 1 B of data");
            break;
        case ProgrammingTarget::RfBand:
            command = configParameterUpload(0xC0, data, 1, "RF band upload requires exactly 1 B of data");
            break;
        case ProgrammingTarget::AccessPassword:
            command = configParameterUpload(0xD0, data, 16, "Access password upload requires exactly 16 B of data");
            break;
        case ProgrammingTarget::UserKey:
            command = configParameterUpload(0xD1, data, 16, "User key upload requires exactly 16 B of data");
            break;
        case ProgrammingTarget::Flash:
            command = flashUpload(data);
            break;
        case ProgrammingTarget::InternalEeprom:
            command = internalEepromUpload(data);
            break;
        case ProgrammingTarget::ExternalEeprom:
            command = externalEepromUpload(data);
            break;
        case ProgrammingTarget::Special:
            command = {CMD_UPLOAD_IQRF, data};
            break;
        case ProgrammingTarget::Config:
            throw std::invalid_argument(
                "Configuration upload is not supported, it must be split into separate memory uploads"
            );
        default:
            throw std::invalid_argument("Unsupported upload target");
    }
    return SpiProtocol::buildPacket(command.cmd, command.payload, true, true);
}

bool SpiProtocol::isCrcmConfirmed(const std::vector<uint8_t> &response) {
    return !response.empty() && response.back() == static_cast<uint8_t>(SpiStatusValue::BufferProtect);
}

std::vector<uint8_t> SpiProtocol::buildDownloadPacket(const ProgrammingTarget target, const uint16_t address) {
    const auto addressLow = static_cast<uint8_t>(address & 0xFF);
    const auto addressHigh = static_cast<uint8_t>(address >> 8);
    switch (target) {
        case ProgrammingTarget::Rfpgm:
        case ProgrammingTarget::RfBand:
            // Both values are stored in the same EEPROM block
            return SpiProtocol::buildPacket(CMD_READ_EEPROM, {0xC0, 0x00}, true, true);
        case ProgrammingTarget::Flash:
            if (address % FLASH_READ_ALIGNMENT != 0) {
                throw std::invalid_argument("Flash download address must be modulo 32");
            }
            return SpiProtocol::buildPacket(CMD_READ_FLASH, {addressLow, addressHigh}, true, true);
        case ProgrammingTarget::InternalEeprom:
            // Only the low byte of address is used for addressing internal EEPROM
            return SpiProtocol::buildPacket(CMD_READ_EEPROM, {addressLow, 0x00}, true, true);
        case ProgrammingTarget::ExternalEeprom:
            if (address < EEEPROM_READ_INDEX_MIN || address > EEEPROM_READ_INDEX_MAX) {
                throw std::invalid_argument("External EEPROM download index address must be in range 0x0400 - 0x05FF");
            }
            return SpiProtocol::buildPacket(CMD_FLASH_EEEPROM, {addressLow, addressHigh}, true, true);
        case ProgrammingTarget::Config:
            throw std::invalid_argument(
                "Configuration download is not supported, it must be split into separate memory downloads"
            );
        case ProgrammingTarget::AccessPassword:
        case ProgrammingTarget::UserKey:
        case ProgrammingTarget::Special:
        default:
            throw std::invalid_argument("Unsupported download target");
    }
}

std::vector<uint8_t> SpiProtocol::parseDownloadResponse(
    const ProgrammingTarget target,
    const std::vector<uint8_t> &block
) {
    switch (target) {
        case ProgrammingTarget::Rfpgm:
            return {block.at(1)};
        case ProgrammingTarget::RfBand:
            return {block.at(0)};
        default:
            return block;
    }
}

}  // namespace iqrf::connector::spi
