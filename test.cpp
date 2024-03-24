#include <iostream>
#include <string>
#include <cosched2/scheduled_thread.hpp>



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
        scheduler.create_task(name, [lt = LifetimeTest()] () {
            auto& task = CoSched::Task::get_current();
            if (task.get_name() == "A")
                task.set_priority(CoSched::PRIO_HIGH);
            std::cout << task.get_name() << ": Scope start" << std::endl;
            lt.read_test();
            if (!task.yield()) return;
            std::cout << task.get_name() << ": Scope middle" << std::endl;
            lt.read_test();
            if (!task.yield()) return;
            std::cout << task.get_name() << ": Scope end" << std::endl;
            lt.read_test();
        });
    }
    scheduler.start();
    scheduler.wait();
}
