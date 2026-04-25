#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include "lsm/sstable/block.h"
#include "lsm/sstable/index_block.h"
#include "lsm/sstable/footer.h"
#include "lsm/util/bloom_filter.h"

namespace lsm {

class SSTable {
public:
    static SSTable open(std::filesystem::path path);

    std::optional<std::string> get(std::string_view key) const;

    std::size_t      file_size()    const { return file_size_; }
    std::string_view smallest_key() const { return index_.entries().front().last_key; }
    std::string_view largest_key()  const { return index_.entries().back().last_key; }
    
    uint64_t         file_id()      const { return file_id_; }
    SSTable(SSTable&&) = default;
    SSTable& operator=(SSTable&&) = default;
    ~SSTable();

private:
    SSTable() = default;

    Block       read_block(const IndexEntry& entry) const;

    int         fd_{-1};
    uint64_t    file_id_{0};
    std::size_t file_size_{0};
    IndexBlock  index_;
    BloomFilter filter_;
    Footer      footer_{};
};
} // namespace lsm