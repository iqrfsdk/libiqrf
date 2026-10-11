/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: CdcProtocolTest.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-12
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "iqrf/connector/IConnector.h"
#include "iqrf/connector/cdc/CdcProtocol.h"

using ::testing::IsEmpty;

namespace iqrf::connector::cdc {

namespace {

std::vector<uint8_t> toBytes(const std::string &text) {
    return {text.begin(), text.end()};
}

}  // namespace

TEST(CdcProtocolTest, buildCommandWithoutPayload) {
    EXPECT_EQ(protocol::buildCommand("IT"), toBytes(">IT\r"));
    EXPECT_EQ(protocol::buildCommand("RT"), toBytes(">RT\r"));
    EXPECT_EQ(protocol::buildCommand("PE"), toBytes(">PE\r"));
    EXPECT_EQ(protocol::buildCommand("PT"), toBytes(">PT\r"));
}

TEST(CdcProtocolTest, buildSendCommand) {
    const std::vector<uint8_t> command = protocol::buildSendCommand({0x01, 0x0D});
    EXPECT_EQ(command, (std::vector<uint8_t>{'>', 'D', 'S', 0x02, ':', 0x01, 0x0D, '\r'}));
}

TEST(CdcProtocolTest, buildSendCommandMaxLength) {
    const std::vector<uint8_t> data(64, 0xAA);
    const std::vector<uint8_t> command = protocol::buildSendCommand(data);
    EXPECT_EQ(command.size(), 3U + 2U + 64U + 1U);
    EXPECT_EQ(command[3], 0x40);
}

TEST(CdcProtocolTest, buildSendCommandTooLong) {
    const std::vector<uint8_t> data(65, 0xAA);
    EXPECT_THROW(protocol::buildSendCommand(data), std::invalid_argument);
}

TEST(CdcProtocolTest, uploadTargets) {
    EXPECT_EQ(protocol::uploadTarget(ProgrammingTarget::Config), 0x80);
    EXPECT_EQ(protocol::uploadTarget(ProgrammingTarget::Rfpgm), 0x81);
    EXPECT_EQ(protocol::uploadTarget(ProgrammingTarget::RfBand), 0x82);
    EXPECT_EQ(protocol::uploadTarget(ProgrammingTarget::AccessPassword), 0x83);
    EXPECT_EQ(protocol::uploadTarget(ProgrammingTarget::UserKey), 0x84);
    EXPECT_EQ(protocol::uploadTarget(ProgrammingTarget::Flash), 0x85);
    EXPECT_EQ(protocol::uploadTarget(ProgrammingTarget::InternalEeprom), 0x86);
    EXPECT_EQ(protocol::uploadTarget(ProgrammingTarget::ExternalEeprom), 0x87);
    EXPECT_EQ(protocol::uploadTarget(ProgrammingTarget::Special), 0x88);
}

TEST(CdcProtocolTest, downloadTargets) {
    EXPECT_EQ(protocol::downloadTarget(ProgrammingTarget::Config), 0x00);
    EXPECT_EQ(protocol::downloadTarget(ProgrammingTarget::Rfpgm), 0x01);
    EXPECT_EQ(protocol::downloadTarget(ProgrammingTarget::RfBand), 0x02);
    EXPECT_EQ(protocol::downloadTarget(ProgrammingTarget::Flash), 0x05);
    EXPECT_EQ(protocol::downloadTarget(ProgrammingTarget::InternalEeprom), 0x06);
    EXPECT_EQ(protocol::downloadTarget(ProgrammingTarget::ExternalEeprom), 0x07);
    EXPECT_THROW(protocol::downloadTarget(ProgrammingTarget::AccessPassword), std::invalid_argument);
    EXPECT_THROW(protocol::downloadTarget(ProgrammingTarget::UserKey), std::invalid_argument);
    EXPECT_THROW(protocol::downloadTarget(ProgrammingTarget::Special), std::invalid_argument);
}

TEST(CdcProtocolTest, buildUploadCommand) {
    const std::vector<uint8_t> command = protocol::buildUploadCommand(ProgrammingTarget::Config, {0x01, 0x02});
    EXPECT_EQ(command, (std::vector<uint8_t>{'>', 'P', 'M', 0x80, 0x01, 0x02, '\r'}));
}

TEST(CdcProtocolTest, buildDownloadCommandWithoutAddress) {
    const std::vector<uint8_t> command = protocol::buildDownloadCommand(ProgrammingTarget::Rfpgm, std::nullopt);
    EXPECT_EQ(command, (std::vector<uint8_t>{'>', 'P', 'M', 0x01, '\r'}));
}

TEST(CdcProtocolTest, buildDownloadCommandWithAddressLittleEndian) {
    const std::vector<uint8_t> command = protocol::buildDownloadCommand(ProgrammingTarget::Flash, 0x3A00);
    EXPECT_EQ(command, (std::vector<uint8_t>{'>', 'P', 'M', 0x05, 0x00, 0x3A, '\r'}));
}

TEST(CdcProtocolTest, extractFrameWithStatus) {
    std::vector<uint8_t> buffer = toBytes("<DS:OK\r<PE:ERR1\r");

    const auto first = protocol::extractFrame(buffer);
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->header, "DS");
    EXPECT_EQ(protocol::statusOf(*first), "OK");

