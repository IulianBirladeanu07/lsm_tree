#include "bloom_filter.h"
#include <cmath>

namespace lsm {

BloomFilter::BloomFilter(std::size_t expected_keys, double false_positive_rate) {
    auto num_bits = optimal_bits(expected_keys, false_positive_rate);
    num_hashes_ = optimal_hashes(num_bits, expected_keys);
    bits_.resize((num_bits + 7) / 8);
}

std::size_t BloomFilter::optimal_bits(std::size_t n, double p) {
    return (std::size_t) (-n * std::log(p) / (kLn2 * kLn2));
}

std::size_t BloomFilter::optimal_hashes(std::size_t bits, std::size_t n) {
    return (std::size_t) ((double)bits / n * kLn2);
}

void BloomFilter::set_bit(std::size_t i) {
    bits_[i / 8] |= (1 << (i % 8));
}

bool BloomFilter::get_bit(std::size_t i) const {
    return bits_[i / 8] & (1 << (i % 8));
}

} // namespace lsm