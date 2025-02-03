#include "Nexusfold.h"

#include <algorithm>
#include <atomic>
#include <execution>
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

TaskScheduler::ExecuteStatus TaskScheduler::execute(
    ExecutePolicy execute_policy) {
    std::unordered_map<std::string_view, std::atomic<int>> prior_task_counts;
    std::for_each(std::execution::par_unseq, task_map_.begin(), task_map_.end(),
                  [&prior_task_counts](const auto& id_task_pair) {
                      prior_task_counts[id_task_pair.first] = 0;
                  });
    std::for_each(std::execution::par_unseq, subsequent_tasks_map_.begin(),
                  subsequent_tasks_map_.end(),
                  [&prior_task_counts](const auto& id_subseqs_pair) {
                      std::for_each(
                          std::execution::par_unseq,
                          id_subseqs_pair.second.begin(),
                          id_subseqs_pair.second.end(),
                          [&prior_task_counts](const auto& subseq_task_id) {
                              prior_task_counts[subseq_task_id]++;
                          });
                  });

    if (execute_policy == seq) {
        std::queue<std::string_view> ready_tasks;
        int processed_task_count = 0;

        for (auto& [task_id, count] : prior_task_counts) {
            if (count == 0) {
                ready_tasks.push(task_id);
            }
        }

        while (!ready_tasks.empty()) {
            std::string_view current_task_id = ready_tasks.front();
            for (auto subsequent_task_id :
                 subsequent_tasks_map_[current_task_id]) {
                if (--prior_task_counts[subsequent_task_id] == 0) {
                    ready_tasks.push(subsequent_task_id);
                }
            }
            // todo: catch the exception.
            try {
                task_map_[current_task_id]->execute();
            } catch (std::exception const& ex) {
                return has_failed_task;
            }
            ready_tasks.pop();
            processed_task_count++;
        }

        return processed_task_count == task_map_.size() ? all_secceed
                                                        : has_unstarted_task;
    } else {
        return execute_policy_not_implemented;
    }
}

}  // namespace nexusfold