    const auto second = protocol::extractFrame(buffer);
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(second->header, "PE");
    EXPECT_EQ(protocol::statusOf(*second), "ERR1");

    EXPECT_THAT(buffer, IsEmpty());
}

TEST(CdcProtocolTest, extractFrameWithoutBody) {
    std::vector<uint8_t> buffer = toBytes("<ERR\r");
    const auto frame = protocol::extractFrame(buffer);
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->header, "ERR");
    EXPECT_TRUE(frame->body.empty());
}

TEST(CdcProtocolTest, extractFrameDiscardsLeadingBytes) {
    std::vector<uint8_t> buffer = toBytes("garbage<RT:OK\r");
    const auto frame = protocol::extractFrame(buffer);
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->header, "RT");
    EXPECT_EQ(protocol::statusOf(*frame), "OK");
}

TEST(CdcProtocolTest, extractFrameIncomplete) {
    std::vector<uint8_t> buffer = toBytes("<DS:O");
    EXPECT_FALSE(protocol::extractFrame(buffer).has_value());
    EXPECT_EQ(buffer, toBytes("<DS:O"));

    buffer = toBytes("no start");
    EXPECT_FALSE(protocol::extractFrame(buffer).has_value());
    EXPECT_THAT(buffer, IsEmpty());
}

TEST(CdcProtocolTest, extractAsyncDataWithTerminatorInData) {
    // "<DR" + length 3 + ':' + data {0x0D, 'A', 'B'} + '\r'
    std::vector<uint8_t> buffer = {'<', 'D', 'R', 0x03, ':', 0x0D, 'A', 'B', '\r', '<', 'O'};

    const auto frame = protocol::extractFrame(buffer);
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->header, "DR");
    EXPECT_EQ(frame->body, (std::vector<uint8_t>{0x0D, 'A', 'B'}));
    EXPECT_EQ(buffer, (std::vector<uint8_t>{'<', 'O'}));
}

TEST(CdcProtocolTest, extractAsyncDataIncomplete) {
    std::vector<uint8_t> buffer = {'<', 'D', 'R', 0x03, ':', 0x0D, 'A'};
    EXPECT_FALSE(protocol::extractFrame(buffer).has_value());
    EXPECT_EQ(buffer.size(), 7U);
}

TEST(CdcProtocolTest, extractAsyncDataInvalid) {
    std::vector<uint8_t> buffer = {'<', 'D', 'R', 0x01, 'X', 'A', 'B', '\r'};
    EXPECT_THROW(protocol::extractFrame(buffer), CdcFrameError);
}

