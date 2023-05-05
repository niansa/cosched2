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
    struct QueueEntry {
        std::string task_name;
        std::function<AwaitableTask<void> ()> task_fcn;
    };

    std::thread thread;
    std::mutex queue_mutex;
    std::queue<QueueEntry> queue;
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
    void create_task(const std::string& task_name, const std::function<AwaitableTask<void> ()>& task_fcn) {
        // Enqueue function
        {
            std::scoped_lock L(queue_mutex);
            queue.emplace(QueueEntry{task_name, task_fcn});
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
        create_task("Shutdown Initiator", [this] () -> AwaitableTask<void> {
                    shutdown_requested = true;
                    co_return;
                });
    }
};
}
#endif // SCHEDULED_THREAD_HPP
