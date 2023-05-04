#include "scheduler.hpp"

#include <iostream>
#include <string>



async::result<void> test_task() {
    auto& task = CoSched::Task::get_current();
    for (unsigned x = 100; co_await task.yield(); x--) {
        std::cout << task.get_name() << ": " << x << '\n';
        if (x == 10) task.terminate();
    }
}

int main () {
    CoSched::Scheduler scheduler;
    for (const auto& name : {"A", "B", "C", "D", "E", "F"}) {
        scheduler.create_task(name);
        async::detach(test_task());
        auto& task = CoSched::Task::get_current();
        if (task.get_name() == "B" || task.get_name() == "D") {
            task.set_priority(CoSched::PRIO_HIGH);
        }
    }
    scheduler.run();
}
