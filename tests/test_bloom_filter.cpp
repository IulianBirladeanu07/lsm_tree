#include <gtest/gtest.h>
#include "lsm/util/bloom_filter.h"

TEST(BloomFilter, ContainsAddedKeys) {
    lsm::BloomFilter bf(100, 0.01);
    bf.add("foo");
    bf.add("bar");
    bf.add("baz");

    EXPECT_TRUE(bf.may_contain("foo"));
    EXPECT_TRUE(bf.may_contain("bar"));
    EXPECT_TRUE(bf.may_contain("baz"));
}

TEST(BloomFilter, DoesNotContainMissingKeys) {
    lsm::BloomFilter bf(100, 0.01);
    bf.add("foo");

    EXPECT_FALSE(bf.may_contain("xyz"));
    EXPECT_FALSE(bf.may_contain("qwe"));
    EXPECT_FALSE(bf.may_contain("abc"));
}

TEST(BloomFilter, SerializeDeserialize) {
    lsm::BloomFilter bf(100, 0.01);
    bf.add("foo");
    bf.add("bar");

    auto bytes = bf.serialize();
    auto bf2 = lsm::BloomFilter::deserialize(bytes);

    EXPECT_TRUE(bf2.may_contain("foo"));
    EXPECT_TRUE(bf2.may_contain("bar"));
    EXPECT_FALSE(bf2.may_contain("xyz"));
}

TEST(BloomFilter, EmptyFilterReturnsFalse) {
    lsm::BloomFilter bf(100, 0.01);
    EXPECT_FALSE(bf.may_contain("anything"));
}

TEST(BloomFilter, LargeKeySet) {
    lsm::BloomFilter bf(1000, 0.01);
    for (int i = 0; i < 1000; i++) {
        bf.add("key_" + std::to_string(i));
    }
    for (int i = 0; i < 1000; i++) {
        EXPECT_TRUE(bf.may_contain("key_" + std::to_string(i)));
    }
}
