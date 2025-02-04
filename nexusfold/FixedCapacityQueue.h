#pragma once

#include <atomic>
#include <vector>

template <class T>
class FixedCapacityQueue {
   public:
    FixedCapacityQueue(size_t capacity)
        : queue_data_(capacity),
          capacity_(capacity),
          push_index_(0),
          pop_index_(0) {}

    bool push(const T& val) {
        size_t index = push_index_.load();
        while (!push_index_.compare_exchange_weak(index, index + 1)) {
        }
        if (index >= capacity_) {
            // queue already full.
            return false;
        }
        queue_data_[index] = val;
        return true;
    }

    bool pop(T& output_val) {
        size_t index = pop_index_.load();
        do {
            if (index >= push_index_.load()) {
                // queue is empty right now.
                return false;
            }
        } while (!pop_index_.compare_exchange_weak(index, index + 1));
        output_val = queue_data_[index];
        return true;
    }

    bool empty() { return pop_index_.load() >= push_index_.load(); }

   private:
    std::vector<T> queue_data_;
    size_t capacity_;
    std::atomic<size_t> push_index_;
    std::atomic<size_t> pop_index_;
};