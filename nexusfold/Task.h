#pragma once

#include <functional>

namespace nexusfold {

class TaskBase {
   public:
    virtual ~TaskBase() = default;

    void execute(void* task_arg = nullptr);

   protected:
    virtual void prepare();
    virtual void* task_body(void* task_arg = nullptr) = 0;
    virtual void postwork(void* task_res = nullptr);
};

class SimpleTask : public TaskBase {
   public:
    SimpleTask(std::function<void(void)> task_body) : task_body_(task_body) {}

    void* task_body(void* task_arg = nullptr) override {
        task_body_();
        return nullptr;
    }

   private:
    std::function<void(void)> task_body_;
};

}  // namespace nexusfold