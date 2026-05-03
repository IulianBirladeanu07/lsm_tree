#include <gtest/gtest.h>
#include <filesystem>
#include "lsm/wal/wal.h"

class WALTest : public ::testing::Test {
protected:
    std::filesystem::path path = "/tmp/test_wal_gtest.log";

    void SetUp() override {
        std::filesystem::remove(path);
    }

    void TearDown() override {
        std::filesystem::remove(path);
    }
};

TEST_F(WALTest, AppendAndRecover) {
    {
        lsm::WAL wal(path);
        wal.append("alpha", "1");
        wal.append("beta", "2");
        wal.append("gamma", "3");
        wal.close();
    }

    lsm::WAL wal(path);
    auto entries = wal.recover();

    ASSERT_EQ(entries.size(), 3u);
    EXPECT_EQ(entries[0].key, "alpha");
    EXPECT_EQ(entries[0].value, "1");
    EXPECT_FALSE(entries[0].is_delete);
    EXPECT_EQ(entries[1].key, "beta");
    EXPECT_EQ(entries[2].key, "gamma");
}

TEST_F(WALTest, RemoveWritesTombstone) {
    {
        lsm::WAL wal(path);
        wal.append("alpha", "1");
        wal.remove("alpha");
        wal.close();
    }

    lsm::WAL wal(path);
    auto entries = wal.recover();

    ASSERT_EQ(entries.size(), 2u);
    EXPECT_FALSE(entries[0].is_delete);
    EXPECT_TRUE(entries[1].is_delete);
    EXPECT_EQ(entries[1].key, "alpha");
}

TEST_F(WALTest, RecoverEmptyFile) {
    lsm::WAL wal(path);
    auto entries = wal.recover();
    EXPECT_TRUE(entries.empty());
}

TEST_F(WALTest, AppendAfterRecover) {
    {
        lsm::WAL wal(path);
        wal.append("a", "1");
        wal.close();
    }

    {
        lsm::WAL wal(path);
        wal.append("b", "2");
        wal.close();
    }

    lsm::WAL wal(path);
    auto entries = wal.recover();
    ASSERT_EQ(entries.size(), 2u);
    EXPECT_EQ(entries[0].key, "a");
    EXPECT_EQ(entries[1].key, "b");
}

TEST_F(WALTest, Truncate) {
    {
        lsm::WAL wal(path);
        wal.append("a", "1");
        wal.append("b", "2");
        wal.truncate();
        wal.close();
    }

    lsm::WAL wal(path);
    auto entries = wal.recover();
    EXPECT_TRUE(entries.empty());
}

TEST_F(WALTest, AppendAfterTruncate) {
    lsm::WAL wal(path);
    wal.append("a", "1");
    wal.truncate();
    wal.append("b", "2");
    wal.close();

    lsm::WAL wal2(path);
    auto entries = wal2.recover();
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_EQ(entries[0].key, "b");
}
