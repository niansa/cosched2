#ifndef SCHEDULER_MUTEX_HPP
#define SCHEDULER_MUTEX_HPP
#include "scheduler.hpp"

#include <queue>


namespace CoSched {
class [[nodiscard("Discarding the lock guard will release the lock immediately.")]] LockGuard {
    class Mutex *mutex;

    void unlock();

public:
    LockGuard() : mutex(nullptr) {}
    LockGuard(Mutex *m) : mutex(m) {}
    LockGuard(const LockGuard&) = delete;
    LockGuard(LockGuard&& o) : mutex(o.mutex) {
        o.mutex = nullptr;
    }
    ~LockGuard() {
        if (mutex) unlock();
    }

    auto& operator =(LockGuard&& o) {
        mutex = o.mutex;
        o.mutex = nullptr;
        return *this;
    }
};


class Mutex {
    Task *holder = nullptr;
    std::queue<Task*> resume_on_unlock;

public:
    Mutex() {}
    Mutex(const Mutex&) = delete;
    Mutex(Mutex&&) = delete;
    ~Mutex() {
        unlock();
    }

    LockGuard lock() {
        auto& task = Task::get_current();
        // Make sure the lock is not already held by same task
        if (holder == &task) return LockGuard();
        // Just hold lock and return if lock isn't currently being held
        if (!holder) {
            holder = &task;
            return LockGuard(this);
        }
        // Lock is already being held, add task to queue and suspend until lock is passed
        resume_on_unlock.push(&task);
        task.set_suspended(true);
        task.yield();
        return LockGuard(this);
    }
    bool unlock() {
        auto& task = Task::get_current();
        // Make sure we are actually the ones holding the lock
        if (holder != &task) return false;
        // If nothing is waiting for the lock to release, just release it and we're done
        if (resume_on_unlock.empty()) {
            holder = nullptr;
            return true;
        }
        // Something is waiting or the lock to be released, just pass it by.
        auto next_task = resume_on_unlock.front();
        next_task->set_suspended(false);
        holder = next_task;
        resume_on_unlock.pop();
        return true;
    }
};


inline void LockGuard::unlock() {
    mutex->unlock();
}
}
#endif // SCHEDULER_MUTEX_HPP
