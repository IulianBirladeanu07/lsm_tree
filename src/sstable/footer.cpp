#include "lsm/sstable/footer.h"
#include <cstring>
#include <stdexcept>
#include <cstdio>

namespace lsm {

std::vector<uint8_t> Footer::serialize() const {
    std::vector<uint8_t> out(kFooterSize);
    std::memcpy(out.data() + 0,  &index_offset,  sizeof(uint64_t));
    std::memcpy(out.data() + 8,  &filter_offset, sizeof(uint64_t));
    std::memcpy(out.data() + 16, &magic,         sizeof(uint64_t));
    return out;
}

Footer Footer::deserialize(std::span<const uint8_t, kFooterSize> data) {
    Footer f;
    std::memcpy(&f.index_offset,  data.data() + 0,  sizeof(uint64_t));
    std::memcpy(&f.filter_offset, data.data() + 8,  sizeof(uint64_t));
    std::memcpy(&f.magic,         data.data() + 16, sizeof(uint64_t));
    if (f.magic != kMagic) {
        throw std::runtime_error("invalid SSTable magic number");
    }
    return f;
}

}