#include "Nexusfold.h"

#include <algorithm>
#include <atomic>
#include <queue>
#include <stdexcept>
#include <thread>

#if PARALLEL
#include <execution>
#define PAR_UNSEQ std::execution::par_unseq,
#else
#define PAR_UNSEQ
#endif

#include "FixedCapacityQueue.h"

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

TaskScheduler::ExecutionStatus TaskScheduler::execute(
    ExecutionPolicy execution_policy) {
    switch (execution_policy) {
        case seq:
            return execute_seq_();
        case par:
            return execute_par_();
        default:
            return execute_policy_not_implemented;
    }
}

TaskScheduler::ExecutionStatus TaskScheduler::execute_seq_() {
    std::unordered_map<std::string_view, int> prior_task_counts;
    for (auto& [task_id, _] : task_map_) {
        prior_task_counts[task_id] = 0;
    }
    for (auto& [_, subsequent_tasks] : subsequent_tasks_map_) {
        for (auto& task_id : subsequent_tasks) {
            prior_task_counts[task_id]++;
        }
    }

    std::queue<std::string_view> ready_tasks;
    for (auto& [task_id, count] : prior_task_counts) {
        if (count == 0) {
            ready_tasks.push(task_id);
        }
    }
    int processed_task_count = 0;

    while (!ready_tasks.empty()) {
        std::string_view current_task_id = ready_tasks.front();
        for (auto subsequent_task_id : subsequent_tasks_map_[current_task_id]) {
            if (--prior_task_counts[subsequent_task_id] == 0) {
                ready_tasks.push(subsequent_task_id);
            }
        }
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
}

TaskScheduler::ExecutionStatus TaskScheduler::execute_par_() {
    std::unordered_map<std::string_view, std::atomic<int>> prior_task_counts;
    std::for_each(PAR_UNSEQ task_map_.begin(), task_map_.end(),
                  [&prior_task_counts](const auto& id_task_pair) {
                      prior_task_counts[id_task_pair.first].store(0);
                  });
    std::for_each(
        PAR_UNSEQ subsequent_tasks_map_.begin(), subsequent_tasks_map_.end(),
        [&prior_task_counts](const auto& id_subseqs_pair) {
            std::for_each(PAR_UNSEQ id_subseqs_pair.second.begin(),
                          id_subseqs_pair.second.end(),
                          [&prior_task_counts](const auto& subseq_task_id) {
                              prior_task_counts[subseq_task_id]++;
                          });
        });

    std::atomic<bool> has_scheduler_error{false};
    FixedCapacityQueue<std::string_view> ready_tasks(task_map_.size());
    std::for_each(
        PAR_UNSEQ prior_task_counts.begin(), prior_task_counts.end(),
        [&has_scheduler_error, &ready_tasks](const auto& id_count_pair) {
            if (id_count_pair.second == 0) {
                if (!ready_tasks.push(id_count_pair.first)) {
                    // queue is full, should never happen.
                    has_scheduler_error.store(true);
                }
            }
        });
    std::atomic<int> checking_tasks{0};
    std::atomic<bool> has_failed_task{false};

    // todo: use a thread pool instead.
    std::vector<std::thread> task_threads;
    do {
        std::string_view current_task_id;
        if (ready_tasks.pop(current_task_id)) {
            checking_tasks++;
            task_threads.emplace_back([this, current_task_id, &checking_tasks,
                                       &has_failed_task, &has_scheduler_error,
                                       &prior_task_counts, &ready_tasks]() {
                for (auto subsequent_task_id :
                     this->subsequent_tasks_map_[current_task_id]) {
                    if (--prior_task_counts[subsequent_task_id] == 0) {
                        if (!ready_tasks.push(subsequent_task_id)) {
                            // queue is full, should never happen.
                            has_scheduler_error.store(true);
                        }
                    }
                }
                checking_tasks--;
                try {
                    task_map_[current_task_id]->execute();
                } catch (std::exception const& ex) {
                    has_failed_task.store(true);
                }
            });
        }
    } while ((checking_tasks.load() > 0 || !ready_tasks.empty()) &&
             !has_failed_task.load() && !has_scheduler_error.load());

    for (auto& task_thread : task_threads) {
        task_thread.join();
    }

    if (has_scheduler_error.load()) {
        return TaskScheduler::ExecutionStatus::has_scheduler_error;
    } else if (has_failed_task.load()) {
        return TaskScheduler::ExecutionStatus::has_failed_task;
    } else {
        return task_threads.size() == task_map_.size() ? all_secceed
                                                       : has_unstarted_task;
    }
}

}  // namespace nexusfold