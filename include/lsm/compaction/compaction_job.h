#pragma once
#include <vector>
#include <memory>
#include <string>

namespace lsm {

class SSTable;

struct CompactionJob {
    std::vector<std::shared_ptr<SSTable>> inputs;
    int output_level;
    std::string output_path;
};

} // namespace lsm