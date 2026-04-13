#include "lsm/util/bloom_filter.h"
#include <cmath>
#include <functional>
#include <cstring>

namespace lsm {

BloomFilter::BloomFilter(std::size_t expected_keys, double false_positive_rate) {
    auto num_bits = optimal_bits(expected_keys, false_positive_rate);
    num_hashes_ = optimal_hashes(num_bits, expected_keys);
    bits_.resize((num_bits + 7) / 8);
}

BloomFilter::BloomFilter(std::size_t num_hashes, std::vector<uint8_t> bits)
    : num_hashes_(num_hashes), bits_(std::move(bits)) {}

std::size_t BloomFilter::optimal_bits(std::size_t n, double p) {
    double bits = -static_cast<double>(n) * std::log(p) / (kLn2 * kLn2);
    return static_cast<std::size_t>(bits);
}

std::size_t BloomFilter::optimal_hashes(std::size_t bits, std::size_t n) {
    double hashes = (static_cast<double>(bits) / static_cast<double>(n)) * kLn2;
    return static_cast<std::size_t>(hashes);
}

void BloomFilter::set_bit(std::size_t i) {
    bits_[i / 8] |= (uint8_t(1) << (i % 8));
}

bool BloomFilter::get_bit(std::size_t i) const {
    return bits_[i / 8] & (1 << (i % 8));
}

void BloomFilter::add(std::string_view key) {
    auto hash1 = std::hash<std::string_view>{}(key);
    auto hash2 = hash1 ^ kFibHashConstant;
    auto num_bits = bits_.size() * 8;

    for (std::size_t i = 0; i < num_hashes_; i++) {
        auto bit_pos = (hash1 + i * hash2) % num_bits;
        set_bit(bit_pos);
    }
}

bool BloomFilter::may_contain(std::string_view key) const {
    auto hash1 = std::hash<std::string_view>{}(key);
    auto hash2 = hash1 ^ kFibHashConstant;
    auto num_bits = bits_.size() * 8;

    for (std::size_t i = 0; i < num_hashes_; i++) {
        auto bit_pos = (hash1 + i * hash2) % num_bits;
        if (!get_bit(bit_pos)) return false;
    }
    return true;
}

std::vector<uint8_t> BloomFilter::serialize() const {
    std::vector<uint8_t> out(sizeof(num_hashes_) + bits_.size());
    std::memcpy(out.data(), &num_hashes_, sizeof(num_hashes_));
    std::memcpy(out.data() + sizeof(num_hashes_), bits_.data(), bits_.size());
    return out;
}

BloomFilter BloomFilter::deserialize(std::span<const uint8_t> data) {
    std::size_t num_hashes;
    std::memcpy(&num_hashes, data.data(), sizeof(num_hashes));
    std::vector<uint8_t> bits(data.begin() + sizeof(num_hashes), data.end());
    return BloomFilter(num_hashes, std::move(bits));
}

} // namespace lsm