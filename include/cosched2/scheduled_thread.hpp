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
    static thread_local ScheduledThread *current;

    struct QueueEntry {
        std::string task_name;
        std::function<void ()> start_fcn;
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

    // Current thread MUST be made by start()
    inline static
    ScheduledThread *get_current() {
        return current;
    }

    // MUST NOT already be running
    void start() {
        thread = std::thread([this] () {
            current = this;
            main_loop();
        });
    }

    // Can be called from anywhere
    void create_task(const std::string& task_name, std::function<void ()>&& task_fcn) {
        // Enqueue function
        {
            std::scoped_lock L(queue_mutex);
            queue.emplace(QueueEntry{task_name, std::move(task_fcn)});
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
        create_task("Shutdown Initiator", [this] () {
                    shutdown_requested = true;
                });
    }
};
}
#endif // SCHEDULED_THREAD_HPP
