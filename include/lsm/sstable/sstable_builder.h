#pragma once
#include <cstdint>
#include <filesystem>
#include <vector>
#include <string_view>
#include "lsm/sstable/block_builder.h"
#include "lsm/sstable/index_block.h"
#include "lsm/util/bloom_filter.h"

namespace lsm {

struct Options;
class  SSTable;

class SSTableBuilder {
public:
    explicit SSTableBuilder(std::filesystem::path path, std::size_t block_size,
                            std::size_t expected_keys, double bloom_fpr);
    ~SSTableBuilder();

    void        add(std::string_view key, std::string_view value);
    void        finish();
    std::size_t estimated_size() const;

private:
    void flush_block();
    void write_bytes(const uint8_t* data, std:size_t len);

    std::filesystem::path path_;
    int                   fd_;
    BlockBuilder          current_block_;
    IndexBlock            index_;
    BloomFilter           filter_;
    uint64_t              offset_{0};
    std::string           last_key_;
};

} // namespace lsm