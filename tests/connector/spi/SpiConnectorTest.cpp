/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: SpiConnectorTest.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-05
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <iterator>
#include <optional>
#include <string>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "iqrf/connector/spi/SpiConnector.h"
#include "iqrf/gpio/Base.h"
#include "iqrf/gpio/Gpio.h"
#include "iqrf/gpio/GpioMock.h"

using ::testing::ElementsAre;
using ::testing::IsEmpty;

namespace iqrf::connector::spi {

/**
 * Simulated TR module state
 */
struct FakeTrModule {
    /// SPI status
    uint8_t status = 0x80;
    /// Data ready to be read by master
    std::vector<uint8_t> outgoing;
    /// Data written by master
    std::vector<std::vector<uint8_t>> written;
    /// Upload packets received from master
    std::vector<std::vector<uint8_t>> uploads;
    /// Confirm uploads
    bool confirmUploads = true;
    /// Complete programming-mode uploads
    bool completeUploads = true;
    /// Confirm programming-mode download requests
    bool confirmDownloads = true;
    /// Results of CRCM verification returned for the following written data packets (default 0x3F)
    std::vector<uint8_t> writeResults;
    /// Number of the following read data packets with corrupted CRCS
    int corruptedReads = 0;
    /// TR module information including IBK
    std::vector<uint8_t> trInfo = {
        0x78, 0x56, 0x34, 0x12, 0x46, 0x24, 0xD8, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x40, 0xFE, 0x11, 0x19, 0x48, 0x1D, 0x8D, 0xE1, 0x3F, 0x04, 0x98, 0x04, 0x1E, 0x81, 0x24, 0x09,
    };
    /// Data block returned by download request
    std::vector<uint8_t> downloadBlock;
    /// Number of the following status reads during TR module startup (SPI not active)
    int startupChecks = 0;
    /// Number of the following TR module information reads with corrupted CRCS
    int corruptedTrInfoReads = 0;

    /**
     * Builds response to a read packet
     * @param data Data to read
     * @param length Length of the response
     * @return Response
     */
    static std::vector<uint8_t> readResponse(const std::vector<uint8_t> &data, std::size_t length) {
        std::vector<uint8_t> response(length, 0);
        std::copy(data.begin(), data.end(), response.begin() + 2);
        response[data.size() + 2] = SpiProtocol::crcs(SpiProtocol::ptype(data.size(), false), data);
        // Trailing SPI_CHECK - CRCM O.K.
        response.back() = 0x3F;
        return response;
    }

    std::vector<uint8_t> transfer(const std::vector<uint8_t> &tx) {
        if (tx.size() == 1 && tx[0] == SpiProtocol::CMD_CHECK) {
            if (this->startupChecks > 0) {
                --this->startupChecks;
                return {0x00};
            }
            return {this->status};
        }
        const uint8_t cmd = tx[0];
        const bool bufferChanged = (tx[1] & 0x80) != 0;
        const std::size_t length = tx[1] & 0x7F;
        switch (cmd) {
            case SpiProtocol::CMD_DATA:
                if (bufferChanged) {
                    std::vector<uint8_t> response(tx.size(), 0x80);
                    response.back() = 0x3F;
                    if (!this->writeResults.empty()) {
                        response.back() = this->writeResults.front();
                        this->writeResults.erase(this->writeResults.begin());
                    }
                    if (response.back() == 0x3F) {
                        const auto first = tx.begin() + 2;
                        this->written.emplace_back(first, first + static_cast<std::ptrdiff_t>(length));
                    }
                    return response;
                } else {
                    std::vector<uint8_t> response = readResponse(this->outgoing, tx.size());
                    if (this->corruptedReads > 0) {
                        // Data remains in bufferCOM
                        --this->corruptedReads;
                        response[length + 2] ^= 0xFF;
                        return response;
                    }
                    this->outgoing.clear();
                    this->status = this->downloadBlock.empty() ? 0x80 : 0x81;
                    this->downloadBlock.clear();
                    return response;
                }
            case SpiProtocol::CMD_TR_MODULE_INFO: {
                std::vector<uint8_t> data = this->trInfo;
                data.resize(length, 0);
                std::vector<uint8_t> response = readResponse(data, tx.size());
                if (this->startupChecks > 0 || this->status == 0x00) {
                    // SPI is not active, nothing is returned
                    return std::vector<uint8_t>(tx.size(), 0);
                }
                if (this->corruptedTrInfoReads > 0) {
                    --this->corruptedTrInfoReads;
                    response[length + 2] ^= 0xFF;
                }
                return response;
            }
            case SpiProtocol::CMD_READ_FLASH:
                this->outgoing = this->downloadBlock;
                this->status = static_cast<uint8_t>(0x40 + this->outgoing.size());
                {
                    std::vector<uint8_t> response(tx.size(), 0);
                    response.back() = this->confirmDownloads ? 0x3F : 0x3E;
                    return response;
                }
            default: {
                this->uploads.push_back(tx);
                std::vector<uint8_t> response(tx.size(), 0);
                response.back() = this->confirmUploads ? 0x3F : 0x3E;
                if (!this->completeUploads) {
                    this->status = 0x80;
                }
                return response;
            }
        }
    }
};

/**
 * Fake SPI device forwarding transfers to the simulated TR module
 */
class FakeSpiDevice : public ISpiDevice {
 public:
    explicit FakeSpiDevice(std::shared_ptr<FakeTrModule> tr): tr(std::move(tr)) {}

