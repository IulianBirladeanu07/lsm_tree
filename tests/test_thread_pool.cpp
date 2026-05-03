#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>
#include "lsm/compaction/thread_pool.h"

TEST(ThreadPool, ExecutesTasks) {
    lsm::ThreadPool pool(4);
    std::atomic<int> counter{0};

    for (int i = 0; i < 100; i++) {
        pool.submit([&counter] {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (counter.load() < 100 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    pool.shutdown();
    EXPECT_EQ(counter.load(), 100);
}

TEST(ThreadPool, SingleThread) {
    lsm::ThreadPool pool(1);
    std::atomic<int> counter{0};

    for (int i = 0; i < 10; i++) {
        pool.submit([&counter] {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (counter.load() < 10 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    pool.shutdown();
    EXPECT_EQ(counter.load(), 10);
}

TEST(ThreadPool, ShutdownOnStoppedPoolThrows) {
    lsm::ThreadPool pool(2);
    pool.shutdown();
    EXPECT_THROW(pool.submit([] {}), std::runtime_error);
}

TEST(ThreadPool, ConcurrentSubmit) {
    lsm::ThreadPool pool(4);
    std::atomic<int> counter{0};
    std::vector<std::thread> submitters;

    for (int t = 0; t < 4; t++) {
        submitters.emplace_back([&pool, &counter] {
            for (int i = 0; i < 25; i++) {
                pool.submit([&counter] {
                    counter.fetch_add(1, std::memory_order_relaxed);
                });
            }
        });
    }

    for (auto& t : submitters) t.join();

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (counter.load() < 100 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    pool.shutdown();
    EXPECT_EQ(counter.load(), 100);
}
