#pragma once
#include "lsm/iterator/iterator.h"
#include "lsm/sstable/block.h"
#include <cstdint>

namespace lsm {

class BlockIterator : public IIterator {
public:
    explicit BlockIterator(const Block& block);

    bool valid() const override;
    void next() override;
    std::string_view key() const override;
    std::string_view value() const override;
    void seek(std::string_view key) override;

private:
    void parse_current();

    const Block& block_;
    std::size_t offset_;
    std::string current_key_;
    std::string current_value_;
    bool valid_;
};

} // namespace lsm