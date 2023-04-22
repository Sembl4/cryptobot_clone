#pragma once

#include <atomic>
#include <mutex>
#include <thread>

namespace utils {

class [[nodiscard]] SpinWait {
  public:
    void SpinOnce() { std::this_thread::yield(); }

    void operator()() { SpinOnce(); }
};

class SpinLock {
    using ScopeGuard = std::lock_guard<SpinLock>;

  public:
    void Lock() {
        SpinWait spin_wait;
        while (locked_.exchange(true, std::memory_order_acquire)) {
            while (locked_.load(std::memory_order_relaxed)) {
                spin_wait();
            }
        }
    }

    bool TryLock() {
        return !locked_.exchange(true, std::memory_order_acquire);
    }

    void Unlock() { locked_.store(false, std::memory_order_release); }

    ScopeGuard Guard() { return ScopeGuard{*this}; }

    // Lockable concept
    void lock() { Lock(); }

    bool try_lock() { return TryLock(); }

    void unlock() { Unlock(); }

  private:
    std::atomic<bool> locked_{false};
};

}  // namespace utils