TEST(CdcProtocolTest, extractTrInfoStandard) {
    // "<IT:" + 16 B body (containing the terminator) + '\r'
    std::vector<uint8_t> buffer = {'<', 'I', 'T', ':'};
    for (uint8_t i = 0; i < 16; ++i) {
        buffer.push_back(i == 10 ? 0x0D : i);
    }
    buffer.push_back('\r');

    const auto frame = protocol::extractFrame(buffer);
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->header, "IT");
    EXPECT_EQ(frame->body.size(), 16U);
    EXPECT_EQ(frame->body[10], 0x0D);
    EXPECT_TRUE(buffer.empty());
}

TEST(CdcProtocolTest, extractTrInfoExtended) {
    std::vector<uint8_t> buffer = {'<', 'I', 'T', ':'};
    for (uint8_t i = 0; i < 32; ++i) {
        buffer.push_back(i);
    }
    buffer.push_back('\r');

    const auto frame = protocol::extractFrame(buffer);
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->body.size(), 32U);
    EXPECT_TRUE(buffer.empty());
}

TEST(CdcProtocolTest, extractAsyncErrorFrame) {
    std::vector<uint8_t> buffer = {'<', 'D', 'R', ':', 'E', 'R', 'R', '\r', '<', 'O', 'K', '\r'};

    const auto frame = protocol::extractFrame(buffer);
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->header, "ERR");
    EXPECT_EQ(buffer, toBytes("<OK\r"));
}

TEST(CdcProtocolTest, extractAsyncErrorFrameIncomplete) {
    std::vector<uint8_t> buffer = toBytes("<DR:ERR");
    EXPECT_FALSE(protocol::extractFrame(buffer).has_value());
    EXPECT_EQ(buffer, toBytes("<DR:ERR"));
}

TEST(CdcProtocolTest, extractDownloadFrameData) {
    // "<PM" + 32 B data starting with the separator and containing the terminator + '\r' + "<OK\r"
    std::vector<uint8_t> buffer = {'<', 'P', 'M', ':', 0x0D};
    buffer.resize(3 + 32, 0x11);
    buffer.push_back('\r');
    const std::vector<uint8_t> rest = toBytes("<OK\r");
    buffer.insert(buffer.end(), rest.begin(), rest.end());

    const auto frame = protocol::extractDownloadFrame(buffer, 32);
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->header, "PM");
    ASSERT_EQ(frame->body.size(), 32U);
    EXPECT_EQ(frame->body[0], ':');
    EXPECT_EQ(frame->body[1], 0x0D);
    EXPECT_EQ(buffer, rest);
}

TEST(CdcProtocolTest, extractDownloadFrameSingleByteWithSeparatorValue) {
    // "<PM" + one byte 0x3A (':') + '\r'
    std::vector<uint8_t> buffer = {'<', 'P', 'M', ':', '\r'};
    const auto frame = protocol::extractDownloadFrame(buffer, 1);
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->body, (std::vector<uint8_t>{':'}));
}

TEST(CdcProtocolTest, extractDownloadFrameStatus) {
    std::vector<uint8_t> buffer = toBytes("<PM:ERR3\r");
    const auto frame = protocol::extractDownloadFrame(buffer, 32);
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->header, "PM");
    EXPECT_EQ(protocol::statusOf(*frame), "ERR3");
    EXPECT_TRUE(buffer.empty());
}

TEST(CdcProtocolTest, extractDownloadFrameBusyFromPcHeader) {
    std::vector<uint8_t> buffer = toBytes("<PC:BUSY\r");
    const auto frame = protocol::extractDownloadFrame(buffer, 1);
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(protocol::statusOf(*frame), "BUSY");
}

TEST(CdcProtocolTest, extractDownloadFrameIncomplete) {
    std::vector<uint8_t> buffer = {'<', 'P', 'M', 0x01, 0x02};
    EXPECT_FALSE(protocol::extractDownloadFrame(buffer, 32).has_value());
    EXPECT_EQ(buffer.size(), 5U);
}

TEST(CdcProtocolTest, extractDownloadFrameDelegatesOtherFrames) {
    std::vector<uint8_t> buffer = toBytes("<DS:OK\r");
    const auto frame = protocol::extractDownloadFrame(buffer, 32);
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->header, "DS");
}

