#include "scheduled_thread.hpp"

#include <iostream>
#include <string>



class LifetimeTest {
    std::string read_test_str = "Test value";

public:
    LifetimeTest() {
        std::cout << this << ": Lifetime start" << std::endl;
    }
    LifetimeTest(const LifetimeTest&) {
        std::cout << this << ": Lifetime copy" << std::endl;
    };
    LifetimeTest(LifetimeTest&&) {
        std::cout << this << ": Lifetime move" << std::endl;
    }
    ~LifetimeTest() {
        std::cout << this << ": Lifetime end" << std::endl;
    }

    void read_test() const {
        std::cout << read_test_str << std::flush;
        std::cout << '\r';
        for (unsigned i = 0; i != read_test_str.size(); i++) {
            std::cout << ' ';
        }
        std::cout << '\r' << std::flush;
        std::cout << this << ": Lifetime read test success" << std::endl;
    }
};

int main () {
    CoSched::ScheduledThread scheduler;
    for (const auto& name : {"A", "B", "C"}) {
        scheduler.create_task(name, [lt = LifetimeTest()] () -> CoSched::AwaitableTask<void> {
            auto& task = CoSched::Task::get_current();
            std::cout << task.get_name() << "Scope start" << std::endl;
            lt.read_test();
            if (!co_await task.yield()) co_return;
            std::cout << task.get_name() << "Scope middle" << std::endl;
            lt.read_test();
            if (!co_await task.yield()) co_return;
            std::cout << task.get_name() << "Scope end" << std::endl;
            lt.read_test();
        });
    }
    scheduler.start();
    scheduler.wait();
}
