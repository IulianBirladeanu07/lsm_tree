#include "lsm/sstable/block_builder.h"
#include "lsm/util/crc32.h"
#include <cstring>

namespace lsm {

BlockBuilder::BlockBuilder(std::size_t block_size) : block_size_(block_size) {}

void BlockBuilder::append_uint32(uint32_t value) {
    buffer_.resize(buffer_.size() + sizeof(uint32_t));
    std::memcpy(buffer_.data() + buffer_.size() - sizeof(uint32_t), &value, sizeof(uint32_t));
}

void BlockBuilder::append_bytes(std::string_view data) {
    buffer_.resize(buffer_.size() + data.size());
    std::memcpy(buffer_.data() + buffer_.size() - data.size(), data.data(), data.size());
}

void BlockBuilder::add(std::string_view key, std::string_view value) {
    append_uint32(static_cast<uint32_t>(key.size()));
    append_uint32(static_cast<uint32_t>(value.size()));
    append_bytes(key);
    append_bytes(value);
    num_entries_++;
}

std::vector<uint8_t> BlockBuilder::finish() {
    uint32_t checksum = crc32c(buffer_);

    buffer_.resize(buffer_.size() + sizeof(uint32_t));
    std::memcpy(buffer_.data() + buffer_.size() - sizeof(uint32_t), &checksum, sizeof(checksum));

    return std::move(buffer_);
}

bool BlockBuilder::is_full() const {
    return buffer_.size() >= block_size_;
}

std::size_t BlockBuilder::current_size() const {
    return buffer_.size();
}

void BlockBuilder::reset() {
    buffer_.clear();
    num_entries_ = 0;
}

} // namespace lsm