    std::vector<uint8_t> transfer(const std::vector<uint8_t> &tx, const SpiTransferTiming &timing) override {
        (void) timing;
        return this->tr->transfer(tx);
    }

 private:
    std::shared_ptr<FakeTrModule> tr;
};

class SpiConnectorTest : public ::testing::Test {
 protected:
    void SetUp() override {
        this->tr = std::make_shared<FakeTrModule>();
        SpiConfig config("/dev/null");
        config.trModuleReset = false;
        this->connector = std::make_unique<SpiConnector>(config, std::make_unique<FakeSpiDevice>(this->tr));
    }

    std::shared_ptr<FakeTrModule> tr;
    std::unique_ptr<SpiConnector> connector;
};

TEST_F(SpiConnectorTest, getSpiStatus) {
    this->tr->status = 0x81;
    EXPECT_TRUE(this->connector->getSpiStatus().is(SpiStatusValue::ReadyProgramming));
    this->tr->status = 0x10;
    EXPECT_THROW(this->connector->getSpiStatus(), std::runtime_error);
}

TEST_F(SpiConnectorTest, getState) {
    EXPECT_EQ(this->connector->getState(), State::Ready);
    this->tr->status = 0x3F;
    EXPECT_EQ(this->connector->getState(), State::NotReady);
    this->tr->status = 0x10;
    EXPECT_EQ(this->connector->getState(), State::NotReady);
}

TEST_F(SpiConnectorTest, receiveNoData) {
    EXPECT_TRUE(this->connector->receive().empty());
}

TEST_F(SpiConnectorTest, receiveData) {
    this->tr->outgoing = {0x01, 0x02, 0x03};
    this->tr->status = 0x43;
    EXPECT_THAT(this->connector->receive(), ElementsAre(0x01, 0x02, 0x03));
    EXPECT_TRUE(this->connector->receive().empty());
}

TEST_F(SpiConnectorTest, send) {
    this->connector->send({0x00, 0x00, 0x06, 0x01, 0xFF, 0xFF});
    ASSERT_EQ(this->tr->written.size(), 1);
    EXPECT_THAT(this->tr->written[0], ElementsAre(0x00, 0x00, 0x06, 0x01, 0xFF, 0xFF));
}

TEST_F(SpiConnectorTest, sendWithDataReady) {
    this->tr->outgoing = {0xAA, 0xBB};
    this->tr->status = 0x42;
    this->connector->send({0x01});
    ASSERT_EQ(this->tr->written.size(), 1);
    // Data read before send must not be lost
    EXPECT_THAT(this->connector->receive(), ElementsAre(0xAA, 0xBB));
    EXPECT_TRUE(this->connector->receive().empty());
}

TEST_F(SpiConnectorTest, sendNotReady) {
    this->tr->status = 0x3F;
    EXPECT_THROW(this->connector->send({0x01}), std::runtime_error);
    EXPECT_TRUE(this->tr->written.empty());
}

TEST_F(SpiConnectorTest, sendInvalidData) {
    EXPECT_THROW(this->connector->send({}), std::invalid_argument);
}

TEST_F(SpiConnectorTest, invalidSpeed) {
    SpiConfig config("/dev/null");
    config.speed = SpiConfig::MAX_SPEED + 1;
    config.trModuleReset = false;
    EXPECT_THROW(
        SpiConnector(config, std::make_unique<FakeSpiDevice>(this->tr)),
        std::invalid_argument
    );
}

TEST_F(SpiConnectorTest, readTrInfo) {
    const TrInfo info = this->connector->readTrInfo();
    EXPECT_EQ(info.mid, 0x12345678);
    EXPECT_EQ(info.osVersion, 0x46);
    EXPECT_EQ(info.trType, 0x24);
    EXPECT_EQ(info.osBuild, 0x08D8);
    EXPECT_EQ(info.ibkString(), "40FE1119481D8DE13F0498041E812409");
}

TEST_F(SpiConnectorTest, readTrInfoDuringStartup) {
    // SPI of TR module is not active for a while after power up
    this->tr->startupChecks = 20;
    const TrInfo info = this->connector->readTrInfo();
    EXPECT_EQ(info.mid, 0x12345678);
    EXPECT_EQ(this->tr->startupChecks, 0);
}

TEST_F(SpiConnectorTest, readTrInfoCorruptedRead) {
    this->tr->corruptedTrInfoReads = 2;
    const TrInfo info = this->connector->readTrInfo();
    EXPECT_EQ(info.mid, 0x12345678);
    EXPECT_EQ(this->tr->corruptedTrInfoReads, 0);
}

TEST_F(SpiConnectorTest, readTrInfoCorruptedReadsExhausted) {
    this->tr->corruptedTrInfoReads = 3;
    EXPECT_THROW(this->connector->readTrInfo(), std::runtime_error);
}

TEST_F(SpiConnectorTest, readTrInfoSpiNotActive) {
    this->tr->status = 0x00;
    EXPECT_THROW(this->connector->readTrInfo(), std::runtime_error);
}

TEST_F(SpiConnectorTest, readTrInfoWithoutIbk) {
    // IQRF OS v4.02D does not provide IBK
    this->tr->trInfo[4] = 0x42;
    const TrInfo info = this->connector->readTrInfo();
    EXPECT_EQ(info.osVersionString(), "4.02D");
    EXPECT_FALSE(info.ibk.has_value());
}

TEST_F(SpiConnectorTest, enterProgrammingModeWithoutGpio) {
    EXPECT_THROW(this->connector->enterProgrammingMode(), std::runtime_error);
    this->tr->status = 0x81;
    EXPECT_NO_THROW(this->connector->enterProgrammingMode());
}

TEST_F(SpiConnectorTest, upload) {
    this->tr->status = 0x81;
    std::vector<uint8_t> data(32, 0xFF);
    data[0] = 0xAA;
    data[1] = 0xBB;
    this->connector->upload(ProgrammingTarget::Flash, data, 0x3A20);
    ASSERT_EQ(this->tr->uploads.size(), 1);
    EXPECT_THAT(
        std::vector(this->tr->uploads[0].begin(), this->tr->uploads[0].begin() + 6),
        ElementsAre(0xF6, 0xA2, 0x20, 0x3A, 0xAA, 0xBB)
    );
}

TEST_F(SpiConnectorTest, uploadNotConfirmed) {
    this->tr->status = 0x81;
    this->tr->confirmUploads = false;
    EXPECT_THROW(this->connector->upload(ProgrammingTarget::RfBand, {0x01}), std::runtime_error);
}

TEST_F(SpiConnectorTest, uploadCompletionTimeout) {
    this->tr->status = 0x81;
    this->tr->completeUploads = false;
    EXPECT_THROW(this->connector->upload(ProgrammingTarget::RfBand, {0x01}), std::runtime_error);
}

TEST_F(SpiConnectorTest, downloadFlash) {
    this->tr->status = 0x81;
    std::vector<uint8_t> block(32);
    for (std::size_t i = 0; i < block.size(); ++i) {
        block[i] = static_cast<uint8_t>(i);
    }
    this->tr->downloadBlock = block;
    EXPECT_EQ(this->connector->download(ProgrammingTarget::Flash, 0x3A20), block);
}

TEST_F(SpiConnectorTest, downloadNotConfirmed) {
    this->tr->status = 0x81;
    this->tr->confirmDownloads = false;
    EXPECT_THROW(this->connector->download(ProgrammingTarget::Flash, 0x3A20), std::runtime_error);
}

TEST_F(SpiConnectorTest, receiveRepeatedAfterCrcsMismatch) {
    this->tr->outgoing = {0x01, 0x02, 0x03};
    this->tr->status = 0x43;
    this->tr->corruptedReads = 2;
    EXPECT_THAT(this->connector->receive(), ElementsAre(0x01, 0x02, 0x03));
    EXPECT_EQ(this->tr->corruptedReads, 0);
}

TEST_F(SpiConnectorTest, receiveCrcsMismatch) {
    this->tr->outgoing = {0x01, 0x02, 0x03};
    this->tr->status = 0x43;
    this->tr->corruptedReads = 3;
    EXPECT_TRUE(this->connector->receive().empty());
}

TEST_F(SpiConnectorTest, sendRepeatedAfterCrcmError) {
    this->tr->writeResults = {0x3E, 0x3E};
    this->connector->send({0x01});
    ASSERT_EQ(this->tr->written.size(), 1);
    EXPECT_THAT(this->tr->written[0], ElementsAre(0x01));
    EXPECT_TRUE(this->tr->writeResults.empty());
}

TEST_F(SpiConnectorTest, sendCrcmError) {
    this->tr->writeResults = {0x3E, 0x3E, 0x3E};
    EXPECT_THROW(this->connector->send({0x01}), std::runtime_error);
    EXPECT_TRUE(this->tr->written.empty());
}

TEST_F(SpiConnectorTest, sendUnknownResultNotRepeated) {
    this->tr->writeResults = {0x80};
    EXPECT_THROW(this->connector->send({0x01}), std::runtime_error);
    EXPECT_TRUE(this->tr->writeResults.empty());
}

TEST_F(SpiConnectorTest, downloadWithoutAddress) {
    EXPECT_THROW(this->connector->download(ProgrammingTarget::Flash), std::invalid_argument);
}

/**
 * GPIO value change recorded by GpioRecorder
 */
struct GpioEvent {
    /// GPIO name
    std::string name;
    /// New GPIO value
    bool value;
    /// Time of the change
    std::chrono::steady_clock::time_point time;
};

/**
 * GPIO driver which fails on every operation
 */
class FailingGpio : public iqrf::gpio::Base {
 public:
    void initInput() override { throw std::runtime_error("GPIO failure"); }
    void initOutput(bool initialValue) override {
        (void) initialValue;
        throw std::runtime_error("GPIO failure");
    }
    void setDirection(iqrf::gpio::GpioDirection direction) override {
        (void) direction;
        throw std::runtime_error("GPIO failure");
    }
    iqrf::gpio::GpioDirection getDirection() override { throw std::runtime_error("GPIO failure"); }
    void setValue(bool value) override {
        (void) value;
        throw std::runtime_error("GPIO failure");
    }
    bool getValue() override { throw std::runtime_error("GPIO failure"); }
};

class SpiConnectorGpioTest : public ::testing::Test {
 protected:
    /**
     * Creates GPIO mock recording its value changes
     * @param name GPIO name
     * @return GPIO mock
     */
    std::shared_ptr<iqrf::gpio::GpioMock> createMock(const std::string &name) {
        auto mock = std::make_shared<iqrf::gpio::GpioMock>(iqrf::gpio::GpioConfig("gpiochip0", 0, name));
        mock->registerValueCallback([this, name](bool, bool newValue) {
            this->events.push_back({name, newValue, std::chrono::steady_clock::now()});
        });
        return mock;
    }

