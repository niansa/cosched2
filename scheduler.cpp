#include "scheduler.hpp"



async::result<void> CoSched::Scheduler::yield(Task *task) {
    task->state = TaskState::sleeping;
    task->stopped_at = std::chrono::system_clock::now();
    co_await task->resume.async_wait();
    task->state = TaskState::running;
}

CoSched::Task *CoSched::Scheduler::get_next_task() {
    std::scoped_lock L(tasks_mutex);

    // Get tasks with highest priority
    std::vector<Task*> max_prio_tasks;
    Priority max_prio = std::numeric_limits<Priority>::min();
    for (auto& task : tasks) {
        // Filter tasks that aren't sleeping
        if (task->state != TaskState::sleeping) continue;
        // Update max priority
        if (task->priority > max_prio) {
            max_prio = task->priority;
            max_prio_tasks.clear();
        }
        // Add task if matching
        if (task->priority == max_prio) {
            max_prio_tasks.push_back(task.get());
        }
    }

    // Get least recently stopped task
    Task *next_task = nullptr;
    for (auto task : max_prio_tasks) {
        if (!next_task || task->stopped_at < next_task->stopped_at) {
            next_task = task;
        }
    }

    // Return next task;
    return next_task;
}
