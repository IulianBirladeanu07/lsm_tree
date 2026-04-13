#pragma once
#include <cstdint>
#include <span>

namespace lsm {

uint32_t crc32c(std::span<const uint8_t> data);
uint32_t crc32c_extend(uint32_t crc, std::span<const uint8_t> data);

} // namespace lsm