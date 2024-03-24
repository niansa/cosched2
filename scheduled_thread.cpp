#include "scheduled_thread.hpp"
#include "minicoro.h"



namespace CoSched {
void ScheduledThread::main_loop() {
    // Create scheduler
    Scheduler sched;
    // Loop until shutdown is requested
    while (!shutdown_requested) {
        // Start all new tasks enqueued
        {
            std::unique_lock<std::mutex> L(queue_mutex);
            while (!queue.empty()) {
                // Get queue entry
                auto e = std::move(queue.front());
                queue.pop();
                // Unlock queue
                L.unlock();
                // Create task for it
                sched.create_task(e.task_name);
                // Move start function
                Task::current->start_fcn = std::move(e.start_fcn);
                // Create coroutine
                mco_desc desc = mco_desc_init([] (mco_coro *coro) {
                    Task::get_current().start_fcn();
                    Task::get_current().state = TaskState::deleting;
                }, 0);
                mco_create(&Task::current->coroutine, &desc);
                // Resume coroutine immediately
                mco_resume(Task::current->coroutine);
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
