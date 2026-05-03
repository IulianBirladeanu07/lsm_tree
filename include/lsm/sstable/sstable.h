#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include "lsm/sstable/block.h"
#include "lsm/sstable/index_block.h"
#include "lsm/sstable/footer.h"
#include "lsm/util/bloom_filter.h"
#include <unistd.h>

namespace lsm {

class SSTable {
public:
    static SSTable open(std::filesystem::path path);

    std::optional<std::string> get(std::string_view key) const;

    std::size_t file_size() const { return file_size_; }
    std::string_view smallest_key() const {
        return index_.entries().empty() ? std::string_view() : index_.entries().front().first_key;
    }
    std::string_view largest_key() const {
        return index_.entries().empty() ? std::string_view() : index_.entries().back().last_key;
    }

    uint64_t file_id() const { return file_id_; }
    Block get_block(std::size_t idx) const;
    const IndexBlock& index() const;

    SSTable(SSTable&& other) noexcept
        : fd_(std::exchange(other.fd_, -1))
        , file_id_(other.file_id_)
        , file_size_(other.file_size_)
        , index_(std::move(other.index_))
        , footer_(other.footer_)
        , filter_(std::move(other.filter_)) {}

    SSTable& operator=(SSTable&& other) noexcept {
        if (this != &other) {
            if (fd_ >= 0) ::close(fd_);
            fd_        = std::exchange(other.fd_, -1);
            file_id_   = other.file_id_;
            file_size_ = other.file_size_;
            index_     = std::move(other.index_);
            footer_    = other.footer_;
            filter_    = std::move(other.filter_);
        }
        return *this;
    }

    ~SSTable();

private:
    SSTable() = default;

    Block read_block(const IndexEntry& entry) const;

    int fd_{-1};
    uint64_t file_id_{0};
    std::size_t file_size_{0};
    IndexBlock index_;
    Footer footer_{};
    std::optional<BloomFilter> filter_;
};

} // namespace lsm