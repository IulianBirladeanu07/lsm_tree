#pragma once
#include "lsm/sstable/block.h"

#include <unordered_map>
#include <optional>
#include <cstdint>
#include <mutex>
#include <list>

namespace lsm {

struct PairHash {
    std::size_t operator()(const std::pair<uint64_t, uint64_t>& p) const {
        return std::hash<uint64_t>{}(p.first) ^ (std::hash<uint64_t>{}(p.second) << 32);
    }
};

class BlockCache {
public:
    explicit BlockCache(std::size_t capacity) : capacity_(capacity), usage_(0) {}

    std::optional<Block> get(uint64_t file_id, uint64_t block_offset);
    void put(uint64_t file_id, uint64_t block_offset, Block block);

    std::size_t capacity() const;
    std::size_t usage() const;


private:
    using Key = std::pair<uint64_t, uint64_t>;
    std::list<std::pair<Key, Block>> lru_;
    std::unordered_map<Key, std::list<std::pair<Key, Block>>::iterator, PairHash> table_;
    std::size_t capacity_;
    std::size_t usage_;
    std::mutex mutex_;

};
}