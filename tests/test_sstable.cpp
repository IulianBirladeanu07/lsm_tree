#include <gtest/gtest.h>
#include <filesystem>
#include "lsm/sstable/sstable_builder.h"
#include "lsm/sstable/sstable.h"

class SSTableTest : public ::testing::Test {
protected:
    std::filesystem::path path = "/tmp/test_sstable_gtest.sst";

    void TearDown() override {
        std::filesystem::remove(path);
    }

    lsm::SSTable build(std::vector<std::pair<std::string, std::string>> entries) {
        lsm::SSTableBuilder builder(path, 4096, entries.size() + 1, 0.01);
        for (auto& [k, v] : entries) {
            builder.add(k, v);
        }
        return builder.finish();
    }
};

TEST_F(SSTableTest, GetExistingKeys) {
    auto sst = build({{"alpha", "1"}, {"beta", "2"}, {"gamma", "3"}});

    EXPECT_EQ(sst.get("alpha"), "1");
    EXPECT_EQ(sst.get("beta"), "2");
    EXPECT_EQ(sst.get("gamma"), "3");
}

TEST_F(SSTableTest, GetMissingKey) {
    auto sst = build({{"alpha", "1"}, {"beta", "2"}});
    EXPECT_EQ(sst.get("missing"), std::nullopt);
}

TEST_F(SSTableTest, SmallestAndLargestKey) {
    auto sst = build({{"alpha", "1"}, {"beta", "2"}, {"gamma", "3"}});
    EXPECT_EQ(sst.smallest_key(), "alpha");
    EXPECT_EQ(sst.largest_key(), "gamma");
}

TEST_F(SSTableTest, FileSizeIsPositive) {
    auto sst = build({{"key", "val"}});
    EXPECT_GT(sst.file_size(), 0u);
}

TEST_F(SSTableTest, ReopenAndGet) {
    {
        lsm::SSTableBuilder builder(path, 4096, 10, 0.01);
        builder.add("alpha", "1");
        builder.add("beta", "2");
        builder.finish();
    }

    auto sst = lsm::SSTable::open(path);
    EXPECT_EQ(sst.get("alpha"), "1");
    EXPECT_EQ(sst.get("beta"), "2");
    EXPECT_EQ(sst.get("zzz"), std::nullopt);
}

TEST_F(SSTableTest, MultipleBlocks) {
    std::vector<std::pair<std::string, std::string>> entries;
    for (int i = 0; i < 500; i++) {
        entries.push_back({"key_" + std::to_string(1000 + i), "val_" + std::to_string(i)});
    }
    auto sst = build(entries);

    EXPECT_EQ(sst.get("key_1000"), "val_0");
    EXPECT_EQ(sst.get("key_1499"), "val_499");
    EXPECT_EQ(sst.get("key_9999"), std::nullopt);
}
