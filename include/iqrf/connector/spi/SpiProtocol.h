/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: SpiProtocol.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-05
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "iqrf/connector/IConnector.h"

namespace iqrf::connector::spi {

/**
 * SPI status values of TR module which do not signal ready data
 * (see IQRF SPI Technical guide, chapter SPI status)
 */
enum class SpiStatusValue : uint8_t {
    /// SPI not active (disabled by the disableSPI() command)
    Disabled = 0x00,
    /// SPI suspended by the stopSPI() command
    Suspended = 0x07,
    /// SPI not ready (buffer full, last CRCM O.K.), data in bufferCOM is protected against overwriting
    BufferProtect = 0x3F,
    /// SPI not ready (buffer full, last CRCM error)
    CrcmError = 0x3E,
    /// SPI ready - communication mode
    ReadyCommunication = 0x80,
    /// SPI ready - programming mode
    ReadyProgramming = 0x81,
    /// SPI ready - debugging mode
    ReadyDebug = 0x82,
    /// SPI probably in slow communication mode
    SlowMode = 0x83,
    /// SPI not active (HW error)
    HwError = 0xFF,
};

/**
 * Parsed SPI status of TR module
 */
class SpiStatus {
 public:
    /**
     * Parses raw SPI status byte
     * @param raw Raw SPI status byte
     * @return Parsed SPI status, std::nullopt for an unknown status value
     */
    static std::optional<SpiStatus> parse(uint8_t raw);

    /**
     * Checks whether TR module has data ready to be read
     * @return true if data are ready
     */
    [[nodiscard]] bool isDataReady() const { return this->dataReadyLength > 0; }

    /**
     * Returns number of bytes ready to be read
     * @return Number of bytes ready to be read, 0 if no data are ready
     */
    [[nodiscard]] uint8_t getDataReadyLength() const { return this->dataReadyLength; }

    /**
     * Checks whether the status is equal to the specified status value
     * @param value Status value
     * @return true if the status is equal to the specified value
     */
    [[nodiscard]] bool is(SpiStatusValue value) const {
        return !this->isDataReady() && this->raw == static_cast<uint8_t>(value);
    }

    /**
     * Returns raw SPI status byte
     * @return Raw SPI status byte
     */
    [[nodiscard]] uint8_t getRaw() const { return this->raw; }

 private:
    SpiStatus(uint8_t raw, uint8_t dataReadyLength): raw(raw), dataReadyLength(dataReadyLength) {}

    /// Raw SPI status byte
    uint8_t raw;
    /// Number of bytes ready to be read
    uint8_t dataReadyLength;
};

/**
 * IQRF SPI protocol packet builder and parser
 *
 * Packet format: CMD | PTYPE | DATA... | CRCM [| SPI_CHECK]
 *
 * The optional trailing SPI_CHECK byte makes TR module return the result of CRCM verification
 * (0x3F - CRCM O.K., 0x3E - CRCM error) in the last received byte.
 *
 * See IQRF SPI Technical guide for TR-7xD and TR-7xG, chapters Packet structure and TR memories handling.
 */
class SpiProtocol {
 public:
    /// SPI check command (status read)
    static constexpr uint8_t CMD_CHECK = 0x00;
    /// SPI data read/write command
    static constexpr uint8_t CMD_DATA = 0xF0;
    /// Read from internal EEPROM (programming mode)
    static constexpr uint8_t CMD_READ_EEPROM = 0xF2;
    /// Write to internal EEPROM (programming mode)
    static constexpr uint8_t CMD_WRITE_EEPROM = 0xF3;
    /// Read TR module information
    static constexpr uint8_t CMD_TR_MODULE_INFO = 0xF5;
    /// Write to Flash, read/write external EEPROM (programming mode)
    static constexpr uint8_t CMD_FLASH_EEEPROM = 0xF6;
    /// Upload IQRF plug-in (programming mode)
    static constexpr uint8_t CMD_UPLOAD_IQRF = 0xF9;
    /// Read (verify) data in Flash (programming mode)
    static constexpr uint8_t CMD_READ_FLASH = 0xFC;

    /// Maximal length of data in a single SPI packet (SPIDLEN)
    static constexpr std::size_t MAX_DATA_LENGTH = 64;
    /// Length of the data block returned by download in programming mode
    static constexpr std::size_t DOWNLOAD_LENGTH = 32;
    /// Length of memory data block written to Flash and external EEPROM, maximal length for internal EEPROM
    static constexpr std::size_t MEMORY_BLOCK_LENGTH = 32;

