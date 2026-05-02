#pragma once
#include "lsm/compaction/compaction_job.h"

namespace lsm {

class LevelManager;

class ICompactionStrategy {
public:
    virtual ~ICompactionStrategy() = default;
    virtual CompactionJob pick_job(const LevelManager& levels) = 0;
    virtual double score(int level, const LevelManager& levels) const = 0;
};

} // namespace lsm