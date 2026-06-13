#include "lsm/sstable/sstable.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdexcept>
#include <cstring>
#include <array>

namespace lsm {

SSTable::~SSTable() {
    if (fd_ >= 0) {
        ::close(fd_);
    }
}

SSTable SSTable::open(std::filesystem::path path, std::shared_ptr<BlockCache> cache) {
    SSTable sst;
    sst.cache_ = std::move(cache);

    sst.fd_ = ::open(path.c_str(), O_RDONLY);
    if (sst.fd_ < 0) {
        throw std::runtime_error("SSTable::open: cannot open file: " + path.string());
    }

    struct stat st;
    if (::fstat(sst.fd_, &st) < 0) {
        throw std::runtime_error("SSTable::open: fstat failed");
    }
    sst.file_size_ = static_cast<std::size_t>(st.st_size);
    sst.file_id_   = static_cast<uint64_t>(st.st_ino);

    if (sst.file_size_ < kFooterSize) {
        throw std::runtime_error("SSTable::open: file too small");
    }

    std::array<uint8_t, kFooterSize> footer_buf;
    ::pread(sst.fd_, footer_buf.data(), kFooterSize, static_cast<off_t>(sst.file_size_ - kFooterSize));
    sst.footer_ = Footer::deserialize(std::span<const uint8_t, kFooterSize>(footer_buf));

    uint64_t index_size = sst.footer_.filter_offset - sst.footer_.index_offset;
    std::vector<uint8_t> index_buf(index_size);
    ::pread(sst.fd_, index_buf.data(), index_size, static_cast<off_t>(sst.footer_.index_offset));
    sst.index_ = IndexBlock::deserialize(index_buf);

    uint64_t filter_size = sst.file_size_ - kFooterSize - sst.footer_.filter_offset;
    std::vector<uint8_t> filter_buf(filter_size);
    ::pread(sst.fd_, filter_buf.data(), filter_size, static_cast<off_t>(sst.footer_.filter_offset));
    sst.filter_ = BloomFilter::deserialize(filter_buf);

    return sst;
}

Block SSTable::read_block(const IndexEntry& entry) const {
    if (cache_) {
        auto cached = cache_->get(file_id_, entry.offset);
        if (cached) return std::move(*cached);
    }

    std::vector<uint8_t> buf(entry.size);
    ssize_t n = ::pread(fd_, buf.data(), entry.size, static_cast<off_t>(entry.offset));
    if (n < 0 || static_cast<uint64_t>(n) != entry.size) {
        throw std::runtime_error("SSTable::read_block: pread failed");
    }

    Block block(std::move(buf));
    if (cache_) cache_->put(file_id_, entry.offset, block);
    return block;
}

std::optional<std::string> SSTable::get(std::string_view key) const {
    if (filter_ && !filter_->may_contain(key)) {
        return std::nullopt;
    }

    for (const auto& entry : index_.entries()) {
        if (key <= entry.last_key) {
            Block block = read_block(entry);
            return block.get(key);
        }
    }

    return std::nullopt;
}

Block SSTable::get_block(std::size_t idx) const {
    return read_block(index_.entries()[idx]);
}

const IndexBlock& SSTable::index() const {
    return index_;
}

} // namespace lsm