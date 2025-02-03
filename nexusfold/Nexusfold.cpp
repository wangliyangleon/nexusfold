#include "Nexusfold.h"

#include <queue>
#include <stdexcept>

namespace nexusfold {

TaskScheduler& TaskScheduler::add_task(std::unique_ptr<TaskBase> new_task,
                                       std::string_view task_id) {
    if (!task_map_.insert({task_id, std::move(new_task)}).second) {
        // task id already exists.
        throw std::invalid_argument("Task [" + std::string(task_id) +
                                    "] already exists");
    }
    return *this;
}

TaskScheduler& TaskScheduler::apply_dependency(
    std::string_view prior_task_id, std::string_view subsequent_task_id) {
    if (task_map_.find(prior_task_id) == task_map_.end() ||
        task_map_.find(subsequent_task_id) == task_map_.end()) {
        throw std::invalid_argument("Prior or subsequent not exists");
    }
    subsequent_tasks_map_[prior_task_id].push_back(subsequent_task_id);
    return *this;
}

TaskScheduler::EXECUTE_STATUS TaskScheduler::execute() {
    std::unordered_map<std::string_view, int> prior_task_counts;
    for (auto& [task_id, task] : task_map_) {
        prior_task_counts[task_id] = 0;
    }
    for (auto& [_, subsequent_task_ids] : subsequent_tasks_map_) {
        for (auto& task_id : subsequent_task_ids) {
            prior_task_counts[task_id]++;
        };
    }

    std::queue<std::string_view> ready_tasks;
    int processed_task_count = 0;
    for (auto& [task_id, count] : prior_task_counts) {
        if (count == 0) {
            ready_tasks.push(task_id);
        }
    }

    while (!ready_tasks.empty()) {
        std::string_view current_task_id = ready_tasks.front();
        for (auto subsequent_task_id : subsequent_tasks_map_[current_task_id]) {
            if (--prior_task_counts[subsequent_task_id] == 0) {
                ready_tasks.push(subsequent_task_id);
            }
        }
        // todo: catch the exception.
        task_map_[current_task_id]->execute();
        ready_tasks.pop();
        processed_task_count++;
    }

    return processed_task_count == task_map_.size() ? ALL_SUCCEED
                                                    : HAS_UNSTARTED_TASKS;
}

}  // namespace nexusfold