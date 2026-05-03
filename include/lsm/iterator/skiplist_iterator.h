#pragma once
#include "lsm/iterator/iterator.h"
#include "lsm/memtable/concurrent_skip_list.h"

namespace lsm {

class SkipListIterator : public IIterator {
public:
    explicit SkipListIterator(const ConcurrentSkipList& list)
        : list_(list), iter_(list.begin()) {}

    bool valid() const override { return iter_.valid(); }
    void next() override { iter_.next(); }
    std::string_view key() const override { return iter_.key(); }
    std::string_view value() const override { return iter_.value(); }

    void seek(std::string_view key) override {
        iter_ = list_.seek(key);
    }

private:
    const ConcurrentSkipList& list_;
    ConcurrentSkipList::Iterator iter_;
};

} // namespace lsm
