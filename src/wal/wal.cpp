#include "lsm/wal/wal.h"
#include "lsm/util/crc32.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <array>

namespace lsm {

WAL::WAL(std::filesystem::path path, bool sync_writes) : path_(std::move(path)), sync_writes_(sync_writes) {
    fd_ = ::open(path_.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd_ < 0){
        throw std::runtime_error("WAL: cannot open file: " + path_.string());
    }
}

WAL::~WAL() {
    close();
}

void WAL::write_record(std::string_view key, std::string_view value, bool is_delete) {
    uint32_t key_len = static_cast<uint32_t>(key.size());
    uint32_t val_len = is_delete ? kDeleteSentinel : static_cast<uint32_t>(value.size());

    std::size_t data_len = key.size() + (is_delete ? 0 : value.size());
    std::vector<uint8_t> buf(sizeof(uint32_t) * 3 + data_len);

    std::size_t pos = sizeof(uint32_t);
    std::memcpy(buf.data() + pos, &key_len, sizeof(uint32_t));
    pos += sizeof(uint32_t);

    std::memcpy(buf.data() + pos, &val_len, sizeof(uint32_t));
    pos += sizeof(uint32_t);

    std::memcpy(buf.data() + pos, key.data(), key.size());
    pos += key.size();

    if (!is_delete) {
        std::memcpy(buf.data() + pos, value.data(), value.size());
    }

    uint32_t crc = crc32c(std::span<const uint8_t>(buf.data() + sizeof(uint32_t),
                                                   buf.size() - sizeof(uint32_t)));
    std::memcpy(buf.data(), &crc, sizeof(uint32_t));

    const uint8_t* ptr = buf.data();
    std::size_t rem = buf.size();
    while (rem > 0) {
        ssize_t n = ::write(fd_, ptr, rem);
        if (n <= 0) {
            throw std::runtime_error("WAL: write failed");
        }
        ptr += n;
        rem -= static_cast<std::size_t>(n);
    }

    if (sync_writes_) {
        ::fsync(fd_);
    }
}

void WAL::append(std::string_view key, std::string_view value) {
    std::lock_guard lock(mu_);
    write_record(key, value, false);
}

void WAL::remove(std::string_view key) {
    std::lock_guard lock(mu_);
    write_record(key, {}, true);
}

void WAL::sync() {
    std::lock_guard lock(mu_);
    ::fsync(fd_);
}

void WAL::close() {
    std::lock_guard lock(mu_);
    if (fd_ >= 0) {
        ::fsync(fd_);
        ::close(fd_);
        fd_ = -1;
    }
}

std::vector<LogEntry> WAL::recover() {
    int rfd = ::open(path_.c_str(), O_RDONLY);
    if (rfd < 0)
        return {};

    std::vector<LogEntry> entries;

    while (true) {
        std::array<uint8_t, sizeof(uint32_t) * 3> hdr;
        ssize_t n = ::read(rfd, hdr.data(), hdr.size());
        if (n == 0) break;
        if (n != static_cast<ssize_t>(hdr.size())) break;

        uint32_t stored_crc, key_len, val_len;
        std::memcpy(&stored_crc, hdr.data(), sizeof(uint32_t));
        std::memcpy(&key_len, hdr.data() + sizeof(uint32_t), sizeof(uint32_t));
        std::memcpy(&val_len, hdr.data() + sizeof(uint32_t) * 2, sizeof(uint32_t));

        bool is_delete = (val_len == kDeleteSentinel);
        std::size_t data_len = key_len + (is_delete ? 0 : val_len);

        std::vector<uint8_t> payload(data_len);
        if (::read(rfd, payload.data(), data_len) != static_cast<ssize_t>(data_len)) break;

        std::vector<uint8_t> crc_data(sizeof(uint32_t) * 2 + data_len);
        std::memcpy(crc_data.data(), hdr.data() + sizeof(uint32_t), sizeof(uint32_t) * 2);
        std::memcpy(crc_data.data() + sizeof(uint32_t) * 2, payload.data(), data_len);

        uint32_t computed = crc32c(crc_data);
        if (computed != stored_crc) break;

        std::string key(reinterpret_cast<char*>(payload.data()), key_len);
        std::string value;
        if (!is_delete)
            value.assign(reinterpret_cast<char*>(payload.data() + key_len), val_len);

        entries.push_back({std::move(key), std::move(value), is_delete});
    }

    ::close(rfd);
    return entries;
}

} // namespace lsm