    /**
     * Creates SPI connector configuration with GPIO mocks
     * @param trModuleReset Reset TR module during initialization
     * @param disablePowerOnShutdown Disable TR power on shutdown
     * @return SPI connector configuration
     */
    SpiConfig createConfig(const bool trModuleReset, const bool disablePowerOnShutdown = true) {
        return {
            "/dev/null",
            iqrf::gpio::Gpio(this->power),
            iqrf::gpio::Gpio(this->bus),
            iqrf::gpio::Gpio(this->pgm),
            std::nullopt,
            std::nullopt,
            std::nullopt,
            trModuleReset,
            disablePowerOnShutdown
        };
    }

    /**
     * Creates SPI connector with fake TR module
     * @param config SPI connector configuration
     * @return SPI connector
     */
    std::unique_ptr<SpiConnector> createConnector(const SpiConfig &config) {
        return std::make_unique<SpiConnector>(config, std::make_unique<FakeSpiDevice>(this->tr));
    }

    /**
     * Returns recorded events of the GPIO
     * @param name GPIO name
     * @return Recorded events
     */
    [[nodiscard]] std::vector<GpioEvent> eventsOf(const std::string &name) const {
        std::vector<GpioEvent> result;
        std::copy_if(this->events.begin(), this->events.end(), std::back_inserter(result),
            [&name](const GpioEvent &event) { return event.name == name; });
        return result;
    }

