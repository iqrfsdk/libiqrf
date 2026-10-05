/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: TrInfo.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-06
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace iqrf::connector {

/**
 * MCU type of TR module (TR type bits 0 - 2)
 */
enum class McuType : uint8_t {
    Unknown = 0,
    PIC16LF819 = 1,
    PIC16LF88 = 2,
    PIC16F886 = 3,
    PIC16LF1938 = 4,
    PIC16LF18877 = 5,
};

/**
 * TR series of TR modules with PIC16LF1938 MCU (TR-xxD, TR type bits 4 - 7)
 *
 * See IQRF OS Reference guide, function moduleInfo().
 */
enum class TrSeriesD : uint8_t {
    TR_52D = 0,
    TR_58D_RJ = 1,
    TR_72D = 2,
    TR_53D = 3,
    TR_78D = 4,
    TR_54D = 8,
    TR_55D = 9,
    TR_56D = 10,
    TR_76D = 11,
    TR_77D = 12,
    TR_75D = 13,
};

/**
 * TR series of TR modules with PIC16LF18877 MCU (TR-xxG, TR type bits 4 - 7)
 *
 * See IQRF OS Reference guide, function moduleInfo().
 */
enum class TrSeriesG : uint8_t {
    TR_82G = 0,
    TR_72G = 2,
    TR_85G = 9,
    TR_86G = 10,
    TR_76G = 11,
    TR_75G = 13,
};

/**
 * Transceiver information as returned by IQRF OS moduleInfo() (Module Info)
 *
 * Module Info block format (LSB first):
 * - bytes 0 - 3: Module ID (MID)
 * - byte 4: IQRF OS version (major in upper nibble, minor in lower nibble)
 * - byte 5: TR type (bits 4 - 7: TR series, bit 3: FCC certified, bits 0 - 2: MCU type),
 *   the most significant bit of TR series distinguishes TR modules with shared and not shared MCU pins
 * - bytes 6 - 7: IQRF OS build
 * - bytes 8 - 15: undefined
 * - bytes 16 - 31: Individual Bonding Key (IBK), available from IQRF OS v4.03
 */
struct TrInfo {
    /// Length of the basic Module Info block
    static constexpr std::size_t BASIC_LENGTH = 16;
    /// Length of the Module Info block including IBK
    static constexpr std::size_t EXTENDED_LENGTH = 32;
    /// Length of the Individual Bonding Key
    static constexpr std::size_t IBK_LENGTH = 16;

    /// Module ID
    uint32_t mid = 0;
    /// IQRF OS version
    uint8_t osVersion = 0;
    /// TR type
    uint8_t trType = 0;
    /// IQRF OS build
    uint16_t osBuild = 0;
    /// Individual Bonding Key, available only if read from the TR module with IQRF OS v4.03 or higher
    std::optional<std::array<uint8_t, IBK_LENGTH>> ibk;

    /**
     * Parses Module Info block
     * @param data Module Info block (at least 8 B, IBK is parsed from 32 B block for IQRF OS v4.03 or higher)
     * @return TR module information
     * @throws std::invalid_argument for too short data block
     */
    static TrInfo parse(const std::vector<uint8_t> &data) {
        if (data.size() < 8) {
            throw std::invalid_argument("TR module information is too short");
        }
        TrInfo info;
        info.mid = static_cast<uint32_t>(data[0]) |
            (static_cast<uint32_t>(data[1]) << 8) |
            (static_cast<uint32_t>(data[2]) << 16) |
            (static_cast<uint32_t>(data[3]) << 24);
        info.osVersion = data[4];
        info.trType = data[5];
        info.osBuild = static_cast<uint16_t>(data[6] | (data[7] << 8));
        if (data.size() >= EXTENDED_LENGTH && info.supportsIbk()) {
            std::array<uint8_t, IBK_LENGTH> key{};
            const auto first = data.begin() + static_cast<std::ptrdiff_t>(BASIC_LENGTH);
            std::copy(first, first + static_cast<std::ptrdiff_t>(IBK_LENGTH), key.begin());
            info.ibk = key;
        }
        return info;
    }

    /**
     * Returns IQRF OS major version
     * @return IQRF OS major version
     */
    [[nodiscard]] uint8_t osVersionMajor() const { return osVersion >> 4; }

    /**
     * Returns IQRF OS minor version
     * @return IQRF OS minor version
     */
    [[nodiscard]] uint8_t osVersionMinor() const { return osVersion & 0x0F; }

    /**
     * Checks whether IQRF OS is able to provide IBK (IQRF OS v4.03 or higher)
     * @return true if IBK is supported
     */
    [[nodiscard]] bool supportsIbk() const {
        return osVersionMajor() > 4 || (osVersionMajor() == 4 && osVersionMinor() >= 3);
    }

    /**
     * Checks whether the TR module is DCTR (MID bit 31)
     *
     * The (DC) prefix of TR module type was used by older IQRF OS versions, current IQRF OS Reference guide
     * does not distinguish DCTR modules.
     *
     * @return true if the TR module is DCTR
     */
    [[nodiscard]] bool isDctr() const { return (mid & 0x80000000U) != 0; }

    /**
     * Returns MCU type
     * @return MCU type (raw value of the TR type bits 0 - 2 cast to McuType)
     */
    [[nodiscard]] McuType mcuType() const {
        // Unknown values are valid as McuType has fixed underlying type
        return static_cast<McuType>(trType & 0x07);  // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
    }

    /**
     * Returns TR series, use TrSeriesD or TrSeriesG depending on the MCU type for interpretation
     * @return TR series
     */
    [[nodiscard]] uint8_t trSeries() const { return trType >> 4; }

    /**
     * Checks whether the TR module is FCC certified
     * @return true if the TR module is FCC certified
     */
    [[nodiscard]] bool isFccCertified() const { return (trType & 0x08) != 0; }

    /**
     * Checks whether the TR module shares MCU pins on the Cx SIM pads (the most significant bit of TR series is 0)
     *
     * E.g. TR-72G has shared MCU pins (RC5 and RC7 to TR pin C8 etc.), TR-76G does not.
     *
     * @return true if MCU pins are shared
     */
    [[nodiscard]] bool hasSharedMcuPins() const { return (trType & 0x80) == 0; }

    /**
     * Returns Module ID as string
     * @return Module ID (e.g. 8110E574)
     */
    [[nodiscard]] std::string midString() const {
        std::ostringstream ss;
        ss << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << mid;
        return ss.str();
    }

    /**
     * Returns MCU type as string
     * @return MCU type (e.g. PIC16LF1938), UNKNOWN for unknown MCU type
     */
    [[nodiscard]] std::string mcuTypeString() const {
        switch (mcuType()) {
            case McuType::PIC16LF819:
                return "PIC16LF819";
            case McuType::PIC16LF88:
                return "PIC16LF88";
            case McuType::PIC16F886:
                return "PIC16F886";
            case McuType::PIC16LF1938:
                return "PIC16LF1938";
            case McuType::PIC16LF18877:
                return "PIC16LF18877";
            default:
                return "UNKNOWN";
        }
    }

    /**
     * Returns TR series as string
     * @return TR series (e.g. TR-72D, TR-76G), UNKNOWN for unknown TR series or MCU type
     */
    [[nodiscard]] std::string trSeriesString() const {
        const std::string series = trSeriesName();
        return series.empty() ? "UNKNOWN" : series;
    }

    /**
     * Returns TR module type as string
     * @return TR module type (e.g. TR-72D, (DC)TR-76G for DCTR), UNKNOWN for unknown TR series or MCU type
     */
    [[nodiscard]] std::string trTypeString() const {
        const std::string series = trSeriesName();
        if (series.empty()) {
            return "UNKNOWN";
        }
        return (isDctr() ? "(DC)" : "") + series;
    }

    /**
     * Returns IQRF OS version as string
     * @return IQRF OS version (e.g. 4.03D)
     */
    [[nodiscard]] std::string osVersionString() const {
        std::ostringstream ss;
        ss << std::uppercase << std::hex << static_cast<int>(osVersionMajor()) << '.'
            << std::setfill('0') << std::setw(2) << static_cast<int>(osVersionMinor()) << osVersionSuffix();
        return ss.str();
    }

    /**
     * Returns IQRF OS build as string
     * @return IQRF OS build (e.g. 08C2)
     */
    [[nodiscard]] std::string osBuildString() const {
        std::ostringstream ss;
        ss << std::uppercase << std::hex << std::setfill('0') << std::setw(4) << osBuild;
        return ss.str();
    }

    /**
     * Returns IQRF OS version and build as string
     * @return IQRF OS version and build (e.g. 4.03D (08C2))
     */
    [[nodiscard]] std::string osString() const {
        return osVersionString() + " (" + osBuildString() + ")";
    }

    /**
     * Returns Individual Bonding Key as string
     * @return IBK (e.g. 40FE1119481D8DE13F0498041E812409), empty string if IBK is not available
     */
    [[nodiscard]] std::string ibkString() const {
        if (!ibk.has_value()) {
            return "";
        }
        std::ostringstream ss;
        ss << std::uppercase << std::hex << std::setfill('0');
        for (const uint8_t byte : *ibk) {
            ss << std::setw(2) << static_cast<int>(byte);
        }
        return ss.str();
    }

    /**
     * Returns human readable summary of TR module information
     * @return Summary (e.g. TR-72D, MCU PIC16LF1938, MID 8110E574, OS 4.03D (08C2), FCC not certified)
     */
    [[nodiscard]] std::string toString() const {
        std::string result = trTypeString() + ", MCU " + mcuTypeString() + ", MID " + midString() +
            ", OS " + osString() + (isFccCertified() ? ", FCC certified" : ", FCC not certified");
        if (ibk.has_value()) {
            result += ", IBK " + ibkString();
        }
        return result;
    }

    /**
     * Writes human readable summary of TR module information to the stream
     * @param os Output stream
     * @param info TR module information
     * @return Output stream
     */
    friend std::ostream &operator<<(std::ostream &os, const TrInfo &info) {
        return os << info.toString();
    }

 private:
    /**
     * Returns IQRF OS version suffix derived from MCU type
     * @return D for PIC16LF1938, G for PIC16LF18877, ? otherwise
     */
    [[nodiscard]] char osVersionSuffix() const {
        switch (mcuType()) {
            case McuType::PIC16LF1938:
                return 'D';
            case McuType::PIC16LF18877:
                return 'G';
            default:
                return '?';
        }
    }

    /**
     * Returns TR series name
     * @return TR series name (e.g. TR-72D), empty string for unknown TR series or MCU type
     */
    [[nodiscard]] std::string trSeriesName() const {
        if (mcuType() == McuType::PIC16LF1938) {
            // Unknown values are valid as TrSeriesD has fixed underlying type
            switch (static_cast<TrSeriesD>(trSeries())) {  // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
                case TrSeriesD::TR_52D:
                    return "TR-52D";
                case TrSeriesD::TR_58D_RJ:
                    return "TR-58D-RJ";
                case TrSeriesD::TR_72D:
                    return "TR-72D";
                case TrSeriesD::TR_53D:
                    return "TR-53D";
                case TrSeriesD::TR_78D:
                    return "TR-78D";
                case TrSeriesD::TR_54D:
                    return "TR-54D";
                case TrSeriesD::TR_55D:
                    return "TR-55D";
                case TrSeriesD::TR_56D:
                    return "TR-56D";
                case TrSeriesD::TR_76D:
                    return "TR-76D";
                case TrSeriesD::TR_77D:
                    return "TR-77D";
                case TrSeriesD::TR_75D:
                    return "TR-75D";
                default:
                    return "";
            }
        }
        if (mcuType() == McuType::PIC16LF18877) {
            switch (static_cast<TrSeriesG>(trSeries())) {  // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
                case TrSeriesG::TR_82G:
                    return "TR-82G";
                case TrSeriesG::TR_72G:
                    return "TR-72G";
                case TrSeriesG::TR_85G:
                    return "TR-85G";
                case TrSeriesG::TR_86G:
                    return "TR-86G";
                case TrSeriesG::TR_76G:
                    return "TR-76G";
                case TrSeriesG::TR_75G:
                    return "TR-75G";
                default:
                    return "";
            }
        }
        return "";
    }
};

}  // namespace iqrf::connector
