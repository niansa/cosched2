#ifndef _SCHEDULER_HPP
#define _SCHEDULER_HPP
#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <limits>
#include <chrono>
#include <algorithm>
#include <async/result.hpp>
#include <async/recurring-event.hpp>


namespace CoSched {
using Priority = uint8_t;
enum {
    PRIO_LOWEST = -99,
    PRIO_LOW = -20,
    PRIO_NORMAL = 0,
    PRIO_HIGH = 20,
    PRIO_HIGHEST = 99,
    PRIO_REALTIME = PRIO_HIGHEST
};

enum class TaskState {
    running,
    sleeping,
    starting,
    stopping
};


class Task {
    friend class Scheduler;

    class Scheduler *scheduler;
    async::recurring_event resume;

    std::chrono::system_clock::time_point stopped_at;

    std::string name;
    Priority priority = PRIO_NORMAL;
    TaskState state = TaskState::starting;

public:
    Task(Scheduler *scheduler, const std::string& name)
        : scheduler(scheduler), name(name) {}
    Task(const Task&) = delete;
    Task(Task&&) = delete;

    const std::string& get_name() const {
        return name;
    }
    void set_name(const std::string& value) {
        name = value;
    }

    Priority get_priority() const {
        return priority;
    }
    void set_priority(Priority value) {
        priority = value;
    }

    TaskState get_state() const {
        return state;
    }

    Scheduler& get_scheduler() const {
        return *scheduler;
    }

    async::result<void> yield();
};


class TaskPtr {
    unsigned *ref_count;
    Task *task;

    void finalize();

public:
    TaskPtr(Task *task) : task(task) {
        ref_count = new unsigned(1);
    }
    ~TaskPtr() {
        if (!task) return;
        if (--*ref_count == 0) {
            delete ref_count;
            finalize();
        }
    }
    TaskPtr(const TaskPtr& o) : ref_count(o.ref_count), task(o.task) {
        ++*ref_count;
    }
    TaskPtr(TaskPtr&& o) : ref_count(o.ref_count), task(o.task) {
        o.task = nullptr;
    }

    auto operator ->() {
        return task;
    }
};


class Scheduler {
    friend class Task;
    friend class TaskPtr;

    std::mutex tasks_mutex;
    std::vector<std::unique_ptr<Task>> tasks;

    async::result<void> yield(Task *task);

    void delete_task(Task *task) {
        std::scoped_lock L(tasks_mutex);
        tasks.erase(std::find_if(tasks.begin(), tasks.end(), [task] (const auto& o) {return o.get() == task;}));
    }

    Task *get_next_task();

public:
    Scheduler() {}
    Scheduler(const Scheduler&) = delete;
    Scheduler(Scheduler&&) = delete;

    const auto& get_tasks() const {
        return tasks;
    }

    TaskPtr create_task(const std::string& name) {
        std::scoped_lock L(tasks_mutex);
        return TaskPtr(tasks.emplace_back(std::make_unique<Task>(this, name)).get());
    }

    void run() {
        while (!tasks.empty()) {
            // Get next task
            auto next_task = get_next_task();

            // Resume task if any
            if (next_task) next_task->resume.raise();
        }
    }
};


inline async::result<void> Task::yield() {
    co_await scheduler->yield(this);
}

inline void TaskPtr::finalize() {
    task->get_scheduler().delete_task(task);
}
}
#endif // _SCHEDULER_HPP
