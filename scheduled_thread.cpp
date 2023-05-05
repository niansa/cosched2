#include "scheduled_thread.hpp"



namespace CoSched {
void ScheduledThread::main_loop() {
    // Create scheduler
    Scheduler sched;
    // Loop until shutdown is requested
    while (!shutdown_requested) {
        // Start all new tasks enqueued
        {
            std::unique_lock L(queue_mutex);
            while (!queue.empty()) {
                // Get queue entry
                auto e = std::move(queue.front());
                queue.pop();
                // Unlock queue
                L.unlock();
                // Create task for it
                sched.create_task(e.task_name);
                // Move start function somewhere else
                auto& start_fcn = std::any_cast<decltype(e.start_fcn)&>(Task::get_current().properties.emplace("start_function", std::move(e.start_fcn)).first->second);
                // Call start function
                start_fcn();
                // Lock queue
                L.lock();
            }
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


thread_local ScheduledThread *ScheduledThread::current;
}
