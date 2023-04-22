#pragma once

#include <spinlock.hpp>

#include <condition_variable>
#include <deque>
#include <optional>

// Unbounded blocking multi-producers/multi-consumers queue

template <typename T>
class BlockingQueue {
  public:
    BlockingQueue() {}

    bool Put(T value) {
        std::lock_guard lock(spinlock_);
        if (closed_) {
            return false;
        }
        queue_.emplace_back(std::move(value));
        any_value_.notify_one();
        return true;
    }

    std::optional<T> Take() {
        std::unique_lock lock(spinlock_);
        while (queue_.empty() && !closed_) {
            any_value_.wait(lock);
        }

        return std::move(TakeFromQueue());
    }

    bool empty() {
        std::unique_lock lock(spinlock_);
        return queue_.empty();
    }

    void Clear() {
        std::unique_lock lock(spinlock_);
        queue_.clear();
    }

    void Close() { CloseImpl(true); }

  private:
    void CloseImpl(bool clear) {
        std::lock_guard lock(spinlock_);
        if (clear) {
            queue_.clear();
        }
        closed_ = true;
        any_value_.notify_all();
    }

    // Under lock
    std::optional<T> TakeFromQueue() {
        if (queue_.empty()) {
            return std::nullopt;
        }

        T value = std::move(queue_.front());
        queue_.pop_front();
        return std::move(value);
    }

  private:
    std::deque<T> queue_;
    utils::SpinLock spinlock_;
    std::condition_variable_any any_value_;
    bool closed_{false};
};
