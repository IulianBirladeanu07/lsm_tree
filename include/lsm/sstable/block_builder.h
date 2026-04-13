#pragma once
#include <cstdint>
#include <string_view>
#include <vector>

namespace lsm {

class BlockBuilder {
public:
    explicit BlockBuilder(std::size_t block_size);

    void                     add(std::string_view key, std::string_view value);
    std::vector<uint8_t>     finish();
    bool                     is_full() const;
    std::size_t              current_size() const;
    void                     reset();

private:
    void append_uint32(uint32_t value);
    void append_bytes(std::string_view data);

    std::vector<uint8_t>  buffer_;
    std::size_t           block_size_;
    std::size_t           num_entries_{0};
};

} // namespace lsm