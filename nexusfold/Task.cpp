#include "Task.h"

namespace nexusfold {
void TaskBase::execute(void* task_arg) {
    this->prepare();
    void* task_res = this->task_body(task_arg);
    this->postwork(task_res);
}

void TaskBase::prepare() {
    // do nothing.
}

void TaskBase::postwork(void*) {
    // do nothing.
}
}  // namespace nexusfold