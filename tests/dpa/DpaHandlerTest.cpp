/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: DpaHandlerTest.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-12
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include "iqrf/connector/IConnector.h"
#include "iqrf/connector/TrInfo.h"
#include "iqrf/dpa/DpaConstants.h"
#include "iqrf/dpa/DpaHandler.h"

namespace iqrf::dpa {

namespace {

/**
 * Simulated connector: sent messages are answered by the responder, the answers are returned by receive()
 */
class FakeConnector : public connector::IConnector {
 public:
    using Responder = std::function<std::vector<std::vector<uint8_t>>(const std::vector<uint8_t> &)>;

    connector::State getState() const override { return connector::State::Ready; }

    void send(const std::vector<uint8_t> &data) override {
        const std::scoped_lock lock(this->mutex);
        this->sent.push_back(data);
        if (this->responder) {
            for (auto &answer : this->responder(data)) {
                this->incoming.push_back(std::move(answer));
            }
        }
    }

    std::vector<uint8_t> receive() override {
        {
            const std::scoped_lock lock(this->mutex);
            if (!this->incoming.empty()) {
                std::vector<uint8_t> message = std::move(this->incoming.front());
                this->incoming.pop_front();
                return message;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return {};
    }

    /// Pushes the message to the receive queue directly
    void deliver(std::vector<uint8_t> message) {
        const std::scoped_lock lock(this->mutex);
        this->incoming.push_back(std::move(message));
    }

    connector::TrInfo readTrInfo() override { throw std::runtime_error("not used"); }
    void resetTr() override {}
    void enterProgrammingMode() override {}
    void awaitProgrammingMode() override {}
    void exitProgrammingMode() override {}
    void upload(connector::ProgrammingTarget, const std::vector<uint8_t> &) override {}
    std::vector<uint8_t> download(connector::ProgrammingTarget) override { return {}; }
    std::vector<uint8_t> download(connector::ProgrammingTarget, uint16_t) override { return {}; }

    /// Messages sent by the handler
    std::vector<std::vector<uint8_t>> sent;
    /// Answers to the sent messages
    Responder responder;

 private:
    std::mutex mutex;
    std::deque<std::vector<uint8_t>> incoming;
};

/// Request to node 1, PNUM 0x06, PCMD 0x00
DpaMessage makeRequest() {
    return DpaMessage(0x0001, 0x06, 0x00, 0xFFFF, {0x01});
}

/// Confirmation with no hops: timeslot 2 (20 ms)
std::vector<uint8_t> makeConfirmation() {
    return {0x01, 0x00, 0x06, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x02, 0x00};
}

/// Response of the request with payload 0xAB
std::vector<uint8_t> makeResponse() {
    return {0x01, 0x00, 0x06, 0x80, 0xFF, 0xFF, 0x00, 0x00, 0xAB};
}

/**
 * Fixture: connector listening for the handler
 */
class DpaHandlerTest : public ::testing::Test {
 protected:
    void SetUp() override {
        this->connector.listen();
    }

    void TearDown() override {
        this->handler.reset();
        this->connector.stopListen();
    }

    FakeConnector connector;
    std::unique_ptr<DpaHandler> handler = std::make_unique<DpaHandler>(connector);
};

}  // namespace

TEST_F(DpaHandlerTest, transactionCompletesWithResponse) {
    this->connector.responder = [](const std::vector<uint8_t> &) {
        return std::vector<std::vector<uint8_t>>{makeConfirmation(), makeResponse()};
    };

    const auto transaction = this->handler->executeDpaTransaction(makeRequest(), 300);
    const auto result = transaction->get();

    EXPECT_EQ(result->errorCode(), static_cast<int>(TransactionError::Ok));
    EXPECT_TRUE(result->isConfirmed());
    EXPECT_TRUE(result->isResponded());
    EXPECT_EQ(result->getResponse().peripheralCommand(), 0x80);
    ASSERT_EQ(this->connector.sent.size(), 1U);
    EXPECT_EQ(this->connector.sent[0], makeRequest().bytes());
}

TEST_F(DpaHandlerTest, transactionTimesOutWithoutResponse) {
    const auto transaction = this->handler->executeDpaTransaction(makeRequest(), MINIMAL_TIMEOUT);
    const auto result = transaction->get();

    EXPECT_EQ(result->errorCode(), static_cast<int>(TransactionError::Timeout));
    EXPECT_FALSE(result->isResponded());
}

TEST_F(DpaHandlerTest, defaultErrorIsReportedWithoutSending) {
    const auto transaction = this->handler->executeDpaTransaction(makeRequest(), 300,
        static_cast<int>(TransactionError::QueueFull));
    const auto result = transaction->get();

    EXPECT_EQ(result->errorCode(), static_cast<int>(TransactionError::QueueFull));
    EXPECT_TRUE(this->connector.sent.empty());
}

TEST_F(DpaHandlerTest, abortedTransactionReportsAborted) {
    this->connector.responder = [](const std::vector<uint8_t> &) { return std::vector<std::vector<uint8_t>>{}; };
    const auto transaction = this->handler->executeDpaTransaction(makeRequest(), 1000);
    std::thread aborter([transaction] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        transaction->abort();
    });
    const auto result = transaction->get();
    aborter.join();

    EXPECT_EQ(result->errorCode(), static_cast<int>(TransactionError::Aborted));
}

TEST_F(DpaHandlerTest, emptyRequestIsRejected) {
    EXPECT_THROW(this->handler->executeDpaTransaction(DpaMessage(), 300), std::invalid_argument);
}

TEST_F(DpaHandlerTest, asyncMessageIsDeliveredToHandler) {
    std::vector<DpaMessage> received;
    std::mutex mutex;
    this->handler->registerAsyncMessageHandler("service", [&](const DpaMessage &message) {
        const std::scoped_lock lock(mutex);
        received.push_back(message);
    });

    // Request direction message from the node (PCMD without the response flag)
    this->connector.deliver({0x01, 0x00, 0x06, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xCD});

    for (int i = 0; i < 200; ++i) {
        {
            const std::scoped_lock lock(mutex);
            if (!received.empty()) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    const std::scoped_lock lock(mutex);
    ASSERT_EQ(received.size(), 1U);
    EXPECT_EQ(received[0].direction(), DpaMessage::Direction::Request);
}

TEST_F(DpaHandlerTest, anyMessageHandlerRejectsDuplicates) {
    this->handler->registerAnyMessageHandler("service", [](const DpaMessage &) {});
    EXPECT_THROW(this->handler->registerAnyMessageHandler("service", [](const DpaMessage &) {}), std::logic_error);
    this->handler->unregisterAnyMessageHandler("service");
    EXPECT_NO_THROW(this->handler->registerAnyMessageHandler("service", [](const DpaMessage &) {}));
}

TEST_F(DpaHandlerTest, timeoutIsRaisedToMinimum) {
    this->handler->setTimeout(10);
    EXPECT_EQ(this->handler->getTimeout(), MINIMAL_TIMEOUT);
    this->handler->setTimeout(1000);
    EXPECT_EQ(this->handler->getTimeout(), 1000);
}

}  // namespace iqrf::dpa
