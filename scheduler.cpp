#include "cosched2/scheduler.hpp"
#define MINICORO_IMPL
#include "minicoro.h"

#include <limits>
#include <algorithm>



namespace CoSched {
std::string_view get_state_string(TaskState state) {
    switch (state) {
    case TaskState::dead: return "dead";
    case TaskState::running: return "running";
    case TaskState::sleeping: return "sleeping";
    case TaskState::terminating: return "terminating";
    default: return "invalid";
    }
}


void CoSched::Task::kill() {
    get_scheduler().delete_task(this);
}

bool Task::yield() {
    // If it was terminating, it can finally be declared dead now
    if (state == TaskState::terminating) {
        state = TaskState::dead;
        return false;
    }
    // Dead tasks may not yield
    if (state == TaskState::dead) {
        return false;
    }
    if (this != current) return true;
    // It's just sleeping
    state = TaskState::sleeping;
    // Let's wait until we're back up!
    stopped_at = std::chrono::system_clock::now();
    if (mco_yield(coroutine) != MCO_SUCCESS)
        return false;
    // If task was terminating during sleep, it can finally be declared dead now
    if (state == TaskState::terminating) {
        state = TaskState::dead;
        return false;
    }
    // Here we go, let's keep going...
    state = TaskState::running;
    return true;
}


void Scheduler::clean_task(Task *task) {
    // If current task has no way to resume, it is considered a zombie so removed from list
    if (task && task->state == TaskState::deleting) {
        delete_task(task);
    }
}

void Scheduler::delete_task(Task *task) {
    mco_destroy(task->coroutine);
    tasks.erase(std::find_if(tasks.begin(), tasks.end(), [task] (const auto& o) {return o.get() == task;}));
}

Task *Scheduler::get_next_task() {
    // Get tasks with highest priority
    std::vector<Task*> max_prio_tasks;
    Priority max_prio = std::numeric_limits<Priority>::min();
    for (auto& task : tasks) {
        // Filter tasks can't currently be resumed
        if (task->state == TaskState::running) continue;
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
    if (Task::current) mco_resume(Task::current->coroutine);
}


thread_local Task *Task::current;
}
