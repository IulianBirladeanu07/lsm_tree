#pragma once
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <span>
#include <vector>
#include <string_view>

namespace lsm {

class BloomFilter {
public:
    BloomFilter(std::size_t expected_keys, double false_positive_rate);

    void add(std::string_view key);
    bool may_contain(std::string_view key) const;

    std::vector<uint8_t> serialize() const;
    static BloomFilter   deserialize(std::span<const uint8_t> data);

private:
    BloomFilter(std::size_t num_hashes, std::vector<uint8_t> bits);

    std::size_t             num_hashes_;
    std::vector<uint8_t>    bits_;
    static constexpr double kLn2 = std::log(2.0);
    static constexpr std::size_t kFibHashConstant = 0x9e3779b97f4a7c15ULL;

    static std::size_t optimal_bits(std::size_t n, double p);
    static std::size_t optimal_hashes(std::size_t bits, std::size_t n);

    void set_bit(std::size_t i);
    bool get_bit(std::size_t i) const;
};

} // namespace lsm