    /**
     * Returns the sequence of recorded GPIO changes as "name=value" strings
     * @return Sequence of GPIO changes
     */
    [[nodiscard]] std::vector<std::string> sequence() const {
        std::vector<std::string> result;
        result.reserve(this->events.size());
        for (const GpioEvent &event : this->events) {
            result.push_back(event.name + "=" + (event.value ? "1" : "0"));
        }
        return result;
    }

    /**
     * Returns time between two events
     * @param from First event
     * @param to Second event
     * @return Time between the events
     */
    static std::chrono::milliseconds elapsed(const GpioEvent &from, const GpioEvent &to) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(to.time - from.time);
    }

    /// Simulated TR module
    std::shared_ptr<FakeTrModule> tr = std::make_shared<FakeTrModule>();
    /// Recorded GPIO value changes
    std::vector<GpioEvent> events;
    /// TR power enable GPIO
    std::shared_ptr<iqrf::gpio::GpioMock> power = createMock("power");
    /// Bus enable GPIO
    std::shared_ptr<iqrf::gpio::GpioMock> bus = createMock("bus");
    /// PGM switch GPIO
    std::shared_ptr<iqrf::gpio::GpioMock> pgm = createMock("pgm");
};

TEST_F(SpiConnectorGpioTest, initWithoutReset) {
    const auto connector = this->createConnector(this->createConfig(false));
    // Initial values are set by initOutput (no change events), then SPI bus is enabled
    EXPECT_THAT(this->sequence(), ElementsAre("bus=1"));
    EXPECT_TRUE(this->power->getValue());
    EXPECT_TRUE(this->bus->getValue());
    EXPECT_FALSE(this->pgm->getValue());
}

