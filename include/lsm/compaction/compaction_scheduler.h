#pragma once

#include "lsm/levels/level_manager.h"
#include "lsm/iterator/merging_iterator.h"
#include "lsm/compaction/compaction_strategy.h"
#include "lsm/compaction/thread_pool.h"
#include "lsm/compaction/compaction_job.h"
#include <functional>
#include <memory>

namespace lsm {

class CompactionScheduler {
public:
    CompactionScheduler(std::unique_ptr<ICompactionStrategy> strategy,
                        std::shared_ptr<LevelManager> levels,
                        std::shared_ptr<ThreadPool> thread_pool,
                        std::function<std::string()> path_generator);
    ~CompactionScheduler();

    void schedule();

private:
    void run_compaction(const CompactionJob& job);

    std::unique_ptr<ICompactionStrategy> strategy_;
    std::shared_ptr<LevelManager> levels_;
    std::shared_ptr<ThreadPool> thread_pool_;
    std::function<std::string()> path_generator_;
};

} // namespace lsm