#include "lsm/sstable/sstable_builder.h"
#include "lsm/sstable/sstable.h"
#include "lsm/sstable/footer.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

namespace lsm {

SSTableBuilder::SSTableBuilder(std::filesystem::path path, std::size_t block_size, std::size_t expected_keys, double bloom_fpr)
    : path_(std::move(path))
    , current_block_(block_size)
    , filter_(expected_keys, bloom_fpr) {

    fd_ = ::open(path_.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_ < 0) {
        throw std::runtime_error("SSTableBuilder: cannot open file: " + path_.string());
    }
}

SSTableBuilder::~SSTableBuilder() {
    if (fd_ >= 0) {
        ::close(fd_);
    }
}

void SSTableBuilder::write_bytes(const uint8_t* data, std::size_t len) {
    while (len > 0) {
        ssize_t written = ::write(fd_, data, len);
        if (written <= 0) {
           throw std::runtime_error("SSTableBuilder:: write failed"); 
        }
        data += written;
        len  -= static_cast<std::size_t>(written);
    }
}

void SSTableBuilder::flush_block() {
    if (current_block_.current_size() == 0) return;

    auto block_data = current_block_.finish();
    uint64_t block_size = block_data.size();
    
    index_.add(last_key_, offset_, block_size);
    write_bytes(block_data.data(), block_data.size());
    offset_ += block_size;

    current_block_.reset();
}

void SSTableBuilder::add(std::string_view key, std::string_view value) {
    filter_.add(key);
    last_key_ = std::string(key);

    if (current_block_.is_full()) {
        flush_block();
    }

    current_block_.add(key, value);
}

SSTable SSTableBuilder::finish() {
    flush_block();

    uint64_t index_offset = offset_;
    auto index_data = index_.serialize();
    write_bytes(index_data.data(), index_data.size());
    offset_ += index_data.size();

    uint64_t filter_offset = offset_;
    auto filter_data = filter_.serialize();
    write_bytes(filter_data.data(), filter_data.size());
    offset_ += filter_data.size();

    Footer footer{index_offset, filter_offset, kMagic};
    auto footer_data = footer.serialize();
    write_bytes(footer_data.data(), footer_data.size());

    ::close(fd_);
    fd_ = -1;

    return SSTable::open(path_);
}

std::size_t SSTableBuilder::estimated_size() const {
    return static_cast<std::size_t>(offset_) + current_block_.current_size();
}

} // namspace lsm