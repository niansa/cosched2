#include "scheduled_thread.hpp"

#include <iostream>
#include <string>


CoSched::AwaitableTask<std::string> get_value() {
    std::string fres = CoSched::Task::get_current().get_name();
    for (unsigned it = 0; it != 100; it++) {
        fres += "Hello";
        co_await CoSched::Task::get_current().yield();
    }
    fres.resize(1);
    co_return fres;
}

CoSched::AwaitableTask<void> test_task() {
    auto& task = CoSched::Task::get_current();
    if (task.get_name() == "B" || task.get_name() == "D") {
        task.set_priority(CoSched::PRIO_HIGH);
    }
    for (unsigned x = 100; co_await task.yield(); x--) {
        std::cout << co_await get_value() << ": " << x << '\n';
        if (x == 10) task.terminate();
    }
}

int main () {
    CoSched::ScheduledThread scheduler;
    for (const auto& name : {"A", "B", "C", "D", "E", "F"}) {
        scheduler.create_task(name, test_task);
    }
    scheduler.start();
    scheduler.wait();
}
