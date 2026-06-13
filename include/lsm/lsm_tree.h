#pragma once

#include "lsm/wal/wal.h"
#include "lsm/memtable/memtable.h"
#include "lsm/levels/level_manager.h"
#include "lsm/compaction/compaction_scheduler.h"
#include "lsm/compaction/thread_pool.h"
#include "lsm/sstable/block_cache.h"

#include <atomic>
#include <shared_mutex>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>

namespace lsm {

static constexpr std::string_view kTombstone = "\xFF";

struct LSMOptions {
    enum class CompactionStyle { Leveled, Tiered };

    std::size_t      memtable_size      = 64 * 1024 * 1024;
    std::size_t      block_cache_size   = 8  * 1024 * 1024;
    int              num_levels         = 7;
    int              compaction_threads = 2;
    bool             sync_writes        = false;
    double           bloom_fpr          = 0.01;
    std::size_t      block_size         = 4096;
    CompactionStyle  compaction_style   = CompactionStyle::Leveled;
};

struct KV {
    std::string key;
    std::string value;
};

class LSMTree {
public:
    explicit LSMTree(std::filesystem::path dir, LSMOptions opts = {});
    ~LSMTree();

    void                       put(std::string_view key, std::string_view value);
    std::optional<std::string> get(std::string_view key) const;
    void                       del(std::string_view key);
    std::vector<KV>            scan(std::string_view start, std::string_view end) const;

private:
    void        flush_memtable(std::shared_ptr<MemTable> old_mem);
    std::string next_sst_path();

    std::filesystem::path dir_;
    LSMOptions            opts_;

    std::unique_ptr<WAL> wal_;

    std::atomic<std::shared_ptr<MemTable>> mem_;
    std::vector<std::shared_ptr<MemTable>> imm_;
    mutable std::shared_mutex              imm_mu_;

    std::shared_ptr<BlockCache>          block_cache_;
    std::shared_ptr<LevelManager>        levels_;
    std::shared_ptr<ThreadPool>          thread_pool_;
    std::unique_ptr<CompactionScheduler> compaction_;

    std::atomic<uint64_t> sst_counter_{0};
};

} // namespace lsm