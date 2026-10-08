/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: SpiProtocolTest.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-05
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

#include "iqrf/connector/spi/SpiProtocol.h"

using ::testing::ElementsAre;

namespace iqrf::connector::spi {

namespace {

/**
 * Parses raw SPI status which is expected to be valid
 * @param raw Raw SPI status byte
 * @return Parsed SPI status
 */
SpiStatus parseValid(const uint8_t raw) {
    const std::optional<SpiStatus> status = SpiStatus::parse(raw);
    if (!status.has_value()) {
        throw std::logic_error("Unexpected invalid SPI status");
    }
    return *status;
}

}  // namespace

TEST(SpiStatusTest, parseDataReady) {
    const SpiStatus status64 = parseValid(0x40);
    EXPECT_TRUE(status64.isDataReady());
    EXPECT_EQ(status64.getDataReadyLength(), 64);

    const SpiStatus status5 = parseValid(0x45);
    EXPECT_TRUE(status5.isDataReady());
    EXPECT_EQ(status5.getDataReadyLength(), 5);

    const SpiStatus status63 = parseValid(0x7F);
    EXPECT_EQ(status63.getDataReadyLength(), 63);
}

TEST(SpiStatusTest, parseDataNotReady) {
    const SpiStatus comm = parseValid(0x80);
    EXPECT_FALSE(comm.isDataReady());
    EXPECT_TRUE(comm.is(SpiStatusValue::ReadyCommunication));
    EXPECT_FALSE(comm.is(SpiStatusValue::ReadyProgramming));

    EXPECT_TRUE(parseValid(0x81).is(SpiStatusValue::ReadyProgramming));
    EXPECT_TRUE(parseValid(0x3F).is(SpiStatusValue::BufferProtect));
    EXPECT_TRUE(parseValid(0x00).is(SpiStatusValue::Disabled));
}

TEST(SpiStatusTest, parseInvalid) {
    EXPECT_FALSE(SpiStatus::parse(0x10).has_value());
    EXPECT_FALSE(SpiStatus::parse(0x90).has_value());
}

TEST(SpiProtocolTest, buildWritePacket) {
    // CRCM = 0x5F ^ 0xF0 ^ 0x82 ^ 0x01 ^ 0x02, trailing SPI_CHECK
    EXPECT_THAT(SpiProtocol::buildWritePacket({0x01, 0x02}), ElementsAre(0xF0, 0x82, 0x01, 0x02, 0x2E, 0x00));
    // Example 1 from IQRF SPI Technical guide
    EXPECT_THAT(SpiProtocol::buildWritePacket({0x69}), ElementsAre(0xF0, 0x81, 0x69, 0x47, 0x00));
}

TEST(SpiProtocolTest, buildWritePacketInvalidLength) {
    EXPECT_THROW(SpiProtocol::buildWritePacket({}), std::invalid_argument);
    EXPECT_THROW(SpiProtocol::buildWritePacket(std::vector<uint8_t>(65, 0)), std::invalid_argument);
    EXPECT_NO_THROW(SpiProtocol::buildWritePacket(std::vector<uint8_t>(64, 0)));
}

TEST(SpiProtocolTest, buildReadPacket) {
    // CRCM = 0x5F ^ 0xF0 ^ 0x02, trailing SPI_CHECK
    EXPECT_THAT(SpiProtocol::buildReadPacket(2), ElementsAre(0xF0, 0x02, 0x00, 0x00, 0xAD, 0x00));
    // Example 1 from IQRF SPI Technical guide
    const std::vector<uint8_t> packet = SpiProtocol::buildReadPacket(10);
    ASSERT_EQ(packet.size(), 14);
    EXPECT_EQ(packet[12], 0xA5);
}

TEST(SpiProtocolTest, parseReadResponseGuideExample) {
    // Example 1 from IQRF SPI Technical guide
    const std::vector<uint8_t> response = {
        0x4A, 0x4A, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x54, 0x3F,
    };
    EXPECT_THAT(
        SpiProtocol::parseReadResponse(10, response).value_or(std::vector<uint8_t>()),
        ElementsAre(0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39)
    );
    EXPECT_TRUE(SpiProtocol::isCrcmConfirmed(response));
}

TEST(SpiProtocolTest, parseReadResponse) {
    // CRCS = 0x5F ^ 0x03 ^ 0x01 ^ 0x02 ^ 0x03
    const std::vector<uint8_t> response = {0xAA, 0xBB, 0x01, 0x02, 0x03, 0x5C};
    EXPECT_THAT(
        SpiProtocol::parseReadResponse(3, response).value_or(std::vector<uint8_t>()),
        ElementsAre(0x01, 0x02, 0x03)
    );
}

TEST(SpiProtocolTest, parseReadResponseCrcsMismatch) {
    const std::vector<uint8_t> response = {0xAA, 0xBB, 0x01, 0x02, 0x03, 0x5D};
    EXPECT_FALSE(SpiProtocol::parseReadResponse(3, response).has_value());
    EXPECT_FALSE(SpiProtocol::parseReadResponse(3, {0x00, 0x00}).has_value());
}

TEST(SpiProtocolTest, buildTrInfoPacket) {
    const std::vector<uint8_t> packet = SpiProtocol::buildTrInfoPacket();
    ASSERT_EQ(packet.size(), 20);
    EXPECT_EQ(packet[0], 0xF5);
    EXPECT_EQ(packet[1], 0x10);
    // CRCM = 0x5F ^ 0xF5 ^ 0x10 (Example 2 from IQRF SPI Technical guide)
    EXPECT_EQ(packet[18], 0xBA);
    EXPECT_EQ(packet[19], 0x00);
    // Extended Module Info including IBK, CRCM = 0x5F ^ 0xF5 ^ 0x20 (Example 2 from IQRF SPI Technical guide)
    const std::vector<uint8_t> extended = SpiProtocol::buildTrInfoPacket(32);
    ASSERT_EQ(extended.size(), 36);
    EXPECT_EQ(extended[1], 0x20);
    EXPECT_EQ(extended[34], 0x8A);
    EXPECT_THROW(SpiProtocol::buildTrInfoPacket(8), std::invalid_argument);
}

TEST(SpiProtocolTest, buildUploadPacketRfBand) {
    // CRCM = 0x5F ^ 0xF3 ^ 0x83 ^ 0xC0 ^ 0x01 ^ 0x05
    EXPECT_THAT(
        SpiProtocol::buildUploadPacket(ProgrammingTarget::RfBand, {0x05}),
        ElementsAre(0xF3, 0x83, 0xC0, 0x01, 0x05, 0xEB, 0x00)
    );
}

TEST(SpiProtocolTest, buildUploadPacketAccessPassword) {
    const std::vector<uint8_t> packet = SpiProtocol::buildUploadPacket(
        ProgrammingTarget::AccessPassword,
        std::vector<uint8_t>(16, 0x11)
    );
    ASSERT_EQ(packet.size(), 22);
    EXPECT_EQ(packet[0], 0xF3);
    EXPECT_EQ(packet[1], 0x92);
    EXPECT_EQ(packet[2], 0xD0);
    EXPECT_EQ(packet[3], 0x10);
    EXPECT_EQ(packet[4], 0x11);
    EXPECT_EQ(packet[19], 0x11);
    EXPECT_THROW(
        SpiProtocol::buildUploadPacket(ProgrammingTarget::AccessPassword, std::vector<uint8_t>(15, 0)),
        std::invalid_argument
    );
}

namespace {

/**
 * Creates upload data prefixed by 2 B little-endian address
 * @param address Address
 * @param length Data length
 * @return Upload data
 */
std::vector<uint8_t> uploadData(const uint16_t address, const std::size_t length) {
    std::vector<uint8_t> data = {static_cast<uint8_t>(address & 0xFF), static_cast<uint8_t>(address >> 8)};
    data.resize(length + 2, 0xAA);
    return data;
}

}  // namespace

TEST(SpiProtocolTest, buildUploadPacketInternalEeprom) {
    const std::vector<uint8_t> packet = SpiProtocol::buildUploadPacket(
        ProgrammingTarget::InternalEeprom,
        {0x10, 0x00, 0xAA, 0xBB}
    );
    EXPECT_THAT(
        std::vector(packet.begin(), packet.begin() + 6),
        ElementsAre(0xF3, 0x84, 0x10, 0x02, 0xAA, 0xBB)
    );
    EXPECT_NO_THROW(SpiProtocol::buildUploadPacket(ProgrammingTarget::InternalEeprom, uploadData(0xF0E0, 32)));
    // Too long data block
    EXPECT_THROW(
        SpiProtocol::buildUploadPacket(ProgrammingTarget::InternalEeprom, uploadData(0x00, 33)),
        std::invalid_argument
    );
    // Data block exceeds EEPROM size
    EXPECT_THROW(
        SpiProtocol::buildUploadPacket(ProgrammingTarget::InternalEeprom, uploadData(0xF0F0, 32)),
        std::invalid_argument
    );
}

TEST(SpiProtocolTest, buildUploadPacketExternalEeprom) {
    // Example from IQRF SPI Technical guide: index address = (0x0220 - 0x0200) / 0x20 = 1
    const std::vector<uint8_t> packet = SpiProtocol::buildUploadPacket(
        ProgrammingTarget::ExternalEeprom,
        uploadData(0x0220, 32)
    );
    ASSERT_EQ(packet.size(), 38);
    EXPECT_THAT(
        std::vector(packet.begin(), packet.begin() + 5),
        ElementsAre(0xF6, 0xA2, 0x01, 0x00, 0xAA)
    );
    // Address below virtual offset
    EXPECT_THROW(
        SpiProtocol::buildUploadPacket(ProgrammingTarget::ExternalEeprom, uploadData(0x0100, 32)),
        std::invalid_argument
    );
    // Unaligned address
    EXPECT_THROW(
        SpiProtocol::buildUploadPacket(ProgrammingTarget::ExternalEeprom, uploadData(0x0210, 32)),
        std::invalid_argument
    );
    // Address out of range
    EXPECT_THROW(
        SpiProtocol::buildUploadPacket(ProgrammingTarget::ExternalEeprom, uploadData(0x4200, 32)),
        std::invalid_argument
    );
    // Data block must be exactly 32 B
    EXPECT_THROW(
        SpiProtocol::buildUploadPacket(ProgrammingTarget::ExternalEeprom, uploadData(0x0220, 1)),
        std::invalid_argument
    );
}

TEST(SpiProtocolTest, buildUploadPacketFlash) {
    const std::vector<uint8_t> packet = SpiProtocol::buildUploadPacket(
        ProgrammingTarget::Flash,
        uploadData(0x3A10, 32)
    );
    ASSERT_EQ(packet.size(), 38);
    EXPECT_THAT(
        std::vector(packet.begin(), packet.begin() + 5),
        ElementsAre(0xF6, 0xA2, 0x10, 0x3A, 0xAA)
    );
    // Unaligned address
    EXPECT_THROW(
        SpiProtocol::buildUploadPacket(ProgrammingTarget::Flash, uploadData(0x3A08, 32)),
        std::invalid_argument
    );
    // Data block must be exactly 32 B
    EXPECT_THROW(
        SpiProtocol::buildUploadPacket(ProgrammingTarget::Flash, uploadData(0x3A10, 16)),
        std::invalid_argument
    );
}

TEST(SpiProtocolTest, buildUploadPacketUnsupported) {
    EXPECT_THROW(SpiProtocol::buildUploadPacket(ProgrammingTarget::Config, {0x00}), std::invalid_argument);
}

TEST(SpiProtocolTest, isCrcmConfirmed) {
    EXPECT_TRUE(SpiProtocol::isCrcmConfirmed({0x00, 0x00, 0x3F}));
    EXPECT_FALSE(SpiProtocol::isCrcmConfirmed({0x00, 0x00, 0x3E}));
    EXPECT_FALSE(SpiProtocol::isCrcmConfirmed({}));
}

TEST(SpiProtocolTest, buildDownloadPacket) {
    EXPECT_THAT(
        SpiProtocol::buildDownloadPacket(ProgrammingTarget::Flash, 0x3A20),
        ElementsAre(0xFC, 0x82, 0x20, 0x3A, SpiProtocol::crcm(0xFC, 0x82, {0x20, 0x3A}), 0x00)
    );
    EXPECT_THAT(
        SpiProtocol::buildDownloadPacket(ProgrammingTarget::Rfpgm, 0),
        ElementsAre(0xF2, 0x82, 0xC0, 0x00, SpiProtocol::crcm(0xF2, 0x82, {0xC0, 0x00}), 0x00)
    );
    EXPECT_THAT(
        SpiProtocol::buildDownloadPacket(ProgrammingTarget::ExternalEeprom, 0x0401),
        ElementsAre(0xF6, 0x82, 0x01, 0x04, SpiProtocol::crcm(0xF6, 0x82, {0x01, 0x04}), 0x00)
    );
    // Flash read address must be modulo 32
    EXPECT_THROW(SpiProtocol::buildDownloadPacket(ProgrammingTarget::Flash, 0x3A10), std::invalid_argument);
    // External EEPROM read index address must be in range 0x0400 - 0x05FF
    EXPECT_THROW(SpiProtocol::buildDownloadPacket(ProgrammingTarget::ExternalEeprom, 0x0001), std::invalid_argument);
    EXPECT_THROW(SpiProtocol::buildDownloadPacket(ProgrammingTarget::ExternalEeprom, 0x0600), std::invalid_argument);
    EXPECT_THROW(SpiProtocol::buildDownloadPacket(ProgrammingTarget::UserKey, 0), std::invalid_argument);
    EXPECT_THROW(SpiProtocol::buildDownloadPacket(ProgrammingTarget::Config, 0), std::invalid_argument);
}

TEST(SpiProtocolTest, parseDownloadResponse) {
    std::vector<uint8_t> block(32, 0);
    block[0] = 0x05;
    block[1] = 0x33;
    EXPECT_THAT(SpiProtocol::parseDownloadResponse(ProgrammingTarget::RfBand, block), ElementsAre(0x05));
    EXPECT_THAT(SpiProtocol::parseDownloadResponse(ProgrammingTarget::Rfpgm, block), ElementsAre(0x33));
    EXPECT_EQ(SpiProtocol::parseDownloadResponse(ProgrammingTarget::Flash, block), block);
}

}  // namespace iqrf::connector::spi
