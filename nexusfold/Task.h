#pragma once

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

}  // namespace nexusfold