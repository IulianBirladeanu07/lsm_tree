#include "lsm/memtable/memtable.h"
#include "lsm/iterator/skiplist_iterator.h"

namespace lsm {

void MemTable::put(std::string_view key, std::string_view value) {
    if (!writable_.load(std::memory_order_acquire)) return;

    auto old_value = skip_list_.put(key, value);
    if (old_value.has_value()) {
        auto delta = static_cast<int64_t>(value.size()) - static_cast<int64_t>(old_value->size());
        if (delta > 0) {
            size_.fetch_add(delta, std::memory_order_acq_rel);
        } else {
            size_.fetch_sub(-delta, std::memory_order_acq_rel);
        }
    } else {
        size_.fetch_add(key.size() + value.size(), std::memory_order_acq_rel);
    }

    if (size_.load(std::memory_order_acquire) >= capacity_) {
        writable_.store(false, std::memory_order_release);
    }
}

std::optional<std::string> MemTable::get(std::string_view key) {
    return skip_list_.get(key);
}

bool MemTable::contains(std::string_view key) {
    return skip_list_.contains(key);
}

bool MemTable::is_full() const {
    return size_.load(std::memory_order_acquire) >= capacity_;
}

bool MemTable::is_mutable() const {
    return writable_.load(std::memory_order_acquire);
}

bool MemTable::try_mark_flushing() {
    bool expected = false;
    return flushing_.compare_exchange_strong(expected, true,
                                             std::memory_order_acq_rel,
                                             std::memory_order_acquire);
}

ConcurrentSkipList::Iterator MemTable::iterator() const {
    return skip_list_.begin();
}

std::unique_ptr<IIterator> MemTable::iiterator() const {
    return std::make_unique<SkipListIterator>(skip_list_);
}

} // namespace lsm