    /**
     * Calculates PTYPE byte
     * @param length Data length
     * @param bufferChanged true if master writes data to TR module (CTYPE bit)
     * @return PTYPE
     */
    static uint8_t ptype(std::size_t length, bool bufferChanged);

    /**
     * Calculates CRCM (master checksum) of a packet
     * @param cmd SPI command
     * @param ptype PTYPE
     * @param data Packet data
     * @return CRCM
     */
    static uint8_t crcm(uint8_t cmd, uint8_t ptype, const std::vector<uint8_t> &data);

    /**
     * Calculates CRCS (slave checksum) of received data
     * @param ptype PTYPE
     * @param data Received data
     * @return CRCS
     */
    static uint8_t crcs(uint8_t ptype, const std::vector<uint8_t> &data);

    /**
     * Builds SPI packet
     * @param cmd SPI command
     * @param data Packet data
     * @param bufferChanged true if master writes data to TR module
     * @param trailingByte Append trailing SPI_CHECK byte to receive the result of CRCM verification
     * @return SPI packet
     */
    static std::vector<uint8_t> buildPacket(
        uint8_t cmd,
        const std::vector<uint8_t> &data,
        bool bufferChanged,
        bool trailingByte = false
    );

    /**
     * Builds packet for writing data to TR module (including trailing SPI_CHECK byte)
     * @param data Data to write
     * @return SPI packet
     * @throws std::invalid_argument for invalid data length
     */
    static std::vector<uint8_t> buildWritePacket(const std::vector<uint8_t> &data);

    /**
     * Builds packet for reading data from TR module (including trailing SPI_CHECK byte)
     * @param length Length of data to read
     * @return SPI packet
     * @throws std::invalid_argument for invalid data length
     */
    static std::vector<uint8_t> buildReadPacket(std::size_t length);

    /**
     * Extracts data from response to a read packet and verifies its CRCS
     * @param length Length of data
     * @param response Bytes received during read packet transfer
     * @return Data, std::nullopt on CRCS mismatch
     */
    static std::optional<std::vector<uint8_t>> parseReadResponse(
        std::size_t length,
        const std::vector<uint8_t> &response
    );

    /**
     * Builds packet for reading TR module information (Module Info)
     * @param length Length of Module Info block (16 B, or 32 B including IBK for IQRF OS v4.03 or higher)
     * @return SPI packet
     * @throws std::invalid_argument for invalid length
     */
    static std::vector<uint8_t> buildTrInfoPacket(std::size_t length = TrInfo::BASIC_LENGTH);

    /**
     * Builds packet for uploading data in programming mode
     *
     * Memory targets are prefixed by 2 B little-endian address:
     * - Flash: virtual address (modulo 16) followed by 32 B of data (16 instructions)
     * - Internal EEPROM: physical or virtual address (only the lower byte is used) followed by 1 - 32 B of data
     * - External EEPROM: virtual address from .HEX file (0x0200 - 0x41FF, modulo 32) followed by 32 B of data
     *
     * @param target Programming target
     * @param data Data to upload
     * @return SPI packet
     * @throws std::invalid_argument for unsupported target or invalid data
     */
    static std::vector<uint8_t> buildUploadPacket(ProgrammingTarget target, const std::vector<uint8_t> &data);

    /**
     * Checks that TR module confirmed CRCM of a packet with trailing SPI_CHECK byte
     * @param response Bytes received during packet transfer
     * @return true if CRCM was confirmed
     */
    static bool isCrcmConfirmed(const std::vector<uint8_t> &response);

    /**
     * Builds packet for download request in programming mode
     *
     * Address meaning depends on the target:
     * - Flash: virtual address (modulo 32)
     * - Internal EEPROM: physical or virtual address (only the lower byte is used)
     * - External EEPROM: read index address (0x0400 - 0x05FF), index = physical address / 0x20 + 0x0400
     * - RF band and RFPGM: ignored
     *
     * @param target Programming target
     * @param address Memory address
     * @return SPI packet
     * @throws std::invalid_argument for unsupported target
     */
    static std::vector<uint8_t> buildDownloadPacket(ProgrammingTarget target, uint16_t address);

    /**
     * Extracts requested data from downloaded data block
     * @param target Programming target
     * @param block Downloaded data block
     * @return Requested data
     */
    static std::vector<uint8_t> parseDownloadResponse(ProgrammingTarget target, const std::vector<uint8_t> &block);

 private:
    /**
     * Checks data length of a packet
     * @param length Data length
     * @throws std::invalid_argument for invalid data length
     */
    static void checkDataLength(std::size_t length);
};

}  // namespace iqrf::connector::spi
