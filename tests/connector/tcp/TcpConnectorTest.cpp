/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: TcpConnectorTest.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-11-16
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include "iqrf/connector/IConnector.h"
#include "iqrf/connector/tcp/ITcpSocket.h"
#include "iqrf/connector/tcp/TcpConfig.h"
#include "iqrf/connector/tcp/TcpConnector.h"

using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::IsEmpty;

namespace iqrf::connector::tcp {

namespace {

using namespace std::chrono_literals;

/**
 * Simulated TCP server state
 */
struct FakeServerState {
    /// Number of the following connection attempts which fail
    int failingConnects = 0;
    /// Number of connection attempts
    int connectAttempts = 0;
    /// Chunks of bytes returned by the following reads
    std::deque<std::vector<uint8_t>> incoming;
    /// Close the connection on the next read
    bool closeOnRead = false;
    /// Fail the next write
    bool failWrite = false;
    /// Written data
    std::vector<std::vector<uint8_t>> written;
    /// Guards the state accessed by the listening thread
    std::mutex mutex;
};

/**
 * Simulated TCP socket
 */
class FakeTcpSocket : public ITcpSocket {
 public:
    explicit FakeTcpSocket(std::shared_ptr<FakeServerState> state): state(std::move(state)) {}

    std::vector<uint8_t> read(const std::size_t maxSize, const std::chrono::milliseconds timeout) override {
        (void) maxSize;
        {
            const std::scoped_lock lock(this->state->mutex);
            if (this->state->closeOnRead) {
                this->state->closeOnRead = false;
                throw std::runtime_error("TCP connection has been closed by the peer");
            }
            if (!this->state->incoming.empty()) {
                std::vector<uint8_t> chunk = std::move(this->state->incoming.front());
                this->state->incoming.pop_front();
                return chunk;
            }
        }
        std::this_thread::sleep_for(std::min(timeout, std::chrono::milliseconds(1)));
        return {};
    }

    void write(const std::vector<uint8_t> &data, const std::chrono::milliseconds timeout) override {
        (void) timeout;
        const std::scoped_lock lock(this->state->mutex);
        if (this->state->failWrite) {
            this->state->failWrite = false;
            throw std::runtime_error("Connection reset by peer");
        }
        this->state->written.push_back(data);
    }

 private:
    std::shared_ptr<FakeServerState> state;
};

/**
 * Creates the connector with the fake socket factory
 */
struct Fixture {
    std::shared_ptr<FakeServerState> state = std::make_shared<FakeServerState>();

