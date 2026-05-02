#pragma once
#include <vector>
#include <memory>
#include <shared_mutex>
#include "lsm/sstable/sstable.h"

namespace lsm {
class LevelManager {
public:
    LevelManager(int levels);
    ~LevelManager();

    void add_sstable(int level, std::shared_ptr<SSTable> sstable);
    void replace_level(std::vector<std::shared_ptr<SSTable>> inputs,
                       std::vector<std::shared_ptr<SSTable>> outputs,
                       int output_level);
    std::vector<std::shared_ptr<SSTable>> get_level(int level) const;
    int num_levels() const;

private:
    std::vector<std::vector<std::shared_ptr<SSTable>>> levels_;
    mutable std::shared_mutex mutex_;
};

} // namespace lsm