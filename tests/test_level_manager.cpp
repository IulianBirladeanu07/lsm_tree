#include <gtest/gtest.h>
#include <filesystem>
#include "lsm/levels/level_manager.h"
#include "lsm/sstable/sstable_builder.h"
#include "lsm/sstable/sstable.h"

class LevelManagerTest : public ::testing::Test {
protected:
    int counter = 0;

    std::shared_ptr<lsm::SSTable> make_sst() {
        auto path = "/tmp/lm_test_" + std::to_string(counter++) + ".sst";
        lsm::SSTableBuilder builder(path, 4096, 10, 0.01);
        builder.add("key", "val");
        auto sst = builder.finish();
        return std::make_shared<lsm::SSTable>(std::move(sst));
    }

    void TearDown() override {
        for (int i = 0; i < counter; i++) {
            std::filesystem::remove("/tmp/lm_test_" + std::to_string(i) + ".sst");
        }
    }
};

TEST_F(LevelManagerTest, AddSSTable) {
    lsm::LevelManager mgr(4);
    mgr.add_sstable(0, make_sst());
    mgr.add_sstable(0, make_sst());
    mgr.add_sstable(1, make_sst());

    EXPECT_EQ(mgr.get_level(0).size(), 2u);
    EXPECT_EQ(mgr.get_level(1).size(), 1u);
    EXPECT_EQ(mgr.get_level(2).size(), 0u);
}

TEST_F(LevelManagerTest, NumLevels) {
    lsm::LevelManager mgr(7);
    EXPECT_EQ(mgr.num_levels(), 7);
}

TEST_F(LevelManagerTest, ReplaceLevel) {
    lsm::LevelManager mgr(4);
    auto sst1 = make_sst();
    auto sst2 = make_sst();
    auto sst3 = make_sst();

    mgr.add_sstable(0, sst1);
    mgr.add_sstable(0, sst2);

    mgr.replace_level({sst1, sst2}, {sst3}, 1);

    EXPECT_EQ(mgr.get_level(0).size(), 0u);
    EXPECT_EQ(mgr.get_level(1).size(), 1u);
}

TEST_F(LevelManagerTest, GetInvalidLevelReturnsEmpty) {
    lsm::LevelManager mgr(4);
    EXPECT_TRUE(mgr.get_level(99).empty());
}

TEST_F(LevelManagerTest, ReplaceWithMultipleOutputs) {
    lsm::LevelManager mgr(4);
    auto sst1 = make_sst();
    auto out1 = make_sst();
    auto out2 = make_sst();

    mgr.add_sstable(0, sst1);
    mgr.replace_level({sst1}, {out1, out2}, 1);

    EXPECT_EQ(mgr.get_level(0).size(), 0u);
    EXPECT_EQ(mgr.get_level(1).size(), 2u);
}
