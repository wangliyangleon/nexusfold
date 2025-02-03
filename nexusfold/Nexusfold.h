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

    enum EXECUTE_STATUS {
        NOT_STARTED,
        ALL_SUCCEED,
        HAS_FAILED_TASKS,
        HAS_UNSTARTED_TASKS
    };
    EXECUTE_STATUS execute();

   private:
    std::unordered_map<std::string_view, std::unique_ptr<TaskBase>> task_map_;
    std::unordered_map<std::string_view, std::vector<std::string_view>>
        subsequent_tasks_map_;
};

}  // namespace nexusfold