    std::unique_ptr<TcpConnector> create(const std::chrono::milliseconds reconnectDelay = 10ms) {
        TcpConfig config("fake.example", 10000);
        config.reconnectDelay = reconnectDelay;
        config.maxReconnectDelay = reconnectDelay * 4;
        return std::make_unique<TcpConnector>(config, [state = this->state](const TcpConfig &) {
            const std::scoped_lock lock(state->mutex);
            ++state->connectAttempts;
            if (state->failingConnects > 0) {
                --state->failingConnects;
                throw std::runtime_error("Connection refused");
            }
            return std::make_unique<FakeTcpSocket>(state);
        });
    }
};

}  // namespace

TEST(TcpConnectorTest, connectsOnConstruction) {
    Fixture fixture;
    const auto connector = fixture.create();

    EXPECT_EQ(connector->getState(), State::Ready);
    EXPECT_EQ(fixture.state->connectAttempts, 1);
}

TEST(TcpConnectorTest, constructionDoesNotFailIfServerIsUnreachable) {
    Fixture fixture;
    fixture.state->failingConnects = 1;
    const auto connector = fixture.create(1h);

    EXPECT_EQ(connector->getState(), State::NotReady);
}

TEST(TcpConnectorTest, sendWritesData) {
    Fixture fixture;
    const auto connector = fixture.create();

    connector->send({0x00, 0x00, 0x06, 0x01, 0xff, 0xff});

    ASSERT_EQ(fixture.state->written.size(), 1U);
    EXPECT_THAT(fixture.state->written[0], ElementsAre(0x00, 0x00, 0x06, 0x01, 0xff, 0xff));
}

TEST(TcpConnectorTest, sendRejectsEmptyData) {
    Fixture fixture;
    const auto connector = fixture.create();

    EXPECT_THROW(connector->send({}), std::runtime_error);
}

TEST(TcpConnectorTest, sendFailsIfNotConnected) {
    Fixture fixture;
    fixture.state->failingConnects = 1;
    const auto connector = fixture.create(1h);

    try {
        connector->send({0x01});
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error &e) {
        EXPECT_THAT(e.what(), HasSubstr("not connected"));
    }
    // Reconnection delay has not elapsed yet
    EXPECT_EQ(fixture.state->connectAttempts, 1);
}

TEST(TcpConnectorTest, sendFailureDisconnects) {
    Fixture fixture;
    const auto connector = fixture.create(1h);
    fixture.state->failWrite = true;

    EXPECT_THROW(connector->send({0x01}), std::runtime_error);
    EXPECT_EQ(connector->getState(), State::NotReady);
}

TEST(TcpConnectorTest, receiveReturnsData) {
    Fixture fixture;
    const auto connector = fixture.create();
    fixture.state->incoming.push_back({0x01, 0x02, 0x03});

    EXPECT_THAT(connector->receive(), ElementsAre(0x01, 0x02, 0x03));
    EXPECT_THAT(connector->receive(), IsEmpty());
}

TEST(TcpConnectorTest, reconnectsAfterConnectionLoss) {
    Fixture fixture;
    const auto connector = fixture.create();
    fixture.state->closeOnRead = true;

    EXPECT_THAT(connector->receive(), IsEmpty());
    EXPECT_EQ(connector->getState(), State::NotReady);

    std::this_thread::sleep_for(20ms);
    fixture.state->incoming.push_back({0x42});
    EXPECT_THAT(connector->receive(), ElementsAre(0x42));
    EXPECT_EQ(connector->getState(), State::Ready);
    EXPECT_EQ(fixture.state->connectAttempts, 2);
}

TEST(TcpConnectorTest, retriesFailedConnectionWithBackoff) {
    Fixture fixture;
    fixture.state->failingConnects = 2;
    const auto connector = fixture.create(10ms);
    EXPECT_EQ(fixture.state->connectAttempts, 1);

    // Delay of the second attempt is 10 ms, of the third one 20 ms
    const auto deadline = std::chrono::steady_clock::now() + 1s;
    while (connector->getState() != State::Ready && std::chrono::steady_clock::now() < deadline) {
        (void) connector->receive();
    }

    EXPECT_EQ(connector->getState(), State::Ready);
    EXPECT_EQ(fixture.state->connectAttempts, 3);
}

TEST(TcpConnectorTest, listeningLoopDeliversData) {
    Fixture fixture;
    const auto connector = fixture.create();
    std::mutex mutex;
    std::vector<std::vector<uint8_t>> received;
    connector->registerResponseHandler([&](const std::vector<uint8_t> &data) {
        const std::scoped_lock lock(mutex);
        received.push_back(data);
        return 0;
    }, AccessType::Normal);
    {
        const std::scoped_lock lock(fixture.state->mutex);
        fixture.state->incoming.push_back({0x0a, 0x0b});
    }

    connector->listen();
    const auto deadline = std::chrono::steady_clock::now() + 1s;
    while (std::chrono::steady_clock::now() < deadline) {
        {
            const std::scoped_lock lock(mutex);
            if (!received.empty()) {
                break;
            }
        }
        std::this_thread::sleep_for(1ms);
    }
    connector->stopListen();

    ASSERT_EQ(received.size(), 1U);
    EXPECT_THAT(received[0], ElementsAre(0x0a, 0x0b));
}

}  // namespace iqrf::connector::tcp
