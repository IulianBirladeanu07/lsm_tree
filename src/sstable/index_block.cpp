#include "lsm/sstable/index_block.h"
#include <cstring>
#include <stdexcept>

namespace lsm {

void IndexBlock::add(std::string first_key, std::string last_key, uint64_t offset, uint64_t size) {
    entries_.push_back({std::move(first_key), std::move(last_key), offset, size});
}

std::vector<uint8_t> IndexBlock::serialize() const {
    std::vector<uint8_t> out;

    for (const auto& e : entries_) {
        uint32_t first_len = static_cast<uint32_t>(e.first_key.size());
        uint32_t last_len  = static_cast<uint32_t>(e.last_key.size());

        std::size_t pos = out.size();
        out.resize(pos + sizeof(uint32_t) + first_len + sizeof(uint32_t) + last_len + sizeof(uint64_t) * 2);

        std::memcpy(out.data() + pos, &first_len, sizeof(uint32_t));
        pos += sizeof(uint32_t);

        std::memcpy(out.data() + pos, e.first_key.data(), first_len);
        pos += first_len;

        std::memcpy(out.data() + pos, &last_len, sizeof(uint32_t));
        pos += sizeof(uint32_t);

        std::memcpy(out.data() + pos, e.last_key.data(), last_len);
        pos += last_len;

        std::memcpy(out.data() + pos, &e.offset, sizeof(uint64_t));
        pos += sizeof(uint64_t);

        std::memcpy(out.data() + pos, &e.size, sizeof(uint64_t));
    }

    return out;
}

IndexBlock IndexBlock::deserialize(std::span<const uint8_t> data) {
    IndexBlock block;
    std::size_t pos = 0;

    while (pos < data.size()) {
        uint32_t first_len;
        uint32_t last_len;
        uint64_t offset, size;

        std::memcpy(&first_len, data.data() + pos, sizeof(uint32_t));
        pos += sizeof(uint32_t);

        std::string first_key(reinterpret_cast<const char*>(data.data() + pos), first_len);
        pos += first_len;

        std::memcpy(&last_len, data.data() + pos, sizeof(uint32_t));
        pos += sizeof(uint32_t);
    
        std::string last_key(reinterpret_cast<const char*>(data.data() + pos), last_len);
        pos += last_len;

        std::memcpy(&offset, data.data() + pos, sizeof(uint64_t));
        pos += sizeof(uint64_t);

        std::memcpy(&size,   data.data() + pos, sizeof(uint64_t));
        pos += sizeof(uint64_t);

        block.entries_.push_back({std::move(first_key), std::move(last_key), offset, size});
    }

    return block;
}

} // namespace lsm