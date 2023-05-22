#ifndef SCHEDULER_MUTEX_HPP
#define SCHEDULER_MUTEX_HPP
#include "scheduler.hpp"

#include <queue>


namespace CoSched {
class Mutex {
    bool held = false;
    std::queue<Task*> resume_on_unlock;

public:
    Mutex() {}
    Mutex(const Mutex&) = delete;
    Mutex(Mutex&&) = delete;
    ~Mutex() {
        unlock();
    }

    AwaitableTask<void> lock() {
        // Just hold lock and return if lock isn't currently being held
        if (!held) {
            held = true;
            co_return;
        }
        // Lock is already being held, add task to queue and suspend until lock is passed
        auto& task = Task::get_current();
        resume_on_unlock.push(&task);
        task.set_suspended(true);
        co_await task.yield();
    }
    void unlock() {
        // If nothing is waiting for the lock to release, just release it and we're done
        if (resume_on_unlock.empty()) {
            held = false;
            return;
        }
        // Something is waiting or the lock to be released, just pass it by.
        resume_on_unlock.front()->set_suspended(false);
        resume_on_unlock.pop();
    }
};


class ScopedLock {
    Mutex& mutex;
    bool held_by_us = false;

public:
    ScopedLock(Mutex &m) : mutex(m) {}
    ScopedLock(const ScopedLock&) = delete;
    ScopedLock(ScopedLock&&) = delete;
    ~ScopedLock() {
        if (held_by_us) unlock();
    }

    AwaitableTask<void> lock() {
        co_await mutex.lock();
        held_by_us = true;
    }
    void unlock() {
        mutex.unlock();
        held_by_us = false;
    }
};
}
#endif // SCHEDULER_MUTEX_HPP
