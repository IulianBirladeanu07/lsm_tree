#include "lsm/sstable/block.h"
#include <cstring>

namespace lsm {
Block::Block(std::vector<uint8_t> data) : data_(std::move(data)) {}


uint32_t Block::read_uint32(std::size_t offset) const {
    uint32_t val;
    std::memcpy(&val, data_.data() + offset, sizeof(uint32_t));
    return val;    
}

bool Block::key_matches(std::size_t offset, std::string_view key) const {
    uint32_t key_len = read_uint32(offset);
    if(key_len != key.size()) return false;

    return memcmp(data_.data() + offset + kHeaderSize, key.data(), key_len) == 0;
}

std::optional<std::string> Block::get(std::string_view key) const {
    std::size_t offset = 0;

    while (offset < data_.size() - 4) {
        uint32_t key_len = read_uint32(offset);
        uint32_t val_len = read_uint32(offset + 4);

        if (key_matches(offset, key)) {
            return std::string(
                reinterpret_cast<const char*>(data_.data() + offset + kHeaderSize + key_len),
                val_len
            );
        }
        offset += kHeaderSize + key_len + val_len; 
    }
    return std::nullopt;

}

} // namespace lsm