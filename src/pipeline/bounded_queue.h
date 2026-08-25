#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <stop_token>
#include <chrono>

namespace sk265::pipeline {

template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(size_t capacity)
        : capacity_(capacity > 0 ? capacity : 1), closed_(false) {}

    ~BoundedQueue() {
        close();
    }

    // Disable copy, enable move
    BoundedQueue(const BoundedQueue&) = delete;
    BoundedQueue& operator=(const BoundedQueue&) = delete;

    bool push(T item, std::stop_token st = {}) {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!closed_ && !st.stop_requested() && queue_.size() >= capacity_) {
            cv_not_full_.wait_for(lock, std::chrono::milliseconds(20));
        }
        if (closed_ || st.stop_requested()) {
            return false;
        }

        queue_.push(std::move(item));
        cv_not_empty_.notify_one();
        return true;
    }

    std::optional<T> pop(std::stop_token st = {}) {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!closed_ && !st.stop_requested() && queue_.empty()) {
            cv_not_empty_.wait_for(lock, std::chrono::milliseconds(20));
        }
        if (queue_.empty()) {
            return std::nullopt; // Closed or stop requested and drained
        }

        T item = std::move(queue_.front());
        queue_.pop();
        cv_not_full_.notify_one();
        return item;
    }

    void close() {
        std::unique_lock<std::mutex> lock(mutex_);
        closed_ = true;
        cv_not_empty_.notify_all();
        cv_not_full_.notify_all();
    }

    size_t size() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.size();
    }

    bool empty() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    size_t capacity() const noexcept {
        return capacity_;
    }

private:
    const size_t capacity_;
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_not_empty_;
    std::condition_variable cv_not_full_;
    bool closed_{false};
};

} // namespace sk265::pipeline
