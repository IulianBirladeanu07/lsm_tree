#include <gtest/gtest.h>
#include "lsm/sstable/block_builder.h"
#include "lsm/sstable/block.h"

static lsm::Block make_block(std::vector<std::pair<std::string, std::string>> entries) {
    lsm::BlockBuilder builder(4096);
    for (auto& [k, v] : entries) {
        builder.add(k, v);
    }
    return lsm::Block(builder.finish());
}

TEST(Block, GetExistingKey) {
    auto block = make_block({{"apple", "fruit"}, {"banana", "yellow"}, {"cherry", "red"}});

    EXPECT_EQ(block.get("apple"), "fruit");
    EXPECT_EQ(block.get("banana"), "yellow");
    EXPECT_EQ(block.get("cherry"), "red");
}

TEST(Block, GetMissingKey) {
    auto block = make_block({{"apple", "fruit"}});
    EXPECT_EQ(block.get("missing"), std::nullopt);
}

TEST(Block, GetFromEmptyBlock) {
    lsm::BlockBuilder builder(4096);
    auto data = builder.finish();
    lsm::Block block(std::move(data));
    EXPECT_EQ(block.get("key"), std::nullopt);
}

TEST(Block, SizeIsPositive) {
    auto block = make_block({{"k", "v"}});
    EXPECT_GT(block.size(), 0u);
}

TEST(Block, GetWithEmptyValue) {
    auto block = make_block({{"key", ""}});
    auto r = block.get("key");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, "");
}
