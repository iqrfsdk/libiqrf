/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: TaskQueue.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-12
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */
#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>

namespace iqrf::dpa {

/**
 * Maintain queue of tasks and invoke sequential processing
 *
 * Provide asynchronous processing of incoming tasks of type T in dedicated worker thread.
 * The tasks are processed in FIFO way. Processing function is passed as parameter in constructor.
 *
 * Tasks pushed after the queue has been stopped are not processed, the remaining tasks are dropped on stop.
 */
template <class T>
class TaskQueue {
 public:
    /// Processing function type
    using ProcessTaskFunc = std::function<void(T)>;

    /**
     * Constructs the queue and starts the worker thread
     *
     * Processing function is used in dedicated worker thread to process incoming queued tasks.
     * The function must be thread safe. The worker thread is started.
     *
     * @param processTask Processing function
     */
    explicit TaskQueue(ProcessTaskFunc processTask): processTask(std::move(processTask)) {
        this->workerThread = std::thread(&TaskQueue::worker, this);
    }

    /**
     * Stops the queue and joins the worker thread
     *
     * Stops working thread
     */
    ~TaskQueue() {
        this->stop();
        if (this->workerThread.joinable()) {
            this->workerThread.join();
        }
    }

    TaskQueue(const TaskQueue&) = delete;
    TaskQueue& operator=(const TaskQueue&) = delete;
    TaskQueue(TaskQueue&&) = delete;
    TaskQueue& operator=(TaskQueue&&) = delete;

    /**
     * Pushes the task to the queue
     *
     * Pushes task to queue to be processed in worker thread. The task type T has to be copyable
     * as the copy is pushed to queue container
     *
     * @param task Task to process
     * @return Number of tasks in the queue including the pushed one (size of queue)
     */
    std::size_t push(const T &task) {
        std::size_t count = 0;
        {
            const std::scoped_lock lock(this->mutex);
            this->tasks.push_back(task);
            count = this->tasks.size();
        }
        this->condition.notify_all();
        return count;
    }

    /**
     * Stops the queue, the worker thread finishes the task in progress
     *
     * Worker thread is explicitly stopped
     */
    void stop() {
        {
            const std::scoped_lock lock(this->mutex);
            this->running = false;
        }
        this->condition.notify_all();
    }

    /**
     * Returns the number of the tasks waiting in the queue (actual queue size)
     */
    [[nodiscard]] std::size_t size() const {
        const std::scoped_lock lock(this->mutex);
        return this->tasks.size();
    }

 private:
    /// Worker thread function
    void worker() {
        std::unique_lock lock(this->mutex);
        while (this->running) {
            // wait for something in the queue, lock is released in wait and reacquired here
            this->condition.wait(lock, [this] { return !this->tasks.empty() || !this->running; });
            while (this->running && !this->tasks.empty()) {
                T task = this->tasks.front();
                this->tasks.pop_front();
                lock.unlock();
                this->processTask(task);
                lock.lock();
            }
        }
    }

    /// Guards the tasks and the running flag
    mutable std::mutex mutex;
    /// Signals new tasks and stop
    std::condition_variable condition;
    /// Waiting tasks
    std::deque<T> tasks;
    /// False after stop
    bool running = true;
    /// Function processing a task
    ProcessTaskFunc processTask;
    /// Worker thread
    std::thread workerThread;
};

}  // namespace iqrf::dpa
