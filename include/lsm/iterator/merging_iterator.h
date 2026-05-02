#pragma once
#include "lsm/iterator/iterator.h"
#include <queue>
#include <vector>
#include <memory>
#include <string_view>

namespace lsm {

struct IteratorComparator {
    bool operator()(const IIterator* a, const IIterator* b) const {
        return a->key() > b->key();
    }
};

class MergingIterator : public IIterator {
public:
    MergingIterator(std::vector<std::unique_ptr<IIterator>> iterators);
    ~MergingIterator();

    bool valid() const override;
    void next() override;
    std::string_view key() const override;
    std::string_view value() const override;
    void seek(std::string_view key) override;

private:
    std::priority_queue<IIterator*,
                        std::vector<IIterator*>,
                        IteratorComparator> heap_;
    std::vector<std::unique_ptr<IIterator>> children_;
};

} // namespace lsm