#include "lsm/compaction/leveled_compaction.h"
#include <cmath>
#include "lsm/levels/level_manager.h"

namespace lsm {

LeveledCompaction::LeveledCompaction(std::size_t base_bytes, int multiplier)
    : base_bytes_(base_bytes), multiplier_(multiplier) {}

double LeveledCompaction::score(int level, const LevelManager& levels) const {
    auto sstables = levels.get_level(level);
    std::size_t total_size = 0;
    for (const auto& sstable : sstables) {
        total_size += sstable->file_size();
    }
    std::size_t level_threshold = base_bytes_ * static_cast<std::size_t>(std::pow(multiplier_, level));
    return static_cast<double>(total_size) / level_threshold;
}

CompactionJob LeveledCompaction::pick_job(const LevelManager& levels) {
    for (int level = 0; level < levels.num_levels() - 1; ++level) {
        if (score(level, levels) > 1.0) {
            auto sstables = levels.get_level(level);
            if (!sstables.empty()) {
                return CompactionJob{sstables, level + 1, ""};
            }
        }
    }
    return CompactionJob{{}, -1, ""};
}

} // namespace lsm