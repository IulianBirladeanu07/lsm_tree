#include "lsm/lsm_tree.h"
#include "lsm/compaction/leveled_compaction.h"
#include "lsm/compaction/tiered_compaction.h"
#include "lsm/sstable/sstable_builder.h"
#include "lsm/sstable/sstable.h"
#include <algorithm>
#include <format>

namespace lsm {

LSMTree::LSMTree(std::filesystem::path dir, LSMOptions opts)
    : dir_(std::move(dir))
    , opts_(opts) {
    std::filesystem::create_directories(dir_);
    wal_ = std::make_unique<WAL>(dir_ / "wal.log", opts_.sync_writes);
    mem_.store(std::make_shared<MemTable>(opts_.memtable_size));
    levels_ = std::make_shared<LevelManager>(opts_.num_levels);
    thread_pool_ = std::make_shared<ThreadPool>(opts_.compaction_threads);

    std::unique_ptr<ICompactionStrategy> strategy;
    if (opts_.compaction_style == LSMOptions::CompactionStyle::Tiered)
        strategy = std::make_unique<TieredCompaction>();
    else
        strategy = std::make_unique<LeveledCompaction>();

    compaction_ = std::make_unique<CompactionScheduler>(
        std::move(strategy),
        levels_,
        thread_pool_,
        [this] { return next_sst_path(); });
}

LSMTree::~LSMTree() {
    thread_pool_->shutdown();
}

void LSMTree::put(std::string_view key, std::string_view value) {
    wal_->append(key, value);

    auto mem = std::atomic_load(&mem_);
    mem->put(key, value);

    if (mem->is_full() && mem->try_mark_flushing()) {
        flush_memtable(mem);
    }
}

std::optional<std::string> LSMTree::get(std::string_view key) const {
    auto is_tombstone = [](const std::optional<std::string>& r) {
        return r && *r == kTombstone;
    };

    auto mem = std::atomic_load(&mem_);
    auto r = mem->get(key);
    if (r) return is_tombstone(r) ? std::nullopt : r;

    {
        std::shared_lock lock(imm_mu_);
        for (auto it = imm_.rbegin(); it != imm_.rend(); ++it) {
            r = (*it)->get(key);
            if (r) return is_tombstone(r) ? std::nullopt : r;
        }
    }

    for (int level = 0; level < levels_->num_levels(); ++level) {
        auto sstables = levels_->get_level(level);
        for (auto& sst : sstables) {
            r = sst->get(key);
            if (r) return is_tombstone(r) ? std::nullopt : r;
        }
    }

    return std::nullopt;
}

void LSMTree::del(std::string_view key) {
    wal_->remove(key);
    auto mem = std::atomic_load(&mem_);
    mem->put(key, kTombstone);
}

void LSMTree::flush_memtable(std::shared_ptr<MemTable> old_mem) {
    {
        std::unique_lock lock(imm_mu_);
        imm_.push_back(old_mem);
    }

    auto new_mem = std::make_shared<MemTable>(opts_.memtable_size);
    std::atomic_store(&mem_, new_mem);

    thread_pool_->submit([this, old_mem] {
        auto path = next_sst_path();
        SSTableBuilder builder(path, opts_.block_size, 1000, opts_.bloom_fpr);

        auto iter = old_mem->iterator();
        while (iter.valid()) {
            builder.add(iter.key(), iter.value());
            iter.next();
        }

        auto sst = std::make_shared<SSTable>(builder.finish());
        levels_->add_sstable(0, sst);

        {
            std::unique_lock lock(imm_mu_);
            imm_.erase(std::remove(imm_.begin(), imm_.end(), old_mem),
                       imm_.end());
        }

        compaction_->schedule();
    });
}

std::string LSMTree::next_sst_path() {
    auto id = sst_counter_.fetch_add(1, std::memory_order_relaxed);
    return (dir_ / std::format("sst_{:06d}.sst", id)).string();
}

} // namespace lsm