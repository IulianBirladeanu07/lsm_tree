#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace lsm {

class Block {
public:
    explicit Block(std::vector<uint8_t> data);
    uint32_t read_uint32(std::size_t offset) const;

    std::optional<std::string> get(std::string_view key) const;
    std::size_t                size() const;

private:
    bool key_matches(std::size_t offset, std::string_view key) const;

    std::vector<uint8_t>         data_;
    static constexpr std::size_t kHeaderSize = sizeof(uint32_t) * 2;

};

} // namespace lsm