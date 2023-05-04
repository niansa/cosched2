#include "scheduler.hpp"

#include <iostream>
#include <string>



async::result<void> test_task(CoSched::TaskPtr t) {
    for (unsigned x = 100; x != 0; x--) {
        std::cout << t->get_name() << ": " << x << '\n';
        co_await t->yield();
    }
}

int main () {
    CoSched::Scheduler scheduler;
    for (const auto& name : {"A", "B", "C", "D", "E", "F"}) {
        auto task = scheduler.create_task(name);
        async::detach(test_task(task));
        if (task->get_name() == "B" || task->get_name() == "D") {
            task->set_priority(CoSched::PRIO_HIGH);
        }
    }
    scheduler.run();
}
