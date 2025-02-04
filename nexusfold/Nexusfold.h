#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Task.h"

namespace nexusfold {

class TaskScheduler {
   public:
    TaskScheduler& add_task(std::unique_ptr<TaskBase> new_task,
                            std::string_view task_id);
    TaskScheduler& apply_dependency(std::string_view prior_task_id,
                                    std::string_view subsequent_task_id);

    enum ExecutionStatus {
        execute_policy_not_implemented,
        all_secceed,
        has_scheduler_error,
        has_failed_task,
        has_unstarted_task
    };
    enum ExecutionPolicy { seq, par };

    ExecutionStatus execute(ExecutionPolicy execution_policy = seq);

   private:
    ExecutionStatus execute_seq_();
    ExecutionStatus execute_par_();
    std::unordered_map<std::string_view, std::unique_ptr<TaskBase>> task_map_;
    std::unordered_map<std::string_view, std::vector<std::string_view>>
        subsequent_tasks_map_;
};

}  // namespace nexusfold