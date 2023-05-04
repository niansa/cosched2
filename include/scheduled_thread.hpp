#ifndef SCHEDULED_THREAD_HPP
#define SCHEDULED_THREAD_HPP
#include "scheduler.hpp"

#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>



namespace CoSched {
class ScheduledThread {
    std::thread thread;
    std::mutex queue_mutex;
    std::queue<std::function<void (Scheduler&)>> queue;
    std::mutex conditional_mutex;
    std::condition_variable conditional_lock;
    bool shutdown_requested = false;
    bool joined = false;

    void main_loop() {
        // Create scheduler
        Scheduler sched;
        // Loop until shutdown is requested
        while (!shutdown_requested) {
            // Start all new tasks enqueued
            std::scoped_lock L(queue_mutex);
            while (!queue.empty()) {
                auto f = std::move(queue.front());
                queue.pop();
                f(sched);
            }
            // Run once
            sched.run_once();
            // Wait for work if there is none
            if (!sched.has_work()) {
                if (joined) break;
                std::unique_lock<std::mutex> lock(conditional_mutex);
                conditional_lock.wait(lock);
            }
        }
    }

public:
    ScheduledThread() {}

    void start() {
        thread = std::thread([this] () {
            main_loop();
        });
    }

    // DO NOT call from within a task
    void enqueue(const std::function<void (Scheduler&)>& f) {
        // Enqueue function
        {
            std::scoped_lock L(queue_mutex);
            queue.emplace(f);
        }

        // Notify thread
        conditional_lock.notify_one();
    }

    void wait() {
        joined = true;
        thread.join();
    }

    void shutdown() {
        enqueue([this] (Scheduler&) {
            shutdown_requested = true;
        });
    }
};
}
#endif // SCHEDULED_THREAD_HPP
