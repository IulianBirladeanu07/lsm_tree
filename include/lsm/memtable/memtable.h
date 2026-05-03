#pragma once
#include "lsm/memtable/concurrent_skip_list.h"
#include <optional>
#include <atomic>
#include <string>

namespace lsm {

class MemTable {
public:
    explicit MemTable(size_t capacity = 64 * 1024 * 1024) : size_(0), capacity_(capacity) {}
    void put(std::string_view key, std::string_view value);
    std::optional<std::string> get(std::string_view key);
    bool contains(std::string_view key);
    bool is_full() const;
    bool is_mutable() const;
    bool try_mark_flushing();
    ConcurrentSkipList::Iterator iterator() const;

private:
    ConcurrentSkipList skip_list_;
    std::atomic<bool> writable_{true};
    std::atomic<bool> flushing_{false};
    std::atomic<size_t> size_;
    size_t capacity_;


};

} // namespace lsm