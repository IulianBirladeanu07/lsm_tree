#include "lsm/compaction/thread_pool.h"

namespace lsm {

ThreadPool::ThreadPool(size_t num_tasks) {
    for(size_t i = 0; i < num_tasks; ++i) {
        workers_.emplace_back([this] {
            while(true) {
                std::function<void()> task;
                {
                    std::unique_lock lock(queue_mutex_);
                    condition_.wait(lock, [this] {
                        return stop_.load(std::memory_order_acquire) || !tasks_.empty();
                    });
                    if(stop_.load(std::memory_order_acquire) && tasks_.empty()) {
                        return;
                    }
                    task = std::move(tasks_.front());
                    tasks_.pop_front();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::submit(std::function<void()> task) {
    {
        std::lock_guard lock(queue_mutex_);
        if(stop_.load(std::memory_order_acquire)) {
            throw std::runtime_error("ThreadPool: submit on stopped pool");
        }
        tasks_.push_back(std::move(task));
        condition_.notify_one();
    }
}

void ThreadPool::shutdown() {
    {
        std::lock_guard lock(queue_mutex_);
        stop_.store(true, std::memory_order_release);
    }
    condition_.notify_all();
}

} // namespace lsm