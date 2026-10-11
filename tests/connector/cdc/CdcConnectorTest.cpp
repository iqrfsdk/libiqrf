/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: CdcConnectorTest.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-12
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "iqrf/connector/IConnector.h"
#include "iqrf/connector/TrInfo.h"
#include "iqrf/connector/cdc/CdcConnector.h"
#include "iqrf/connector/cdc/CdcProtocol.h"
#include "iqrf/connector/cdc/ICdcPort.h"

using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::IsEmpty;

namespace iqrf::connector::cdc {

namespace {

std::vector<uint8_t> toBytes(const std::string &text) {
    return {text.begin(), text.end()};
}

/**
 * Simulated CDC port state
 */
struct FakeCdcPortState {
    /// Chunks of bytes returned by the following reads
    std::deque<std::vector<uint8_t>> incoming;
    /// Written commands
    std::vector<std::vector<uint8_t>> written;
    /// Produces the bytes received in reply to a written command
    std::function<std::vector<uint8_t>(const std::vector<uint8_t> &)> responder;
    /// Guards the state accessed by the listening thread
    std::mutex mutex;
};

/**
 * Simulated CDC port
 */
class FakeCdcPort : public ICdcPort {
 public:
    explicit FakeCdcPort(std::shared_ptr<FakeCdcPortState> state): state(std::move(state)) {}

    std::vector<uint8_t> read(const std::size_t maxSize, const std::chrono::milliseconds timeout) override {
        {
            const std::scoped_lock lock(this->state->mutex);
            if (!this->state->incoming.empty()) {
                std::vector<uint8_t> &chunk = this->state->incoming.front();
                const auto count = static_cast<std::ptrdiff_t>(std::min(maxSize, chunk.size()));
                std::vector<uint8_t> result(chunk.begin(), chunk.begin() + count);
                chunk.erase(chunk.begin(), chunk.begin() + count);
                if (chunk.empty()) {
                    this->state->incoming.pop_front();
                }
                return result;
            }
        }
        std::this_thread::sleep_for(std::min(timeout, std::chrono::milliseconds(1)));
        return {};
    }

    void write(const std::vector<uint8_t> &data, const std::chrono::milliseconds timeout) override {
        (void) timeout;
        const std::scoped_lock lock(this->state->mutex);
        this->state->written.push_back(data);
        if (this->state->responder) {
            std::vector<uint8_t> reply = this->state->responder(data);
            if (!reply.empty()) {
                this->state->incoming.push_back(std::move(reply));
            }
        }
    }

    void flushInput() override {
        const std::scoped_lock lock(this->state->mutex);
        this->state->incoming.clear();
    }

 private:
    std::shared_ptr<FakeCdcPortState> state;
};

/**
 * Creates the connector with the fake port and returns the state of the port
 */
struct Fixture {
    std::shared_ptr<FakeCdcPortState> state = std::make_shared<FakeCdcPortState>();
    std::unique_ptr<CdcConnector> connector = std::make_unique<CdcConnector>(
        "/dev/fake",
        std::make_unique<FakeCdcPort>(state)
    );

    /// Replies the same bytes to every written command
    void replyWith(const std::vector<uint8_t> &reply) {
        this->state->responder = [reply](const std::vector<uint8_t> &) { return reply; };
    }

