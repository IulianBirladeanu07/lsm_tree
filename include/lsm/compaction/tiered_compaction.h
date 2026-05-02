#pragma once
#include "lsm/compaction/compaction_strategy.h"

namespace lsm {

class TieredCompaction : public ICompactionStrategy {
public:
    explicit TieredCompaction(int size_ratio = 4);
    CompactionJob pick_job(const LevelManager& levels) override;
    double score(int level, const LevelManager& levels) const override;

private:
    int size_ratio_;
};

} // namespace lsm