TEST_F(SpiConnectorGpioTest, initWithReset) {
    const auto connector = this->createConnector(this->createConfig(true));
    EXPECT_THAT(this->sequence(), ElementsAre("power=0", "power=1", "bus=1"));
    const auto powerEvents = this->eventsOf("power");
    EXPECT_GE(elapsed(powerEvents[0], powerEvents[1]).count(), 300);
}

TEST_F(SpiConnectorGpioTest, destructor) {
    this->createConnector(this->createConfig(false)).reset();
    EXPECT_THAT(this->sequence(), ElementsAre("bus=1", "power=0", "bus=0"));
    EXPECT_FALSE(this->pgm->getValue());
}

TEST_F(SpiConnectorGpioTest, destructorKeepsPower) {
    this->createConnector(this->createConfig(false, false)).reset();
    EXPECT_THAT(this->sequence(), ElementsAre("bus=1", "bus=0"));
    EXPECT_TRUE(this->power->getValue());
}

TEST_F(SpiConnectorGpioTest, resetTr) {
    const auto connector = this->createConnector(this->createConfig(false));
    this->events.clear();
    connector->resetTr();
    // SPI master is disconnected from TR module during the power cycle
    EXPECT_THAT(this->sequence(), ElementsAre("bus=0", "power=0", "power=1", "bus=1"));
    const auto powerEvents = this->eventsOf("power");
    EXPECT_GE(elapsed(powerEvents[0], powerEvents[1]).count(), 300);
}

TEST_F(SpiConnectorGpioTest, initWithResetWithoutPowerGpio) {
    SpiConfig config = this->createConfig(true);
    config.powerEnableGpio = std::nullopt;
    const auto connector = this->createConnector(config);
    // TR module reset is skipped
    EXPECT_THAT(this->sequence(), ElementsAre("bus=1"));
}

