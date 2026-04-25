#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <span>

namespace lsm {

struct IndexEntry {
    std::string last_key;
    uint64_t    offset;
    uint64_t    size;
};

class IndexBlock {
public:
    void add(std::string last_key, uint64_t offset, uint64_t size);

    std::vector<uint8_t>        serialize() const;
    static IndexBlock           deserialize(std::span<const uint8_t> data);

    const std::vector<IndexEntry>& entries() const { return entries_; }

private:
    std::vector<IndexEntry> entries_;
};

} // namespace lsm