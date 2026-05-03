#include <gtest/gtest.h>
#include <filesystem>
#include "lsm/compaction/leveled_compaction.h"
#include "lsm/compaction/tiered_compaction.h"
#include "lsm/levels/level_manager.h"
#include "lsm/sstable/sstable_builder.h"
#include "lsm/sstable/sstable.h"

class CompactionStrategyTest : public ::testing::Test {
protected:
    int counter = 0;

    std::shared_ptr<lsm::SSTable> make_sst() {
        auto path = "/tmp/cs_gtest_" + std::to_string(counter++) + ".sst";
        lsm::SSTableBuilder builder(path, 4096, 10, 0.01);
        builder.add("key", "val");
        auto sst = builder.finish();
        return std::make_shared<lsm::SSTable>(std::move(sst));
    }

    void TearDown() override {
        for (int i = 0; i < counter; i++) {
            std::filesystem::remove("/tmp/cs_gtest_" + std::to_string(i) + ".sst");
        }
    }
};

TEST_F(CompactionStrategyTest, TieredScoreExceedsThreshold) {
    lsm::LevelManager levels(4);
    for (int i = 0; i < 5; i++) levels.add_sstable(0, make_sst());

    lsm::TieredCompaction tiered(4);
    EXPECT_GT(tiered.score(0, levels), 1.0);
}

TEST_F(CompactionStrategyTest, TieredPicksJobWhenFull) {
    lsm::LevelManager levels(4);
    for (int i = 0; i < 4; i++) levels.add_sstable(0, make_sst());

    lsm::TieredCompaction tiered(4);
    auto job = tiered.pick_job(levels);

    EXPECT_EQ(job.output_level, 1);
    EXPECT_FALSE(job.inputs.empty());
}

TEST_F(CompactionStrategyTest, TieredNoJobWhenBelowThreshold) {
    lsm::LevelManager levels(4);
    levels.add_sstable(0, make_sst());
    levels.add_sstable(0, make_sst());

    lsm::TieredCompaction tiered(4);
    auto job = tiered.pick_job(levels);
    EXPECT_TRUE(job.inputs.empty());
}

TEST_F(CompactionStrategyTest, LeveledScoreZeroWhenEmpty) {
    lsm::LevelManager levels(4);
    lsm::LeveledCompaction leveled;
    EXPECT_EQ(leveled.score(0, levels), 0.0);
}

TEST_F(CompactionStrategyTest, LeveledNoJobWhenEmpty) {
    lsm::LevelManager levels(4);
    lsm::LeveledCompaction leveled;
    auto job = leveled.pick_job(levels);
    EXPECT_TRUE(job.inputs.empty());
}
