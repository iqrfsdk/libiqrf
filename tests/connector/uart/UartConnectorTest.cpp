/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: UartConnectorTest.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-06
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "iqrf/connector/uart/HdlcFrame.h"
#include "iqrf/connector/uart/UartConnector.h"
#include "iqrf/gpio/Base.h"
#include "iqrf/gpio/Gpio.h"
#include "iqrf/gpio/GpioMock.h"

using ::testing::ElementsAre;
using ::testing::IsEmpty;

namespace iqrf::connector::uart {

/**
 * Simulated UART port state
 */
struct FakeUartPortState {
    /// Chunks of bytes returned by the following reads
    std::deque<std::vector<uint8_t>> incoming;
    /// Written data
    std::vector<std::vector<uint8_t>> written;
    /// Number of input flushes
    int flushes = 0;
    /// Fail the following writes
    bool failWrites = false;
    /// Number of the following failing reads
    std::atomic_int failingReads = 0;
    /// Guards incoming data accessed by the listening thread
    std::mutex mutex;
};

/**
 * Simulated UART port
 */
class FakeUartPort : public IUartPort {
 public:
    explicit FakeUartPort(std::shared_ptr<FakeUartPortState> state): state(std::move(state)) {}

    std::vector<uint8_t> read(const std::size_t maxSize, const std::chrono::milliseconds timeout) override {
        (void) timeout;
        const std::scoped_lock lock(this->state->mutex);
        if (this->state->failingReads > 0) {
            --this->state->failingReads;
            throw std::runtime_error("Read failure");
        }
        if (this->state->incoming.empty()) {
            return {};
        }
        std::vector<uint8_t> &chunk = this->state->incoming.front();
        const auto count = static_cast<std::ptrdiff_t>(std::min(maxSize, chunk.size()));
        std::vector<uint8_t> result(chunk.begin(), chunk.begin() + count);
        chunk.erase(chunk.begin(), chunk.begin() + count);
        if (chunk.empty()) {
            this->state->incoming.pop_front();
        }
        return result;
    }

    void write(const std::vector<uint8_t> &data, const std::chrono::milliseconds timeout) override {
        (void) timeout;
        if (this->state->failWrites) {
            throw std::runtime_error("Timeout while writing to UART port");
        }
        this->state->written.push_back(data);
    }

    void flushInput() override {
        const std::scoped_lock lock(this->state->mutex);
        this->state->incoming.clear();
        ++this->state->flushes;
    }

 private:
    /// Simulated UART port state
    std::shared_ptr<FakeUartPortState> state;
};

/**
 * GPIO value change
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

class UartConnectorTest : public ::testing::Test {
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
     * Creates UART connector configuration with GPIO mocks
     * @param trModuleReset Reset TR module during initialization
     * @param disablePowerOnShutdown Disable TR power on shutdown
     * @param busEnableGpio Bus enable GPIO driver, the bus GPIO mock is used if null
     * @return UART connector configuration
     */
    UartConfig createConfig(
        const bool trModuleReset,
        const bool disablePowerOnShutdown = true,
        const std::shared_ptr<iqrf::gpio::Base> &busEnableGpio = nullptr
    ) {
        return {
            "/dev/null",
            115200,
            iqrf::gpio::Gpio(this->power),
            iqrf::gpio::Gpio(busEnableGpio != nullptr ? busEnableGpio : this->bus),
            iqrf::gpio::Gpio(this->pgm),
            std::nullopt,
            std::nullopt,
            std::nullopt,
            trModuleReset,
            disablePowerOnShutdown
        };
    }

