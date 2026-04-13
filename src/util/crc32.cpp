#include "lsm/util/crc32.h"

#ifdef __SSE4_2__
#include <nmmintrin.h>
#endif

namespace lsm {

uint32_t crc32c(std::span<const uint8_t> data) {
#ifdef __SSE4_2__
    uint32_t crc = 0xFFFFFFFF;
    for (auto byte : data) {
        crc = _mm_crc32_u8(crc, byte);
    }
    return crc ^ 0xFFFFFFFF;
#else
    return 0;
#endif
}

uint32_t crc32c_extend(uint32_t crc, std::span<const uint8_t> data) {
#ifdef __SSE4_2__
    for (auto byte : data) {
        crc = _mm_crc32_u8(crc, byte);
    }
    return crc;
#else
    return 0;
#endif
}

} // namespace lsm