TEST_F(SpiConnectorGpioTest, resetTrWithoutPowerGpio) {
    SpiConfig config = this->createConfig(false);
    config.powerEnableGpio = std::nullopt;
    const auto connector = this->createConnector(config);
    this->events.clear();

    EXPECT_NO_THROW(connector->resetTr());
    EXPECT_THAT(this->sequence(), IsEmpty());
}

TEST_F(SpiConnectorGpioTest, enterProgrammingMode) {
    const auto connector = this->createConnector(this->createConfig(false));
    // TR module enters programming mode if PGM pin is active during power up
    this->power->registerValueCallback([this](bool, bool newValue) {
        this->events.push_back({"power", newValue, std::chrono::steady_clock::now()});
        if (newValue && this->pgm->getValue()) {
            this->tr->status = 0x81;
        }
    });
    this->events.clear();

    connector->enterProgrammingMode();

    EXPECT_THAT(
        this->sequence(),
        ElementsAre("bus=0", "pgm=1", "power=0", "power=1", "pgm=0", "bus=1")
    );
    const auto powerEvents = this->eventsOf("power");
    EXPECT_GE(elapsed(powerEvents[0], powerEvents[1]).count(), 300);
    // PGM pin is held active for 500 ms after power up
    EXPECT_GE(elapsed(powerEvents[1], this->eventsOf("pgm")[1]).count(), 500);
    EXPECT_TRUE(connector->getSpiStatus().is(SpiStatusValue::ReadyProgramming));
}

TEST_F(SpiConnectorGpioTest, enterProgrammingModeTimeout) {
    const auto connector = this->createConnector(this->createConfig(false));
    // TR module does not enter programming mode
    EXPECT_THROW(connector->enterProgrammingMode(), std::runtime_error);
    EXPECT_FALSE(this->pgm->getValue());
    EXPECT_TRUE(this->bus->getValue());
}

TEST_F(SpiConnectorGpioTest, exitProgrammingMode) {
    const auto connector = this->createConnector(this->createConfig(false));
    this->tr->status = 0x81;
    this->power->registerValueCallback([this](bool, bool newValue) {
        this->events.push_back({"power", newValue, std::chrono::steady_clock::now()});
        if (newValue) {
            this->tr->status = 0x80;
        }
    });
    this->events.clear();

    connector->exitProgrammingMode();

    EXPECT_THAT(this->sequence(), ElementsAre("bus=0", "power=0", "power=1", "bus=1"));
    EXPECT_TRUE(connector->getSpiStatus().is(SpiStatusValue::ReadyCommunication));
}

TEST_F(SpiConnectorGpioTest, gpioFailureRestoresGpio) {
    // Bus switch fails during initialization after TR power was enabled
    const SpiConfig config(
        "/dev/null",
        iqrf::gpio::Gpio(this->power),
        iqrf::gpio::Gpio(std::make_shared<FailingGpio>()),
        iqrf::gpio::Gpio(this->pgm),
        std::nullopt,
        std::nullopt,
        std::nullopt,
        false,
        true
    );
    EXPECT_THROW(this->createConnector(config), std::runtime_error);
    // Power is disabled again, failures of the bus switch are only logged (no std::terminate)
    EXPECT_THAT(this->sequence(), ElementsAre("power=0"));
    EXPECT_FALSE(this->pgm->getValue());
}

TEST_F(SpiConnectorGpioTest, deviceFailureKeepsGpioUntouched) {
    SpiConfig config = this->createConfig(true);
    config.device = "/nonexistent/spidev0.0";
    // SPI device is opened before GPIOs are initialized
    EXPECT_THROW(SpiConnector{config}, std::exception);
    EXPECT_EQ(this->power->getState(), iqrf::gpio::GpioMockState::Uninitialized);
    EXPECT_EQ(this->bus->getState(), iqrf::gpio::GpioMockState::Uninitialized);
    EXPECT_EQ(this->pgm->getState(), iqrf::gpio::GpioMockState::Uninitialized);
    EXPECT_TRUE(this->events.empty());
}

}  // namespace iqrf::connector::spi
