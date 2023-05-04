#include "scheduler.hpp"

#include <limits>
#include <algorithm>



void CoSched::Task::kill() {
    get_scheduler().delete_task(this);
}

async::result<bool> CoSched::Task::yield() {
    if (state == TaskState::terminating) {
        // If it was terminating, it can finally be declared dead now
        state = TaskState::dead;
        co_return false;
    }
    // It's just sleeping
    state = TaskState::sleeping;
    // Let's wait until we're back up!
    stopped_at = std::chrono::system_clock::now();
    co_await resume.async_wait();
    // Here we go, let's keep going...
    state = TaskState::running;
    co_return true;
}


void CoSched::Scheduler::delete_task(Task *task) {
    std::scoped_lock L(tasks_mutex);
    tasks.erase(std::find_if(tasks.begin(), tasks.end(), [task] (const auto& o) {return o.get() == task;}));
}

CoSched::Task *CoSched::Scheduler::get_next_task() {
    std::scoped_lock L(tasks_mutex);

    // Get tasks with highest priority
    std::vector<Task*> max_prio_tasks;
    Priority max_prio = std::numeric_limits<Priority>::min();
    for (auto& task : tasks) {
        // Filter tasks that aren't sleeping
        if (task->state != TaskState::sleeping) continue;
        // Filter tasks that are suspended
        if (task->suspended) continue;
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

void CoSched::Scheduler::run() {
    while (!tasks.empty()) {
        // If current task isn't sleeping, it is considered a zombie so removed from list
        if (Task::current && Task::current->state != TaskState::sleeping) {
            delete_task(std::exchange(Task::current, nullptr));
        }

        // Get new task
        Task::current = get_next_task();

        // Resume task if any
        if (Task::current) Task::current->resume.raise();
    }
}

thread_local CoSched::Task *CoSched::Task::current;
