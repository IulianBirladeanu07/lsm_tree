#include "lsm/compaction/tiered_compaction.h"
#include "lsm/levels/level_manager.h"

namespace lsm {

TieredCompaction::TieredCompaction(int size_ratio) : size_ratio_(size_ratio) {}

CompactionJob TieredCompaction::pick_job(const LevelManager& levels) {
    for (int level = 0; level < levels.num_levels() - 1; ++level) {
        auto sstables = levels.get_level(level);
        if (sstables.size() >= static_cast<size_t>(size_ratio_)) {
            return CompactionJob{sstables, level + 1, ""};
        }
    }
    return CompactionJob{{}, -1, ""};
}

double TieredCompaction::score(int level, const LevelManager& levels) const {
    auto sstables = levels.get_level(level);
    return static_cast<double>(sstables.size()) / size_ratio_;
}

} // namespace lsm