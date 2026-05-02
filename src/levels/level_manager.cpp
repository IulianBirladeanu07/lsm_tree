#include "lsm/levels/level_manager.h"
#include <mutex>
#include <algorithm>

namespace lsm {

LevelManager::LevelManager(int levels) : levels_(levels) {}

LevelManager::~LevelManager() {}

void LevelManager::add_sstable(int level, std::shared_ptr<SSTable> sstable) {
    std::unique_lock lock(mutex_);
    if (level >= 0 && static_cast<size_t>(level) < levels_.size()) {
        levels_[level].push_back(sstable);
    }
}

void LevelManager::replace_level(std::vector<std::shared_ptr<SSTable>> inputs,
                                 std::vector<std::shared_ptr<SSTable>> outputs,
                                 int output_level) {
    std::unique_lock lock(mutex_);
    if (output_level >= 0 && static_cast<size_t>(output_level) < levels_.size()) {
        // Remove input SSTables from all levels
        for (const auto& sstable : inputs) {
            for (auto& level : levels_) {
                level.erase(std::remove(level.begin(), level.end(), sstable), level.end());
            }
        }
        // Add output SSTables to the specified level
        levels_[output_level].insert(levels_[output_level].end(), outputs.begin(), outputs.end());
    }
}

std::vector<std::shared_ptr<SSTable>> LevelManager::get_level(int level) const {
    std::shared_lock lock(mutex_);
    if (level >= 0 && static_cast<size_t>(level) < levels_.size()) {
        return levels_[level];
    }
    return {};
}

int LevelManager::num_levels() const {
    return static_cast<int>(levels_.size());
}

} // namespace lsm