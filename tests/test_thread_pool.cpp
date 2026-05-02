#include "lsm/compaction/thread_pool.h"
#include <iostream>
#include <atomic>
#include <chrono>
#include <thread>

using namespace lsm;

int main() {
    ThreadPool pool(4);
    std::atomic<int> counter{0};

    for (int i = 0; i < 100; ++i) {
        pool.submit([&counter] {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    // Wait for all tasks to complete
    while (counter.load(std::memory_order_acquire) < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    pool.shutdown();
    
    std::cout << "All tasks completed: " << counter.load() << "\n";
    return 0;
}
