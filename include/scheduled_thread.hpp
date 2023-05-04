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

    void main_loop();

public:
    ScheduledThread() {}

    // MUST NOT already be running
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

    // MUST already be running
    void wait() {
        joined = true;
        thread.join();
    }

    // MUST already be running
    void shutdown() {
        enqueue([this] (Scheduler&) {
            shutdown_requested = true;
        });
    }
};
}
#endif // SCHEDULED_THREAD_HPP
