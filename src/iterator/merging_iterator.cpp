#include "lsm/iterator/merging_iterator.h"

namespace lsm {

MergingIterator::MergingIterator(std::vector<std::unique_ptr<IIterator>> iterators) {
    for(auto& it : iterators) {
        if(it->valid()) {
            heap_.push(it.get());
        }
        children_.push_back(std::move(it));
    }
}

MergingIterator::~MergingIterator() {}

bool MergingIterator::valid() const {
    return !heap_.empty();
}

void MergingIterator::next() {
    if (heap_.empty()) return;

    auto top = heap_.top();
    heap_.pop();
    top->next();
    if (top->valid()) {
        heap_.push(top);
    }
}

std::string_view MergingIterator::key() const {
    if (heap_.empty()) return {};
    return heap_.top()->key();
}

std::string_view MergingIterator::value() const {
    if (heap_.empty()) return {};
    return heap_.top()->value();
}

void MergingIterator::seek(std::string_view key) {
    // Clear the heap and re-seek all children
    std::priority_queue<IIterator*, std::vector<IIterator*>, IteratorComparator> new_heap;
    for (auto& child : children_) {
        child->seek(key);
        if (child->valid()) {
            new_heap.push(child.get());
        }
    }
    heap_ = std::move(new_heap);
}

} // namespace lsm