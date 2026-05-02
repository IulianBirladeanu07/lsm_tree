#pragma once
#include "lsm/compaction/compaction_strategy.h"

namespace lsm {

class LeveledCompaction : public ICompactionStrategy {
public:
    explicit LeveledCompaction(std::size_t base_bytes = 256 * 1024 * 1024,
                               int multiplier = 10);
    CompactionJob pick_job(const LevelManager& levels) override;
    double score(int level, const LevelManager& levels) const override;

private:
    std::size_t base_bytes_;
    int multiplier_;
};

} // namespace lsm