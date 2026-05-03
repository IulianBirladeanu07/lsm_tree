#include "lsm/iterator/sstable_iterator.h"

namespace lsm {

SSTableIterator::SSTableIterator(const SSTable& sst) : sst_(sst), block_idx_(0) {
    if (!sst_.index().entries().empty()) {
        load_block(0);
    }
}

void SSTableIterator::next() {
    if (!block_iter_.has_value()) return;

    block_iter_->next();

    while (block_iter_.has_value() && !block_iter_->valid()) {
        block_idx_++;
        if (block_idx_ < sst_.index().entries().size()) {
            load_block(block_idx_);
        } else {
            block_iter_.reset();
            current_block_.reset();
            break;
        }
    }
}

void SSTableIterator::seek(std::string_view key) {
    const auto& entries = sst_.index().entries();
    std::size_t left = 0;
    std::size_t right = entries.size();

    while (left < right) {
        std::size_t mid = left + (right - left) / 2;
        if (entries[mid].last_key < key) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }

    if (left < entries.size()) {
        load_block(left);
        block_iter_->seek(key);
        while (block_iter_.has_value() && !block_iter_->valid()) {
            block_idx_++;
            if (block_idx_ < entries.size()) {
                load_block(block_idx_);
            } else {
                block_iter_.reset();
                current_block_.reset();
                break;
            }
        }
    } else {
        block_iter_.reset();
        current_block_.reset();
    }
}

void SSTableIterator::load_block(std::size_t idx) {
    current_block_ = sst_.get_block(idx);
    block_iter_.emplace(*current_block_);
}

bool SSTableIterator::valid() const {
    return block_iter_.has_value() && block_iter_->valid();
}

std::string_view SSTableIterator::key() const {
    if (!valid()) return {};
    return block_iter_->key();
}

std::string_view SSTableIterator::value() const {
    if (!valid()) return {};
    return block_iter_->value();
}

} // namespace lsm