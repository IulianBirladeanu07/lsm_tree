#include <gtest/gtest.h>
#include <filesystem>
#include <thread>
#include <vector>
#include <atomic>
#include "lsm/lsm_tree.h"

class LSMTreeTest : public ::testing::Test {
protected:
    std::filesystem::path dir = "/tmp/test_lsm_gtest";

    void SetUp() override {
        std::filesystem::remove_all(dir);
    }

    void TearDown() override {
        std::filesystem::remove_all(dir);
    }
};

TEST_F(LSMTreeTest, PutAndGet) {
    lsm::LSMTree tree(dir);
    tree.put("a", "1");
    tree.put("b", "2");
    tree.put("c", "3");

    EXPECT_EQ(tree.get("a"), "1");
    EXPECT_EQ(tree.get("b"), "2");
    EXPECT_EQ(tree.get("c"), "3");
}

TEST_F(LSMTreeTest, GetMissingKey) {
    lsm::LSMTree tree(dir);
    EXPECT_EQ(tree.get("missing"), std::nullopt);
}

TEST_F(LSMTreeTest, UpdateKey) {
    lsm::LSMTree tree(dir);
    tree.put("key", "old");
    tree.put("key", "new");
    EXPECT_EQ(tree.get("key"), "new");
}

TEST_F(LSMTreeTest, DeleteKey) {
    lsm::LSMTree tree(dir);
    tree.put("key", "val");
    tree.del("key");
    EXPECT_EQ(tree.get("key"), std::nullopt);
}

TEST_F(LSMTreeTest, DeleteNonExistentKey) {
    lsm::LSMTree tree(dir);
    EXPECT_NO_THROW(tree.del("missing"));
    EXPECT_EQ(tree.get("missing"), std::nullopt);
}

TEST_F(LSMTreeTest, Scan) {
    lsm::LSMTree tree(dir);
    tree.put("a", "1");
    tree.put("b", "2");
    tree.put("c", "3");
    tree.put("d", "4");

    auto results = tree.scan("b", "c");
    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].key, "b");
    EXPECT_EQ(results[1].key, "c");
}

TEST_F(LSMTreeTest, ScanExcludesDeleted) {
    lsm::LSMTree tree(dir);
    tree.put("a", "1");
    tree.put("b", "2");
    tree.put("c", "3");
    tree.del("b");

    auto results = tree.scan("a", "c");
    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].key, "a");
    EXPECT_EQ(results[1].key, "c");
}

TEST_F(LSMTreeTest, TieredCompactionStyle) {
    lsm::LSMOptions opts;
    opts.compaction_style = lsm::LSMOptions::CompactionStyle::Tiered;
    lsm::LSMTree tree(dir, opts);

    tree.put("a", "1");
    tree.put("b", "2");
    EXPECT_EQ(tree.get("a"), "1");
}

TEST_F(LSMTreeTest, MemtableFlushOnFull) {
    lsm::LSMOptions opts;
    opts.memtable_size = 512;
    lsm::LSMTree tree(dir, opts);

    for (int i = 0; i < 50; i++) {
        tree.put("key_" + std::to_string(i), "value_" + std::to_string(i));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    for (int i = 0; i < 50; i++) {
        EXPECT_EQ(tree.get("key_" + std::to_string(i)), "value_" + std::to_string(i));
    }
}

TEST_F(LSMTreeTest, ConcurrentPutAndGet) {
    lsm::LSMTree tree(dir);
    std::atomic<int> failures{0};

    std::vector<std::thread> writers;
    for (int t = 0; t < 4; t++) {
        writers.emplace_back([&tree, t] {
            for (int i = 0; i < 25; i++) {
                tree.put("key_" + std::to_string(t * 25 + i), std::to_string(i));
            }
        });
    }
    for (auto& w : writers) w.join();

    for (int i = 0; i < 100; i++) {
        auto r = tree.get("key_" + std::to_string(i));
        if (!r) failures.fetch_add(1);
    }

    EXPECT_EQ(failures.load(), 0);
}
