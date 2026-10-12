/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: TaskQueueTest.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-12
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <gtest/gtest.h>

#include <condition_variable>
#include <mutex>
#include <vector>

#include "iqrf/dpa/TaskQueue.h"

namespace iqrf::dpa {

TEST(TaskQueueTest, processesTasksInOrder) {
    std::mutex mutex;
    std::condition_variable condition;
    std::vector<int> processed;
    {
        TaskQueue<int> queue([&](int task) {
            const std::scoped_lock lock(mutex);
            processed.push_back(task);
            condition.notify_all();
        });
        EXPECT_EQ(queue.push(1), 1U);
        queue.push(2);
        queue.push(3);
        std::unique_lock lock(mutex);
        condition.wait(lock, [&] { return processed.size() == 3; });
    }
    EXPECT_EQ(processed, (std::vector<int>{1, 2, 3}));
}

TEST(TaskQueueTest, stoppedQueueDoesNotProcessTasks) {
    std::mutex mutex;
    int processed = 0;
    TaskQueue<int> queue([&](int) {
        const std::scoped_lock lock(mutex);
        ++processed;
    });
    queue.stop();
    queue.push(1);
    queue.push(2);
    // The destructor joins the worker thread, the tasks pushed after stop are never processed
}

}  // namespace iqrf::dpa
