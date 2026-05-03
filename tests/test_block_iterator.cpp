#include <gtest/gtest.h>
#include "lsm/iterator/block_iterator.h"
#include "lsm/sstable/block_builder.h"
#include "lsm/sstable/block.h"

static lsm::Block make_block(std::vector<std::pair<std::string, std::string>> entries) {
    lsm::BlockBuilder builder(4096);
    for (auto& [k, v] : entries) builder.add(k, v);
    return lsm::Block(builder.finish());
}

TEST(BlockIterator, IteratesInOrder) {
    auto block = make_block({{"a", "1"}, {"b", "2"}, {"c", "3"}, {"d", "4"}});
    lsm::BlockIterator it(block);

    ASSERT_TRUE(it.valid()); EXPECT_EQ(it.key(), "a"); EXPECT_EQ(it.value(), "1");
    it.next();
    ASSERT_TRUE(it.valid()); EXPECT_EQ(it.key(), "b"); EXPECT_EQ(it.value(), "2");
    it.next();
    ASSERT_TRUE(it.valid()); EXPECT_EQ(it.key(), "c");
    it.next();
    ASSERT_TRUE(it.valid()); EXPECT_EQ(it.key(), "d");
    it.next();
    EXPECT_FALSE(it.valid());
}

TEST(BlockIterator, SeekExact) {
    auto block = make_block({{"a", "1"}, {"b", "2"}, {"c", "3"}});
    lsm::BlockIterator it(block);

    it.seek("b");
    ASSERT_TRUE(it.valid());
    EXPECT_EQ(it.key(), "b");
    EXPECT_EQ(it.value(), "2");
}

TEST(BlockIterator, SeekToFirst) {
    auto block = make_block({{"a", "1"}, {"b", "2"}, {"c", "3"}});
    lsm::BlockIterator it(block);

    it.seek("a");
    ASSERT_TRUE(it.valid());
    EXPECT_EQ(it.key(), "a");
}

TEST(BlockIterator, SeekPastEnd) {
    auto block = make_block({{"a", "1"}, {"b", "2"}});
    lsm::BlockIterator it(block);

    it.seek("z");
    EXPECT_FALSE(it.valid());
}

TEST(BlockIterator, SeekBetweenKeys) {
    auto block = make_block({{"a", "1"}, {"c", "3"}});
    lsm::BlockIterator it(block);

    it.seek("b");
    ASSERT_TRUE(it.valid());
    EXPECT_EQ(it.key(), "c");
}

TEST(BlockIterator, SingleEntry) {
    auto block = make_block({{"only", "val"}});
    lsm::BlockIterator it(block);

    ASSERT_TRUE(it.valid());
    EXPECT_EQ(it.key(), "only");
    it.next();
    EXPECT_FALSE(it.valid());
}
