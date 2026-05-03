#pragma once
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace lsm {

struct LogEntry {
    std::string key;
    std::string value;
    bool is_delete;
};

class WAL {
public:
    explicit WAL(std::filesystem::path path, bool sync_writes = false);
    ~WAL();

    void append(std::string_view key, std::string_view value);
    void remove(std::string_view key);
    std::vector<LogEntry> recover();
    void truncate();
    void sync();
    void close();

private:
    void write_record(std::string_view key, std::string_view value, bool is_delete);

    int fd_{-1};
    std::filesystem::path path_;
    bool sync_writes_;
    std::mutex mu_;

    static constexpr uint32_t kDeleteSentinel = 0xFFFFFFFF;
};

} // namespace lsm