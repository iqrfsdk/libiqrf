/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: TrInfoTest.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-06
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "iqrf/connector/TrInfo.h"

namespace iqrf::connector {

namespace {

/**
 * Returns Module Info including IBK (Example 2 from IQRF SPI Technical guide)
 * @return Module Info block
 */
std::vector<uint8_t> moduleInfo() {
    return {
        0x74, 0xE5, 0x10, 0x81, 0x43, 0x24, 0xC2, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x40, 0xFE, 0x11, 0x19, 0x48, 0x1D, 0x8D, 0xE1, 0x3F, 0x04, 0x98, 0x04, 0x1E, 0x81, 0x24, 0x09,
    };
}

/**
 * Creates TR module information with the specified TR type
 * @param trType TR type
 * @param mid Module ID
 * @return TR module information
 */
TrInfo withTrType(const uint8_t trType, const uint32_t mid = 0x0110E574) {
    TrInfo info;
    info.mid = mid;
    info.osVersion = 0x46;
    info.trType = trType;
    return info;
}

}  // namespace

TEST(TrInfoTest, parse) {
    const TrInfo info = TrInfo::parse(moduleInfo());
    EXPECT_EQ(info.mid, 0x8110E574);
    EXPECT_EQ(info.osVersion, 0x43);
    EXPECT_EQ(info.osVersionMajor(), 4);
    EXPECT_EQ(info.osVersionMinor(), 3);
    EXPECT_EQ(info.trType, 0x24);
    EXPECT_EQ(info.trSeries(), static_cast<uint8_t>(TrSeriesD::TR_72D));
    EXPECT_EQ(info.mcuType(), McuType::PIC16LF1938);
    EXPECT_FALSE(info.isFccCertified());
    EXPECT_TRUE(info.isDctr());
    EXPECT_EQ(info.osBuild, 0x08C2);
    ASSERT_TRUE(info.ibk.has_value());
    const std::array<uint8_t, TrInfo::IBK_LENGTH> ibk = info.ibk.value_or(std::array<uint8_t, TrInfo::IBK_LENGTH>{});
    EXPECT_EQ(ibk.front(), 0x40);
    EXPECT_EQ(ibk.back(), 0x09);
}

TEST(TrInfoTest, parseBasic) {
    std::vector<uint8_t> data = moduleInfo();
    data.resize(TrInfo::BASIC_LENGTH);
    const TrInfo info = TrInfo::parse(data);
    EXPECT_EQ(info.mid, 0x8110E574);
    EXPECT_FALSE(info.ibk.has_value());
    EXPECT_EQ(info.ibkString(), "");
}

TEST(TrInfoTest, parseIbkNotSupported) {
    // IQRF OS v4.02D does not provide IBK, the rest of the block is undefined
    std::vector<uint8_t> data = moduleInfo();
    data[4] = 0x42;
    const TrInfo info = TrInfo::parse(data);
    EXPECT_FALSE(info.supportsIbk());
    EXPECT_FALSE(info.ibk.has_value());
}

TEST(TrInfoTest, parseTooShort) {
    EXPECT_THROW(TrInfo::parse({0x00, 0x01, 0x02}), std::invalid_argument);
}

TEST(TrInfoTest, strings) {
    const TrInfo info = TrInfo::parse(moduleInfo());
    EXPECT_EQ(info.midString(), "8110E574");
    EXPECT_EQ(info.mcuTypeString(), "PIC16LF1938");
    EXPECT_EQ(info.trTypeString(), "(DC)TR-72D");
    EXPECT_EQ(info.osVersionString(), "4.03D");
    EXPECT_EQ(info.osBuildString(), "08C2");
    EXPECT_EQ(info.osString(), "4.03D (08C2)");
    EXPECT_EQ(info.ibkString(), "40FE1119481D8DE13F0498041E812409");
    EXPECT_EQ(
        info.toString(),
        "(DC)TR-72D, MCU PIC16LF1938, MID 8110E574, OS 4.03D (08C2), FCC not certified, "
        "IBK 40FE1119481D8DE13F0498041E812409"
    );
    std::ostringstream ss;
    ss << info;
    EXPECT_EQ(ss.str(), info.toString());
}

TEST(TrInfoTest, trTypeString) {
    EXPECT_EQ(withTrType(0x24).trTypeString(), "TR-72D");
    EXPECT_EQ(withTrType(0xB4).trTypeString(), "TR-76D");
    EXPECT_EQ(withTrType(0xD4).trTypeString(), "TR-75D");
    EXPECT_EQ(withTrType(0x25).trTypeString(), "TR-72G");
    EXPECT_EQ(withTrType(0xBD).trTypeString(), "TR-76G");
    EXPECT_EQ(withTrType(0xB5, 0x8110E574).trTypeString(), "(DC)TR-76G");
    // Unknown TR series
    EXPECT_EQ(withTrType(0x54).trTypeString(), "UNKNOWN");
    EXPECT_EQ(withTrType(0x15).trTypeString(), "UNKNOWN");
    // Unknown MCU type
    EXPECT_EQ(withTrType(0x26).trTypeString(), "UNKNOWN");
    EXPECT_EQ(withTrType(0x26).mcuTypeString(), "UNKNOWN");
    EXPECT_EQ(withTrType(0x26).osVersionString(), "4.06?");
}

TEST(TrInfoTest, referenceGuideExamples) {
    // Examples from IQRF OS Reference guide, function moduleInfo()
    const TrInfo tr72d = TrInfo::parse({0x1C, 0x10, 0x00, 0x01, 0x42, 0x24, 0x91, 0x08});
    EXPECT_EQ(tr72d.midString(), "0100101C");
    EXPECT_EQ(tr72d.osVersionString(), "4.02D");
    EXPECT_EQ(tr72d.trSeriesString(), "TR-72D");
    EXPECT_EQ(tr72d.mcuTypeString(), "PIC16LF1938");
    EXPECT_FALSE(tr72d.isFccCertified());
    EXPECT_EQ(tr72d.osBuildString(), "0891");

    const TrInfo tr76d = TrInfo::parse({0x1C, 0x10, 0x00, 0x81, 0x42, 0xBC, 0xC8, 0x08});
    EXPECT_EQ(tr76d.midString(), "8100101C");
    EXPECT_EQ(tr76d.osVersionString(), "4.02D");
    EXPECT_EQ(tr76d.trSeriesString(), "TR-76D");
    EXPECT_EQ(tr76d.mcuTypeString(), "PIC16LF1938");
    EXPECT_TRUE(tr76d.isFccCertified());
    EXPECT_EQ(tr76d.osBuildString(), "08C8");

    const TrInfo tr72g = TrInfo::parse({0x1C, 0x10, 0x00, 0x81, 0x43, 0x25, 0xC8, 0x08});
    EXPECT_EQ(tr72g.osVersionString(), "4.03G");
    EXPECT_EQ(tr72g.trSeriesString(), "TR-72G");
    EXPECT_EQ(tr72g.mcuTypeString(), "PIC16LF18877");
    EXPECT_FALSE(tr72g.isFccCertified());
}

TEST(TrInfoTest, trSeriesString) {
    // TR-xxD series
    EXPECT_EQ(withTrType(0x04).trSeriesString(), "TR-52D");
    EXPECT_EQ(withTrType(0x14).trSeriesString(), "TR-58D-RJ");
    EXPECT_EQ(withTrType(0x34).trSeriesString(), "TR-53D");
    EXPECT_EQ(withTrType(0x44).trSeriesString(), "TR-78D");
    EXPECT_EQ(withTrType(0x84).trSeriesString(), "TR-54D");
    EXPECT_EQ(withTrType(0x94).trSeriesString(), "TR-55D");
    EXPECT_EQ(withTrType(0xA4).trSeriesString(), "TR-56D");
    EXPECT_EQ(withTrType(0xC4).trSeriesString(), "TR-77D");
    // TR-xxG series
    EXPECT_EQ(withTrType(0x05).trSeriesString(), "TR-82G");
    EXPECT_EQ(withTrType(0x95).trSeriesString(), "TR-85G");
    EXPECT_EQ(withTrType(0xA5).trSeriesString(), "TR-86G");
    EXPECT_EQ(withTrType(0xD5).trSeriesString(), "TR-75G");
    // Unknown TR series
    EXPECT_EQ(withTrType(0x15).trSeriesString(), "UNKNOWN");
    EXPECT_EQ(withTrType(0xF4).trSeriesString(), "UNKNOWN");
}

TEST(TrInfoTest, sharedMcuPins) {
    // Examples from IQRF OS Reference guide, function moduleInfo()
    EXPECT_TRUE(withTrType(0x25).hasSharedMcuPins());
    EXPECT_FALSE(withTrType(0xB5).hasSharedMcuPins());
}

TEST(TrInfoTest, fccCertification) {
    EXPECT_TRUE(withTrType(0xBD).isFccCertified());
    EXPECT_FALSE(withTrType(0xB5).isFccCertified());
}

}  // namespace iqrf::connector
