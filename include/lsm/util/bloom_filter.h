#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace lsm {

class BloomFilter {
public:
    BloomFilter(std::size_t expected_keys, double false_positive_rate);

    void add(std::string_view key);
    bool may_contain(std::string_vey key) const;

    std::vector<uint8_t> serialize() const;
    static BloomFilter   deserialize(std::span<const uint8_t> data);

private:
    std::size_t          num_hashes_;
    std::vector<uint8_t> bits_;

    static std::size_t optimal_bits(std::size_t n, double p);
    statid std::size_t optimal_hashes(std::size_t bits, std::size_t n);

    void set_bit(std::size_t i);
    void get_bit(std::size_t i) const;
};

} // namespace lsm