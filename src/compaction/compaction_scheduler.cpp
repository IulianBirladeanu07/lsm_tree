#include "lsm/compaction/compaction_scheduler.h"
#include "lsm/iterator/sstable_iterator.h"
#include "lsm/sstable/sstable_builder.h"
#include "lsm/sstable/sstable.h"

namespace lsm {

CompactionScheduler::CompactionScheduler(std::unique_ptr<ICompactionStrategy> strategy,
                                         std::shared_ptr<LevelManager> levels,
                                         std::shared_ptr<ThreadPool> thread_pool)
    : strategy_(std::move(strategy)), levels_(std::move(levels)), thread_pool_(std::move(thread_pool)) {}

CompactionScheduler::~CompactionScheduler() {}

void CompactionScheduler::schedule() {
    auto job = strategy_->pick_job(*levels_);
    if (job.inputs.empty()) {
        return;
    }
    thread_pool_->submit([this, job] {
        run_compaction(job);
    });
}

void CompactionScheduler::run_compaction(const CompactionJob& job) {
    std::vector<std::unique_ptr<IIterator>> iters;
    for (const auto& sst : job.inputs) {
        iters.push_back(std::make_unique<SSTableIterator>(*sst));
    }

    MergingIterator merged(std::move(iters));

    SSTableBuilder builder(job.output_path, 4096, 
                           job.inputs.size() * 100, 0.01);

    while (merged.valid()) {
        builder.add(merged.key(), merged.value());
        merged.next();
    }

    auto new_sst = std::make_shared<SSTable>(builder.finish());
    levels_->replace_level(job.inputs, {new_sst}, job.output_level);
}

} // namespace lsm