    /**
     * Creates UART connector with simulated UART port
     * @param config UART connector configuration
     * @return UART connector
     */
    std::unique_ptr<UartConnector> createConnector(const UartConfig &config) {
        return std::make_unique<UartConnector>(config, std::make_unique<FakeUartPort>(this->port));
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

    /**
     * Encodes data into HDLC frame
     * @param data Frame data
     * @return Encoded frame
     */
    static std::vector<uint8_t> encode(const std::vector<uint8_t> &data) {
        return HdlcFrame(data).encode();
    }

    /// Simulated UART port
    std::shared_ptr<FakeUartPortState> port = std::make_shared<FakeUartPortState>();
    /// Recorded GPIO value changes
    std::vector<GpioEvent> events;
    /// TR power enable GPIO
    std::shared_ptr<iqrf::gpio::GpioMock> power = createMock("power");
    /// Bus enable GPIO
    std::shared_ptr<iqrf::gpio::GpioMock> bus = createMock("bus");
    /// PGM switch GPIO
    std::shared_ptr<iqrf::gpio::GpioMock> pgm = createMock("pgm");
    /// DPA request
    const std::vector<uint8_t> request = {0x00, 0x00, 0x06, 0x03, 0xff, 0xff};
    /// DPA confirmation
    const std::vector<uint8_t> confirmation = {0x00, 0x00, 0x06, 0x83, 0x00, 0x00, 0xff, 0x02, 0x04, 0x01};
    /// DPA response (contains bytes to escape)
    const std::vector<uint8_t> response = {0x00, 0x00, 0x06, 0x83, 0x7e, 0x7d, 0x00, 0x40};
};

TEST_F(UartConnectorTest, initWithoutReset) {
    const auto connector = this->createConnector(this->createConfig(false));
    // Initial values are set by initOutput (no change events), then UART bus is enabled
    EXPECT_THAT(this->sequence(), ElementsAre("bus=1"));
    EXPECT_TRUE(this->power->getValue());
    EXPECT_FALSE(this->pgm->getValue());
    EXPECT_EQ(this->port->flushes, 1);
}

TEST_F(UartConnectorTest, initWithReset) {
    const auto connector = this->createConnector(this->createConfig(true));
    EXPECT_THAT(this->sequence(), ElementsAre("power=0", "power=1", "bus=1"));
    EXPECT_GE(elapsed(this->events[0], this->events[1]).count(), 300);
}

TEST_F(UartConnectorTest, initDiscardsReceivedData) {
    this->port->incoming.push_back(this->encode(this->response));
    const auto connector = this->createConnector(this->createConfig(false));
    EXPECT_THAT(connector->receive(), IsEmpty());
}

TEST_F(UartConnectorTest, destructor) {
    this->createConnector(this->createConfig(false)).reset();
    EXPECT_THAT(this->sequence(), ElementsAre("bus=1", "power=0", "bus=0"));
    EXPECT_FALSE(this->pgm->getValue());
}

TEST_F(UartConnectorTest, destructorKeepsPower) {
    this->createConnector(this->createConfig(false, false)).reset();
    EXPECT_THAT(this->sequence(), ElementsAre("bus=1", "bus=0"));
    EXPECT_TRUE(this->power->getValue());
}

TEST_F(UartConnectorTest, resetTr) {
    const auto connector = this->createConnector(this->createConfig(false));
    this->events.clear();
    this->port->incoming.push_back(this->encode(this->response));

    connector->resetTr();

    // UART is disconnected from TR module during the power cycle
    EXPECT_THAT(this->sequence(), ElementsAre("bus=0", "power=0", "power=1", "bus=1"));
    EXPECT_GE(elapsed(this->events[1], this->events[2]).count(), 300);
    // Data received before the reset are discarded
    EXPECT_THAT(connector->receive(), IsEmpty());
}

TEST_F(UartConnectorTest, resetTrWithoutPowerGpio) {
    UartConfig config = this->createConfig(false);
    config.powerEnableGpio = std::nullopt;
    const auto connector = this->createConnector(config);
    this->events.clear();

    EXPECT_NO_THROW(connector->resetTr());
    EXPECT_THAT(this->sequence(), IsEmpty());
}

TEST_F(UartConnectorTest, gpioFailureRestoresGpio) {
    // Bus switch fails during initialization after TR power was enabled
    const UartConfig config = this->createConfig(false, true, std::make_shared<FailingGpio>());
    EXPECT_THROW(this->createConnector(config), std::runtime_error);
    // Power is disabled again, failures of the bus switch are only logged (no std::terminate)
    EXPECT_THAT(this->sequence(), ElementsAre("power=0"));
    EXPECT_FALSE(this->pgm->getValue());
}

TEST_F(UartConnectorTest, portFailureKeepsGpioUntouched) {
    UartConfig config = this->createConfig(true);
    config.device = "/nonexistent/ttyS0";
    // UART port is opened before GPIOs are initialized
    EXPECT_THROW(UartConnector{config}, std::runtime_error);
    EXPECT_EQ(this->power->getState(), iqrf::gpio::GpioMockState::Uninitialized);
    EXPECT_EQ(this->bus->getState(), iqrf::gpio::GpioMockState::Uninitialized);
    EXPECT_EQ(this->pgm->getState(), iqrf::gpio::GpioMockState::Uninitialized);
    EXPECT_TRUE(this->events.empty());
}

TEST_F(UartConnectorTest, send) {
    const auto connector = this->createConnector(this->createConfig(false));
    connector->send(this->request);
    EXPECT_THAT(this->port->written, ElementsAre(this->encode(this->request)));
    EXPECT_THROW(connector->send({}), std::runtime_error);
}

TEST_F(UartConnectorTest, sendFailure) {
    const auto connector = this->createConnector(this->createConfig(false));
    this->port->failWrites = true;
    EXPECT_THROW(connector->send(this->request), std::runtime_error);
}

TEST_F(UartConnectorTest, receiveNoData) {
    const auto connector = this->createConnector(this->createConfig(false));
    EXPECT_THAT(connector->receive(), IsEmpty());
}

TEST_F(UartConnectorTest, receiveFramesInOneChunk) {
    const auto connector = this->createConnector(this->createConfig(false));
    std::vector<uint8_t> chunk = this->encode(this->confirmation);
    const std::vector<uint8_t> responseFrame = this->encode(this->response);
    chunk.insert(chunk.end(), responseFrame.begin(), responseFrame.end());
    this->port->incoming.push_back(chunk);

    EXPECT_EQ(connector->receive(), this->confirmation);
    EXPECT_EQ(connector->receive(), this->response);
    EXPECT_THAT(connector->receive(), IsEmpty());
}

TEST_F(UartConnectorTest, receiveFrameInMoreChunks) {
    const auto connector = this->createConnector(this->createConfig(false));
    const std::vector<uint8_t> frame = this->encode(this->response);
    const auto middle = frame.begin() + static_cast<std::ptrdiff_t>(frame.size() / 2);
    this->port->incoming.emplace_back(frame.begin(), middle);
    this->port->incoming.emplace_back(middle, frame.end());

    EXPECT_EQ(connector->receive(), this->response);
}

TEST_F(UartConnectorTest, receiveFrameInterruptedByTimeout) {
    const auto connector = this->createConnector(this->createConfig(false));
    const std::vector<uint8_t> frame = this->encode(this->response);
    const auto middle = frame.begin() + static_cast<std::ptrdiff_t>(frame.size() / 2);
    this->port->incoming.emplace_back(frame.begin(), middle);
    // Partially received frame is kept until the next receive
    EXPECT_THAT(connector->receive(), IsEmpty());
    this->port->incoming.emplace_back(middle, frame.end());
    EXPECT_EQ(connector->receive(), this->response);
}

TEST_F(UartConnectorTest, receiveDiscardsInvalidFrame) {
    const auto connector = this->createConnector(this->createConfig(false));
    std::vector<uint8_t> invalidFrame = this->encode(this->confirmation);
    // Corrupt CRC
    invalidFrame[invalidFrame.size() - 2] ^= 0x01;
    this->port->incoming.push_back(invalidFrame);
    this->port->incoming.push_back(this->encode(this->response));

    EXPECT_EQ(connector->receive(), this->response);
}

TEST_F(UartConnectorTest, listenContinuesAfterErrors) {
    const auto connector = this->createConnector(this->createConfig(false));
    std::mutex receivedMutex;
    std::vector<std::vector<uint8_t>> received;
    connector->registerResponseHandler([&receivedMutex, &received](const std::vector<uint8_t> &data) {
        const std::scoped_lock lock(receivedMutex);
        received.push_back(data);
        if (received.size() == 1) {
            throw std::runtime_error("Handler failure");
        }
        return 0;
    }, AccessType::Normal);
    {
        const std::scoped_lock lock(this->port->mutex);
        this->port->failingReads = 1;
        this->port->incoming.push_back(this->encode(this->confirmation));
        this->port->incoming.push_back(this->encode(this->response));
    }

    connector->listen();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
        {
            const std::scoped_lock lock(receivedMutex);
            if (received.size() == 2) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    connector->stopListen();

    // Neither receive error nor handler error stops the listening loop
    const std::scoped_lock lock(receivedMutex);
    EXPECT_THAT(received, ElementsAre(this->confirmation, this->response));
}

}  // namespace iqrf::connector::uart
