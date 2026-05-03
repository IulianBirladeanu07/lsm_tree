#include <gtest/gtest.h>
#include <filesystem>
#include "lsm/iterator/sstable_iterator.h"
#include "lsm/sstable/sstable_builder.h"
#include "lsm/sstable/sstable.h"

class SSTableIteratorTest : public ::testing::Test {
protected:
    std::filesystem::path path = "/tmp/test_sst_iter_gtest.sst";

    void TearDown() override {
        std::filesystem::remove(path);
    }

    lsm::SSTable build(std::vector<std::pair<std::string, std::string>> entries) {
        lsm::SSTableBuilder builder(path, 4096, entries.size() + 1, 0.01);
        for (auto& [k, v] : entries) builder.add(k, v);
        return builder.finish();
    }
};

TEST_F(SSTableIteratorTest, IteratesAllKeys) {
    auto sst = build({{"a", "1"}, {"b", "2"}, {"c", "3"}, {"d", "4"}, {"e", "5"}});
    lsm::SSTableIterator it(sst);

    std::vector<std::string> keys;
    while (it.valid()) {
        keys.push_back(std::string(it.key()));
        it.next();
    }

    EXPECT_EQ(keys, (std::vector<std::string>{"a", "b", "c", "d", "e"}));
}

TEST_F(SSTableIteratorTest, SeekExact) {
    auto sst = build({{"a", "1"}, {"b", "2"}, {"c", "3"}});
    lsm::SSTableIterator it(sst);

    it.seek("b");
    ASSERT_TRUE(it.valid());
    EXPECT_EQ(it.key(), "b");
    EXPECT_EQ(it.value(), "2");
}

TEST_F(SSTableIteratorTest, SeekPastEnd) {
    auto sst = build({{"a", "1"}, {"b", "2"}});
    lsm::SSTableIterator it(sst);

    it.seek("z");
    EXPECT_FALSE(it.valid());
}

TEST_F(SSTableIteratorTest, SeekBetweenKeys) {
    auto sst = build({{"a", "1"}, {"c", "3"}, {"e", "5"}});
    lsm::SSTableIterator it(sst);

    it.seek("b");
    ASSERT_TRUE(it.valid());
    EXPECT_EQ(it.key(), "c");
}

TEST_F(SSTableIteratorTest, IteratesAcrossMultipleBlocks) {
    std::vector<std::pair<std::string, std::string>> entries;
    for (int i = 0; i < 500; i++) {
        entries.push_back({"key_" + std::to_string(1000 + i), "val_" + std::to_string(i)});
    }
    auto sst = build(entries);
    lsm::SSTableIterator it(sst);

    int count = 0;
    while (it.valid()) {
        count++;
        it.next();
    }
    EXPECT_EQ(count, 500);
}
