#include <gtest/gtest.h>
#include "lsm/sstable/block_builder.h"
#include "lsm/sstable/block.h"

TEST(BlockBuilder, AddAndFinish) {
    lsm::BlockBuilder builder(4096);
    builder.add("apple", "fruit");
    builder.add("banana", "yellow");
    builder.add("cherry", "red");

    EXPECT_GT(builder.current_size(), 0u);
    EXPECT_FALSE(builder.is_full());

    auto data = builder.finish();
    EXPECT_GT(data.size(), 0u);
}

TEST(BlockBuilder, IsFullWhenExceedsBlockSize) {
    lsm::BlockBuilder builder(32);
    builder.add("key_that_is_long_enough", "value_that_is_long_enough");
    EXPECT_TRUE(builder.is_full());
}

TEST(BlockBuilder, Reset) {
    lsm::BlockBuilder builder(4096);
    builder.add("key", "value");
    EXPECT_GT(builder.current_size(), 0u);

    builder.reset();
    EXPECT_EQ(builder.current_size(), 0u);
}

TEST(BlockBuilder, FinishedBlockIsReadable) {
    lsm::BlockBuilder builder(4096);
    builder.add("alpha", "1");
    builder.add("beta", "2");

    auto data = builder.finish();
    lsm::Block block(std::move(data));

    EXPECT_EQ(block.get("alpha"), "1");
    EXPECT_EQ(block.get("beta"), "2");
}
