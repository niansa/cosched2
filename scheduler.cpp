#include "scheduler.hpp"

#include <limits>
#include <algorithm>



namespace CoSched {
void CoSched::Task::kill() {
    get_scheduler().delete_task(this);
}

AwaitableTask<bool> Task::yield() {
    // If it was terminating, it can finally be declared dead now
    if (state == TaskState::terminating) {
        state = TaskState::dead;
        co_return false;
    }
    // Dead tasks may not yield
    if (state == TaskState::dead) {
        co_return false;
    }
    if (this != current) co_return true;
    // It's just sleeping
    state = TaskState::sleeping;
    // Create event for resume
    resume_event = std::make_unique<SingleEvent<void>>();
    // Let's wait until we're back up!
    stopped_at = std::chrono::system_clock::now();
    co_await *resume_event;
    // Delete resume event
    resume_event = nullptr;
    // If task was terminating during sleep, it can finally be declared dead now
    if (state == TaskState::terminating) {
        state = TaskState::dead;
        co_return false;
    }
    // Here we go, let's keep going...
    state = TaskState::running;
    co_return true;
}


void Scheduler::clean_task(Task *task) {
    // If current task isn't sleeping, it is considered a zombie so removed from list
    if (task && task->state != TaskState::sleeping) {
        delete_task(std::exchange(task, nullptr));
    }
}

void Scheduler::delete_task(Task *task) {
    std::scoped_lock L(tasks_mutex);
    tasks.erase(std::find_if(tasks.begin(), tasks.end(), [task] (const auto& o) {return o.get() == task;}));
}

Task *Scheduler::get_next_task() {
    std::scoped_lock L(tasks_mutex);

    // Get tasks with highest priority
    std::vector<Task*> max_prio_tasks;
    Priority max_prio = std::numeric_limits<Priority>::min();
    for (auto& task : tasks) {
        // Filter tasks can't currently be resumed
        if (task->resume_event == nullptr) continue;
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

void Scheduler::run_once() {
    // Clean up old task
    clean_task(Task::current);

    // Get new task
    Task::current = get_next_task();

    // Resume task if any
    if (Task::current) Task::current->resume_event->set();
}


thread_local Task *Task::current;
}
