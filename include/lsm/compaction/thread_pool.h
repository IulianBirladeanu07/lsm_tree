#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <functional>
#include <condition_variable>

namespace lsm {

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();

    void submit(std::function<void()> task);
    void shutdown();

private:
    std::vector<std::jthread> workers_;
    std::deque<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};
};
} // namespace lsm