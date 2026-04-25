#pragma once
#include <cstdint>
#include <span>
#include <vector>

namespace lsm {

static constexpr uint64_t kMagic = 0x4C534D545245454BULL;
static constexpr std::size_t kFooterSize = 24;

struct Footer {
    uint64_t index_offset;
    uint64_t filter_offset;
    uint64_t magic;

    std::vector<uint8_t> serialize() const;
    static Footer deserialize(std::span<const uint8_t, kFooterSize> data);
};

} // namespace lsm