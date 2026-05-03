#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include "lsm/memtable/concurrent_skip_list.h"

TEST(ConcurrentSkipList, PutAndGet) {
    lsm::ConcurrentSkipList sl;
    sl.put("a", "1");
    sl.put("b", "2");
    sl.put("c", "3");

    EXPECT_EQ(sl.get("a"), "1");
    EXPECT_EQ(sl.get("b"), "2");
    EXPECT_EQ(sl.get("c"), "3");
}

TEST(ConcurrentSkipList, GetMissingKey) {
    lsm::ConcurrentSkipList sl;
    sl.put("a", "1");
    EXPECT_EQ(sl.get("missing"), std::nullopt);
}

TEST(ConcurrentSkipList, UpdateExistingKey) {
    lsm::ConcurrentSkipList sl;
    sl.put("key", "old");
    sl.put("key", "new");
    EXPECT_EQ(sl.get("key"), "new");
}

TEST(ConcurrentSkipList, Contains) {
    lsm::ConcurrentSkipList sl;
    sl.put("a", "1");
    EXPECT_TRUE(sl.contains("a"));
    EXPECT_FALSE(sl.contains("b"));
}

TEST(ConcurrentSkipList, Size) {
    lsm::ConcurrentSkipList sl;
    EXPECT_EQ(sl.size(), 0u);
    sl.put("a", "1");
    sl.put("b", "2");
    EXPECT_EQ(sl.size(), 2u);
}

TEST(ConcurrentSkipList, IteratorOrder) {
    lsm::ConcurrentSkipList sl;
    sl.put("c", "3");
    sl.put("a", "1");
    sl.put("b", "2");

    auto it = sl.begin();
    ASSERT_TRUE(it.valid());
    EXPECT_EQ(it.key(), "a");
    it.next();
    EXPECT_EQ(it.key(), "b");
    it.next();
    EXPECT_EQ(it.key(), "c");
    it.next();
    EXPECT_FALSE(it.valid());
}

TEST(ConcurrentSkipList, SeekExact) {
    lsm::ConcurrentSkipList sl;
    sl.put("a", "1");
    sl.put("b", "2");
    sl.put("c", "3");

    auto it = sl.seek("b");
    ASSERT_TRUE(it.valid());
    EXPECT_EQ(it.key(), "b");
}

TEST(ConcurrentSkipList, SeekPastEnd) {
    lsm::ConcurrentSkipList sl;
    sl.put("a", "1");

    auto it = sl.seek("z");
    EXPECT_FALSE(it.valid());
}

TEST(ConcurrentSkipList, ConcurrentReads) {
    lsm::ConcurrentSkipList sl;
    for (int i = 0; i < 100; i++) {
        sl.put("key_" + std::to_string(i), std::to_string(i));
    }

    std::vector<std::thread> threads;
    std::atomic<int> failures{0};

    for (int t = 0; t < 8; t++) {
        threads.emplace_back([&sl, &failures] {
            for (int i = 0; i < 100; i++) {
                auto r = sl.get("key_" + std::to_string(i));
                if (!r || *r != std::to_string(i)) {
                    failures.fetch_add(1);
                }
            }
        });
    }

    for (auto& t : threads) t.join();
    EXPECT_EQ(failures.load(), 0);
}
