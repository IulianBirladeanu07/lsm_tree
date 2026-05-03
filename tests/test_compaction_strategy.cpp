#include "lsm/compaction/leveled_compaction.h"
#include "lsm/compaction/tiered_compaction.h"
#include "lsm/levels/level_manager.h"
#include "lsm/sstable/sstable_builder.h"
#include <cassert>
#include <iostream>
#include <filesystem>

auto make_sst(const std::string& path) {
    lsm::SSTableBuilder builder(path, 4096, 10, 0.01);
    builder.add("key", "val");
    auto sst = builder.finish();
    return std::make_shared<lsm::SSTable>(std::move(sst));
}

int main() {
    {
        lsm::LevelManager levels(4);
        auto sst1 = make_sst("/tmp/cs1.sst");
        auto sst2 = make_sst("/tmp/cs2.sst");
        auto sst3 = make_sst("/tmp/cs3.sst");
        auto sst4 = make_sst("/tmp/cs4.sst");
        auto sst5 = make_sst("/tmp/cs5.sst");

        levels.add_sstable(0, sst1);
        levels.add_sstable(0, sst2);
        levels.add_sstable(0, sst3);
        levels.add_sstable(0, sst4);
        levels.add_sstable(0, sst5);

        lsm::TieredCompaction tiered(4);
        assert(tiered.score(0, levels) > 1.0);

        auto job = tiered.pick_job(levels);
        assert(job.output_level == 1);
        assert(!job.inputs.empty());
        std::cout << "tiered compaction: OK\n";
    }

    {
        lsm::LevelManager levels(4);
        lsm::LeveledCompaction leveled;
        double s = leveled.score(0, levels);
        assert(s == 0.0);
        std::cout << "leveled score empty: OK\n";
    }

    for (int i = 1; i <= 5; i++)
        std::filesystem::remove("/tmp/cs" + std::to_string(i) + ".sst");

    std::cout << "all tests passed\n";
    return 0;
}