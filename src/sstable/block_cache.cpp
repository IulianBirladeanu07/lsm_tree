#include "lsm/sstable/block_cache.h"

namespace lsm {

std::optional<Block> BlockCache::get(uint64_t file_id, uint64_t block_offset) {
    std::lock_guard<std::mutex> lock(mutex_);
    Key key {file_id, block_offset};

    auto it = table_.find(key);
    if (it == table_.end()) {
        return std::nullopt;
    }
    lru_.splice(lru_.end(), lru_, it->second);
    return it->second->second;
}

void BlockCache::put(uint64_t file_id, uint64_t block_offset, Block block) {
    std::lock_guard<std::mutex> lock(mutex_);
    Key key {file_id, block_offset};

    auto it = table_.find(key);
    if (it == table_.end()) {
        lru_.push_back({key, std::move(block)});
        table_[key] = std::prev(lru_.end());
        usage_ += lru_.back().second.size();
    } else {
        usage_ -= it->second->second.size();
        it->second->second = std::move(block);
        usage_ += it->second->second.size();
        lru_.splice(lru_.end(), lru_, it->second);
    }

    while (usage_ > capacity_) {
        auto& evicted = lru_.front();
        usage_ -= evicted.second.size();
        table_.erase(evicted.first);
        lru_.pop_front();
    }
}

std::size_t BlockCache::capacity() const {
    return capacity_;
}

std::size_t BlockCache::usage() const {
    return usage_;
}

} // namespace lsm