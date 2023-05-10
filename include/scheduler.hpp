#ifndef _SCHEDULER_HPP
#define _SCHEDULER_HPP
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <any>
#include <AwaitableTask.hpp>
#include <SingleEvent.hpp>


namespace CoSched {
using namespace basiccoro;


using Priority = int8_t;
enum {
    PRIO_LOWEST = -99,
    PRIO_LOW = -20,
    PRIO_NORMAL = 0,
    PRIO_HIGH = 20,
    PRIO_HIGHEST = 99,
    PRIO_REALTIME = PRIO_HIGHEST
};

enum class TaskState {
    running, // Task is currently in a normal running state
    sleeping, // Task is currently waiting to be scheduled again
    terminating, // Task will start terminating soon
    dead // Taks is currently terminating
};


std::string_view get_state_string(TaskState);


class Task {
    friend class Scheduler;

    static thread_local class Task *current;

    class Scheduler *scheduler;
    std::unique_ptr<SingleEvent<void>> resume_event = nullptr;

    std::chrono::system_clock::time_point stopped_at;

    std::string name;
    Priority priority = PRIO_NORMAL;
    TaskState state = TaskState::running;
    bool suspended = false;

    void kill();

public:
    Task(Scheduler *scheduler, const std::string& name)
        : scheduler(scheduler), name(name) {}
    Task(const Task&) = delete;
    Task(Task&&) = delete;

    // Misc property storage, unused
    std::unordered_map<std::string, std::any> properties;

    // Returns the task that is currently being executed on this thread
    static inline
    Task& get_current() {
        return *current;
    }

    // Sets a task name
    const std::string& get_name() const {
        return name;
    }
    void set_name(const std::string& value) {
        name = value;
    }

    // Sets the task priority
    Priority get_priority() const {
        return priority;
    }
    void set_priority(Priority value) {
        priority = value;
    }

    // Returns the state of this task
    TaskState get_state() const {
        return state;
    }
    std::string_view get_state_string() const {
        return ::CoSched::get_state_string(state);
    }

    // Returns the scheduler that is scheduling this task
    Scheduler& get_scheduler() const {
        return *scheduler;
    }

    // Terminates the task as soon as possible
    void terminate() {
        state = TaskState::terminating;
    }

    // Suspends (pauses) the task as soon as possible
    void set_suspended(bool value = true) {
        suspended = value;
    }
    bool is_suspended() const {
        return suspended;
    }

    // Returns if task is dead
    bool is_dead() const {
        return state == TaskState::dead;
    }

    // Allows other tasks to execute
    AwaitableTask<bool> yield();
};


class Scheduler {
    friend class Task;
    friend class TaskPtr;

    std::vector<std::unique_ptr<Task>> tasks;

    void clean_task(Task *task);
    void delete_task(Task *task);
    Task *get_next_task();

public:
    Scheduler() {}
    Scheduler(const Scheduler&) = delete;
    Scheduler(Scheduler&&) = delete;

    // Returns all tasks
    const auto& get_tasks() const {
        return tasks;
    }

    // Checks if there is nothing left to do
    bool has_work() const {
        return !tasks.empty();
    }

    // Creates new task, returns it and switches to it
    // DO NOT call from within a task
    void create_task(const std::string& name) {
        // Clean up old task
        clean_task(Task::current);

        // Create and switch to new task
        Task::current = tasks.emplace_back(std::make_unique<Task>(this, name)).get();
    }

    // Run until there are no more tasks left to process
    // DO NOT call from within a task
    void run() {
        // Repeat while we have work to do
        while (has_work()) {
            run_once();
        }
    }

    // Run once
    // DO NOT call from within a task
    void run_once();
};
}
#endif // _SCHEDULER_HPP