    /// Replies the bytes to the command with the specified header
    void replyTo(const std::string &header, const std::vector<uint8_t> &reply) {
        this->state->responder = [header, reply](const std::vector<uint8_t> &command) {
            const std::string prefix = ">" + header;
            if (std::string(command.begin(), command.begin() + static_cast<std::ptrdiff_t>(prefix.size())) == prefix) {
                return reply;
            }
            return std::vector<uint8_t>{};
        };
    }
};

}  // namespace

TEST(CdcConnectorTest, sendSucceedsOnOkStatus) {
    Fixture fixture;
    fixture.replyWith(toBytes("<DS:OK\r"));

    EXPECT_NO_THROW(fixture.connector->send({0x00, 0x01, 0x02}));

    ASSERT_EQ(fixture.state->written.size(), 1U);
    EXPECT_EQ(fixture.state->written[0], (std::vector<uint8_t>{'>', 'D', 'S', 0x03, ':', 0x00, 0x01, 0x02, '\r'}));
}

TEST(CdcConnectorTest, sendFailsOnErrStatus) {
    Fixture fixture;
    fixture.replyWith(toBytes("<DS:ERR\r"));
    EXPECT_THROW(fixture.connector->send({0x00}), std::runtime_error);
}

TEST(CdcConnectorTest, sendFailsOnBusyStatus) {
    Fixture fixture;
    fixture.replyWith(toBytes("<DS:BUSY\r"));
    try {
        fixture.connector->send({0x00});
        FAIL() << "Exception expected";
    } catch (const std::runtime_error &e) {
        EXPECT_THAT(e.what(), HasSubstr("busy"));
    }
}

TEST(CdcConnectorTest, sendRejectsEmptyData) {
    Fixture fixture;
    EXPECT_THROW(fixture.connector->send({}), std::runtime_error);
    EXPECT_TRUE(fixture.state->written.empty());
}

TEST(CdcConnectorTest, sendQueuesAsyncDataReceivedBeforeResponse) {
    Fixture fixture;
    // Asynchronous data "{0x01, 0x02}" arrive before the DS response
    fixture.state->responder = [](const std::vector<uint8_t> &) {
        return std::vector<uint8_t>{'<', 'D', 'R', 0x02, ':', 0x01, 0x02, '\r', '<', 'D', 'S', ':', 'O', 'K', '\r'};
    };

    EXPECT_NO_THROW(fixture.connector->send({0x10}));
    EXPECT_THAT(fixture.connector->receive(), ElementsAre(0x01, 0x02));
}

TEST(CdcConnectorTest, receiveReturnsAsyncData) {
    Fixture fixture;
    fixture.state->incoming.push_back({'<', 'D', 'R', 0x03, ':', 0x0D, 'A', 'B', '\r'});

    EXPECT_THAT(fixture.connector->receive(), ElementsAre(0x0D, 'A', 'B'));
}

TEST(CdcConnectorTest, receiveReturnsFramesSplitIntoChunks) {
    Fixture fixture;
    fixture.state->incoming.push_back({'<', 'D', 'R'});
    fixture.state->incoming.push_back({0x02, ':', 0x01});
    fixture.state->incoming.push_back({0x02, '\r'});

    EXPECT_THAT(fixture.connector->receive(), ElementsAre(0x01, 0x02));
}

TEST(CdcConnectorTest, receiveReturnsEmptyWithoutData) {
    Fixture fixture;
    EXPECT_THAT(fixture.connector->receive(), IsEmpty());
}

TEST(CdcConnectorTest, readTrInfoStandard) {
    Fixture fixture;
    // Module ID 0x01020304, OS version 0x31, TR type 0x92, OS build 0x0102 (little endian), then reserved bytes
    std::vector<uint8_t> body = {0x04, 0x03, 0x02, 0x01, 0x31, 0x92, 0x02, 0x01};
    body.resize(TrInfo::BASIC_LENGTH, 0x00);
    std::vector<uint8_t> reply = {'<', 'I', 'T', ':'};
    reply.insert(reply.end(), body.begin(), body.end());
    reply.push_back('\r');
    fixture.replyTo("IT", reply);

    const TrInfo info = fixture.connector->readTrInfo();

    EXPECT_EQ(info.mid, 0x01020304U);
    EXPECT_EQ(info.osVersion, 0x31);
    EXPECT_EQ(info.trType, 0x92);
    EXPECT_EQ(info.osBuild, 0x0102);
    EXPECT_FALSE(info.ibk.has_value());
    ASSERT_EQ(fixture.state->written.size(), 1U);
    EXPECT_EQ(fixture.state->written[0], toBytes(">IT\r"));
}

TEST(CdcConnectorTest, readTrInfoExtendedWithTerminatorInIbk) {
    Fixture fixture;
    std::vector<uint8_t> body(TrInfo::EXTENDED_LENGTH, 0x00);
    body[4] = 0x43;  // IQRF OS 4.03, IBK is available
    body[TrInfo::BASIC_LENGTH] = 0x0D;  // The first byte of IBK is the terminator
    std::vector<uint8_t> reply = {'<', 'I', 'T', ':'};
    reply.insert(reply.end(), body.begin(), body.end());
    reply.push_back('\r');
    fixture.replyTo("IT", reply);

    const TrInfo info = fixture.connector->readTrInfo();

    ASSERT_TRUE(info.ibk.has_value());
    EXPECT_EQ((*info.ibk)[0], 0x0D);
}

TEST(CdcConnectorTest, resetTrSucceedsOnOkStatus) {
    Fixture fixture;
    fixture.replyTo("RT", toBytes("<RT:OK\r"));

    EXPECT_NO_THROW(fixture.connector->resetTr());
    EXPECT_EQ(fixture.state->written.at(0), toBytes(">RT\r"));
}

TEST(CdcConnectorTest, resetTrFailsOnErrorStatus) {
    Fixture fixture;
    fixture.replyTo("RT", toBytes("<RT:ERR\r"));
    EXPECT_THROW(fixture.connector->resetTr(), std::runtime_error);
}

TEST(CdcConnectorTest, enterProgrammingModeSucceedsOnOkStatus) {
    Fixture fixture;
    fixture.replyTo("PE", toBytes("<PE:OK\r"));

    EXPECT_NO_THROW(fixture.connector->enterProgrammingMode());
    EXPECT_EQ(fixture.state->written.at(0), toBytes(">PE\r"));
}

TEST(CdcConnectorTest, enterProgrammingModeFailsOnErr1) {
    Fixture fixture;
    fixture.replyTo("PE", toBytes("<PE:ERR1\r"));
    EXPECT_THROW(fixture.connector->enterProgrammingMode(), std::runtime_error);
}

TEST(CdcConnectorTest, exitProgrammingModeSucceedsOnOkStatus) {
    Fixture fixture;
    fixture.replyTo("PT", toBytes("<PT:OK\r"));

    EXPECT_NO_THROW(fixture.connector->exitProgrammingMode());
    EXPECT_EQ(fixture.state->written.at(0), toBytes(">PT\r"));
}

TEST(CdcConnectorTest, exitProgrammingModeFailsOnErr1) {
    Fixture fixture;
    fixture.replyTo("PT", toBytes("<PT:ERR1\r"));
    EXPECT_THROW(fixture.connector->exitProgrammingMode(), std::runtime_error);
}

TEST(CdcConnectorTest, uploadSucceedsOnOkStatus) {
    Fixture fixture;
    fixture.replyTo("PM", toBytes("<PM:OK\r"));

    EXPECT_NO_THROW(fixture.connector->upload(ProgrammingTarget::Rfpgm, {0x01}));
    EXPECT_EQ(fixture.state->written.at(0), (std::vector<uint8_t>{'>', 'P', 'M', 0x81, 0x01, '\r'}));
}

TEST(CdcConnectorTest, uploadFailsOnErrorStatus) {
    Fixture fixture;
    fixture.replyTo("PM", toBytes("<PM:ERR4\r"));
    EXPECT_THROW(fixture.connector->upload(ProgrammingTarget::Flash, {0x01}), std::runtime_error);
}

TEST(CdcConnectorTest, uploadSpecialUsesPluginTarget) {
    Fixture fixture;
    fixture.replyTo("PM", toBytes("<PM:OK\r"));

    EXPECT_NO_THROW(fixture.connector->upload(ProgrammingTarget::Special, {0x01}));
    EXPECT_EQ(fixture.state->written.at(0), (std::vector<uint8_t>{'>', 'P', 'M', 0x88, 0x01, '\r'}));
}

TEST(CdcConnectorTest, downloadRejectsUnsupportedTargetWithoutCommand) {
    Fixture fixture;
    EXPECT_THROW(fixture.connector->download(ProgrammingTarget::UserKey), std::invalid_argument);
    EXPECT_TRUE(fixture.state->written.empty());
}

TEST(CdcConnectorTest, downloadReturnsData) {
    Fixture fixture;
    // Downloaded data are returned without the separator
    fixture.replyTo("PM", std::vector<uint8_t>{'<', 'P', 'M', 0x03, '\r'});

    EXPECT_THAT(fixture.connector->download(ProgrammingTarget::RfBand), ElementsAre(0x03));
    EXPECT_EQ(fixture.state->written.at(0), (std::vector<uint8_t>{'>', 'P', 'M', 0x02, '\r'}));
}

TEST(CdcConnectorTest, downloadWithAddressSendsAddress) {
    Fixture fixture;
    std::vector<uint8_t> reply = {'<', 'P', 'M'};
    for (uint8_t i = 0; i < 32; ++i) {
        reply.push_back(i == 5 ? 0x0D : i);  // The terminator inside the data
    }
    reply.push_back('\r');
    fixture.replyTo("PM", reply);

    const std::vector<uint8_t> data = fixture.connector->download(ProgrammingTarget::Flash, 0x3A00);
    ASSERT_EQ(data.size(), 32U);
    EXPECT_EQ(data[5], 0x0D);
    EXPECT_EQ(fixture.state->written.at(0), (std::vector<uint8_t>{'>', 'P', 'M', 0x05, 0x00, 0x3A, '\r'}));
}

TEST(CdcConnectorTest, downloadFailsOnErrorStatus) {
    Fixture fixture;
    fixture.replyTo("PM", toBytes("<PM:ERR3\r"));
    EXPECT_THROW(fixture.connector->download(ProgrammingTarget::Flash, 0x0000), std::runtime_error);
}

TEST(CdcConnectorTest, downloadFailsOnBusyFromPcHeader) {
    Fixture fixture;
    fixture.replyTo("PM", toBytes("<PC:BUSY\r"));
    EXPECT_THROW(fixture.connector->download(ProgrammingTarget::Config), std::runtime_error);
}

TEST(CdcConnectorTest, uploadFailsOnBusyFromPcHeader) {
    Fixture fixture;
    fixture.replyTo("PM", toBytes("<PC:BUSY\r"));
    EXPECT_THROW(fixture.connector->upload(ProgrammingTarget::Rfpgm, {0x01}), std::runtime_error);
}

TEST(CdcConnectorTest, resetUsbSucceedsOnOkStatus) {
    Fixture fixture;
    fixture.replyTo("R", toBytes("<R:OK\r"));

    EXPECT_NO_THROW(fixture.connector->resetUsb());
    EXPECT_EQ(fixture.state->written.at(0), toBytes(">R\r"));
}

TEST(CdcConnectorTest, resetUsbFailsOnErrorStatus) {
    Fixture fixture;
    fixture.replyTo("R", toBytes("<R:ERR\r"));
    EXPECT_THROW(fixture.connector->resetUsb(), std::runtime_error);
}

TEST(CdcConnectorTest, indicateConnectivity) {
    Fixture fixture;
    fixture.replyTo("B", toBytes("<B:OK\r"));

    EXPECT_NO_THROW(fixture.connector->indicateConnectivity());
    EXPECT_EQ(fixture.state->written.at(0), toBytes(">B\r"));
}

TEST(CdcConnectorTest, readUsbInfo) {
    Fixture fixture;
    fixture.replyTo("I", toBytes("<I:GW-USB-03#02.01#03010000\r"));

    const UsbDeviceInfo info = fixture.connector->readUsbInfo();
    EXPECT_EQ(info.type, "GW-USB-03");
    EXPECT_EQ(info.version, "02.01");
    EXPECT_EQ(info.id, "03010000");
    EXPECT_EQ(fixture.state->written.at(0), toBytes(">I\r"));
}

TEST(CdcConnectorTest, readSpiStatus) {
    Fixture fixture;
    fixture.replyTo("S", std::vector<uint8_t>{'<', 'S', ':', 0x81, '\r'});

    EXPECT_EQ(fixture.connector->readSpiStatus(), 0x81);
    EXPECT_EQ(fixture.state->written.at(0), toBytes(">S\r"));
}

TEST(CdcConnectorTest, readSpiStatusTerminatorValue) {
    Fixture fixture;
    fixture.replyTo("S", std::vector<uint8_t>{'<', 'S', ':', 0x0D, '\r'});
    EXPECT_EQ(fixture.connector->readSpiStatus(), 0x0D);
}

TEST(CdcConnectorTest, switchCommandsUseTheirHeaders) {
    {
        Fixture fixture;
        fixture.replyTo("U", toBytes("<U:OK\r"));
        EXPECT_NO_THROW(fixture.connector->switchToCustomClass());
        EXPECT_EQ(fixture.state->written.at(0), toBytes(">U\r"));
    }
    {
        Fixture fixture;
        fixture.replyTo("UU", toBytes("<UU:OK\r"));
        EXPECT_NO_THROW(fixture.connector->switchToCdcUart());
        EXPECT_EQ(fixture.state->written.at(0), toBytes(">UU\r"));
    }
    {
        Fixture fixture;
        fixture.replyTo("US", toBytes("<US:OK\r"));
        EXPECT_NO_THROW(fixture.connector->switchToCdcSpi());
        EXPECT_EQ(fixture.state->written.at(0), toBytes(">US\r"));
    }
}

TEST(CdcConnectorTest, switchFailsOnErrorStatus) {
    Fixture fixture;
    fixture.replyTo("UU", toBytes("<UU:ERR\r"));
    EXPECT_THROW(fixture.connector->switchToCdcUart(), std::runtime_error);
}

TEST(CdcConnectorTest, errorFrameFailsTransaction) {
    Fixture fixture;
    fixture.replyWith(toBytes("<ERR\r"));
    EXPECT_THROW(fixture.connector->readTrInfo(), std::runtime_error);
}

}  // namespace iqrf::connector::cdc
