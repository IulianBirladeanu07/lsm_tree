#include "lsm/iterator/block_iterator.h"

namespace lsm {

BlockIterator::BlockIterator(const Block& block) : block_(block), offset_(0), valid_(false) {
    parse_current();
}

void BlockIterator::next() {
    if (!valid_) return;

    offset_ += Block::kHeaderSize + current_key_.size() + current_value_.size();
    parse_current();
}

void BlockIterator::seek(std::string_view key) {
    offset_ = 0;
    valid_ = false;

    while (offset_ + Block::kHeaderSize <= block_.size()) {
        uint32_t key_len = block_.read_uint32(offset_);
        uint32_t val_len = block_.read_uint32(offset_ + 4);

        if (offset_ + Block::kHeaderSize + key_len + val_len > block_.size()) {
            valid_ = false;
            return; // Corrupted block
        }

        std::string_view current_key(
            reinterpret_cast<const char*>(block_.data().data() + offset_ + Block::kHeaderSize),
            key_len
        );

        if (current_key >= key) {
            parse_current();
            return;
        }
        offset_ += Block::kHeaderSize + key_len + val_len;
    }
    valid_ = false; // Reached end of block
}


void BlockIterator::parse_current() {
    if (offset_ + Block::kHeaderSize > block_.size()) {
        valid_ = false;
        return; // No more entries
    }

    uint32_t key_len = block_.read_uint32(offset_);
    uint32_t val_len = block_.read_uint32(offset_ + 4);

    if (offset_ + Block::kHeaderSize + key_len + val_len > block_.size()) {
        valid_ = false;
        return; // Corrupted block
    }

    current_key_ = std::string(
        reinterpret_cast<const char*>(block_.data().data() + offset_ + Block::kHeaderSize),
        key_len
    );
    current_value_ = std::string(
        reinterpret_cast<const char*>(block_.data().data() + offset_ + Block::kHeaderSize + key_len),
        val_len
    );
    valid_ = true;
}

std::string_view BlockIterator::key() const {
    return current_key_;
}

std::string_view BlockIterator::value() const {
    return current_value_;
}

bool BlockIterator::valid() const {
    return valid_;
}

} // namespace lsm