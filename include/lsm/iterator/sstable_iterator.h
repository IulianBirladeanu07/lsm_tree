#pragma once
#include "lsm/iterator/iterator.h"
#include "lsm/iterator/block_iterator.h"
#include "lsm/sstable/sstable.h"
#include <memory>
#include <optional>

namespace lsm {

class SSTableIterator : public IIterator {
public:
    explicit SSTableIterator(const SSTable& sst);

    bool valid() const override;
    void next() override;
    std::string_view key() const override;
    std::string_view value() const override;
    void seek(std::string_view key) override;

private:
    void load_block(std::size_t block_idx);

    const SSTable& sst_;
    std::size_t block_idx_;
    std::optional<Block> current_block_;
    std::optional<BlockIterator> block_iter_;
};

} // namespace lsm