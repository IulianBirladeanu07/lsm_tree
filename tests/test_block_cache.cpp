#include <gtest/gtest.h>
#include "lsm/sstable/block_cache.h"

static lsm::Block make_block(uint8_t fill, std::size_t size) {
    return lsm::Block(std::vector<uint8_t>(size, fill));
}

TEST(BlockCache, PutAndGet) {
    lsm::BlockCache cache(1024 * 1024);
    cache.put(1, 0, make_block(0xAA, 512));

    auto r = cache.get(1, 0);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->size(), 512u);
}

TEST(BlockCache, GetMissingKey) {
    lsm::BlockCache cache(1024);
    EXPECT_EQ(cache.get(99, 99), std::nullopt);
}

TEST(BlockCache, UpdateExistingKey) {
    lsm::BlockCache cache(1024 * 1024);
    cache.put(1, 0, make_block(0xAA, 256));
    cache.put(1, 0, make_block(0xBB, 512));

    auto r = cache.get(1, 0);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->size(), 512u);
}

TEST(BlockCache, EvictsWhenFull) {
    lsm::BlockCache cache(512);
    cache.put(1, 0, make_block(0xAA, 512));
    cache.put(2, 0, make_block(0xBB, 512));

    EXPECT_EQ(cache.get(1, 0), std::nullopt);
    ASSERT_TRUE(cache.get(2, 0).has_value());
}

TEST(BlockCache, MultipleEntries) {
    lsm::BlockCache cache(1024 * 1024);
    cache.put(1, 0,   make_block(0xAA, 128));
    cache.put(1, 128, make_block(0xBB, 128));
    cache.put(2, 0,   make_block(0xCC, 128));

    EXPECT_TRUE(cache.get(1, 0).has_value());
    EXPECT_TRUE(cache.get(1, 128).has_value());
    EXPECT_TRUE(cache.get(2, 0).has_value());
    EXPECT_EQ(cache.get(3, 0), std::nullopt);
}

TEST(BlockCache, CapacityAndUsage) {
    lsm::BlockCache cache(1024);
    EXPECT_EQ(cache.capacity(), 1024u);
    EXPECT_EQ(cache.usage(), 0u);

    cache.put(1, 0, make_block(0xAA, 256));
    EXPECT_EQ(cache.usage(), 256u);
}
