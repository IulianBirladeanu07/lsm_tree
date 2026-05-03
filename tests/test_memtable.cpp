#include <gtest/gtest.h>
#include "lsm/memtable/memtable.h"

TEST(MemTable, PutAndGet) {
    lsm::MemTable mem(1024 * 1024);
    mem.put("alpha", "1");
    mem.put("beta", "2");
    mem.put("gamma", "3");

    EXPECT_EQ(mem.get("alpha"), "1");
    EXPECT_EQ(mem.get("beta"), "2");
    EXPECT_EQ(mem.get("gamma"), "3");
}

TEST(MemTable, GetMissingKey) {
    lsm::MemTable mem(1024 * 1024);
    EXPECT_EQ(mem.get("missing"), std::nullopt);
}

TEST(MemTable, UpdateKey) {
    lsm::MemTable mem(1024 * 1024);
    mem.put("key", "old");
    mem.put("key", "new");
    EXPECT_EQ(mem.get("key"), "new");
}

TEST(MemTable, Contains) {
    lsm::MemTable mem(1024 * 1024);
    mem.put("key", "val");
    EXPECT_TRUE(mem.contains("key"));
    EXPECT_FALSE(mem.contains("missing"));
}

TEST(MemTable, IsFullAfterCapacity) {
    lsm::MemTable mem(32);
    EXPECT_FALSE(mem.is_full());
    mem.put("key_long_enough", "value_long_enough");
    EXPECT_TRUE(mem.is_full());
}

TEST(MemTable, IsMutableByDefault) {
    lsm::MemTable mem(1024 * 1024);
    EXPECT_TRUE(mem.is_mutable());
}

TEST(MemTable, TryMarkFlushing) {
    lsm::MemTable mem(32);
    mem.put("key_long_enough", "value_long_enough");

    EXPECT_TRUE(mem.try_mark_flushing());
    EXPECT_FALSE(mem.try_mark_flushing());
}

TEST(MemTable, IteratorOrder) {
    lsm::MemTable mem(1024 * 1024);
    mem.put("c", "3");
    mem.put("a", "1");
    mem.put("b", "2");

    auto it = mem.iterator();
    ASSERT_TRUE(it.valid());
    EXPECT_EQ(it.key(), "a");
    it.next();
    EXPECT_EQ(it.key(), "b");
    it.next();
    EXPECT_EQ(it.key(), "c");
    it.next();
    EXPECT_FALSE(it.valid());
}