TEST(CdcProtocolTest, downloadLengths) {
    EXPECT_EQ(protocol::downloadLength(ProgrammingTarget::Config), 32U);
    EXPECT_EQ(protocol::downloadLength(ProgrammingTarget::Rfpgm), 1U);
    EXPECT_EQ(protocol::downloadLength(ProgrammingTarget::RfBand), 1U);
    EXPECT_EQ(protocol::downloadLength(ProgrammingTarget::Flash), 32U);
    EXPECT_EQ(protocol::downloadLength(ProgrammingTarget::InternalEeprom), 32U);
    EXPECT_EQ(protocol::downloadLength(ProgrammingTarget::ExternalEeprom), 32U);
    EXPECT_THROW(protocol::downloadLength(ProgrammingTarget::UserKey), std::invalid_argument);
}

TEST(CdcProtocolTest, extractSpiStatus) {
    // The status value may be the terminator itself
    std::vector<uint8_t> buffer = {'<', 'S', ':', 0x0D, '\r', '<', 'O'};

    const auto frame = protocol::extractFrame(buffer);
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->header, "S");
    EXPECT_EQ(frame->body, (std::vector<uint8_t>{0x0D}));
    EXPECT_EQ(buffer, (std::vector<uint8_t>{'<', 'O'}));
}

TEST(CdcProtocolTest, extractSpiStatusIncomplete) {
    std::vector<uint8_t> buffer = {'<', 'S', ':', 0x81};
    EXPECT_FALSE(protocol::extractFrame(buffer).has_value());
    EXPECT_EQ(buffer.size(), 4U);
}

TEST(CdcProtocolTest, extractSpiStatusInvalid) {
    std::vector<uint8_t> buffer = {'<', 'S', ':', 0x81, 0x02};
    EXPECT_THROW(protocol::extractFrame(buffer), CdcFrameError);
}

TEST(CdcProtocolTest, parseUsbDeviceInfo) {
    const UsbDeviceInfo info = protocol::parseUsbDeviceInfo(toBytes("GW-USB-03#02.01#03010000"));
    EXPECT_EQ(info.type, "GW-USB-03");
    EXPECT_EQ(info.version, "02.01");
    EXPECT_EQ(info.id, "03010000");
}

TEST(CdcProtocolTest, parseUsbDeviceInfoInvalid) {
    EXPECT_THROW(protocol::parseUsbDeviceInfo(toBytes("GW-USB-03#02.01")), std::runtime_error);
    EXPECT_THROW(protocol::parseUsbDeviceInfo(toBytes("GW-USB-03#02.01#1#2")), std::runtime_error);
    EXPECT_THROW(protocol::parseUsbDeviceInfo(toBytes("")), std::runtime_error);
}

TEST(CdcProtocolTest, extractTrInfoShortWithoutModule) {
    // "<IT:" + 8 B module info (zeros, no TR module) + '\r'
    std::vector<uint8_t> buffer = {'<', 'I', 'T', ':', 0, 0, 0, 0, 0, 0, 0, 0, '\r'};

    const auto frame = protocol::extractFrame(buffer);
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->header, "IT");
    EXPECT_EQ(frame->body.size(), 8U);
    EXPECT_TRUE(buffer.empty());
}

TEST(CdcProtocolTest, extractTrInfoIncomplete) {
    std::vector<uint8_t> buffer = {'<', 'I', 'T', ':'};
    buffer.resize(20, 0x01);
    EXPECT_FALSE(protocol::extractFrame(buffer).has_value());
    EXPECT_EQ(buffer.size(), 20U);
}

TEST(CdcProtocolTest, extractTrInfoInvalid) {
    std::vector<uint8_t> buffer = {'<', 'I', 'T', ':'};
    buffer.resize(37, 0x01);
    EXPECT_THROW(protocol::extractFrame(buffer), CdcFrameError);
}

}  // namespace iqrf::connector::cdc
