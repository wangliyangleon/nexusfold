#include <iostream>
#include <memory>
#include <stdexcept>

#include "../nexusfold/Nexusfold.h"

namespace {

class TestTask : public nexusfold::TaskBase {
   public:
    TestTask(std::string_view task_name) : task_name_(task_name) {}

   protected:
    void* task_body(void*) override {
        std::cout << "Hello from Nexusfold task: " << task_name_ << std::endl;
        return nullptr;
    }

   private:
    std::string_view task_name_;
};

class ExpTask : public nexusfold::TaskBase {
   protected:
    void* task_body(void*) override {
        std::cout << "Hello before I die" << std::endl;
        throw std::runtime_error("Now I'm dead");
    }
};

}  // namespace

int main() {
    nexusfold::TaskScheduler task_scheduler;
    task_scheduler.add_task(std::make_unique<TestTask>("Task 1"), "t1")
        .add_task(std::make_unique<TestTask>("Task 2"), "t2")
        .add_task(std::make_unique<TestTask>("Task 3"), "t3")
        .add_task(std::make_unique<TestTask>("Task 4"), "t4")
        .add_task(std::make_unique<nexusfold::SimpleTask>([]() {
                      std::cout << "Hello from SimpleTask 5" << std::endl;
                  }),
                  "t5")
        // t3->t2->t1
        //     t5---^
        //      |->t4
        .apply_dependency("t3", "t2")
        .apply_dependency("t2", "t1")
        .apply_dependency("t5", "t1")
        .apply_dependency("t5", "t4");
    std::cout << task_scheduler.execute() << std::endl;

    std::cout << task_scheduler.execute(nexusfold::TaskScheduler::par)
              << std::endl;

    task_scheduler.apply_dependency("t4", "t5");
    std::cout << task_scheduler.execute() << std::endl;

    task_scheduler.add_task(std::make_unique<ExpTask>(), "t6");
    std::cout << task_scheduler.execute() << std::endl;
    return 0;
}