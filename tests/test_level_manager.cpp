#include "lsm/levels/level_manager.h"
#include "lsm/sstable/sstable_builder.h"
#include "lsm/sstable/sstable.h"
#include <cassert>
#include <iostream>
#include <filesystem>

auto make_sst(const std::string& path) {
    lsm::SSTableBuilder builder(path, 4096, 10, 0.01);
    builder.add("key", "val");
    auto sst = builder.finish();
    return std::make_shared<lsm::SSTable>(std::move(sst));
}

int main() {
    lsm::LevelManager manager(3);

    auto sst1 = make_sst("/tmp/sst1.sst");
    auto sst2 = make_sst("/tmp/sst2.sst");
    auto sst3 = make_sst("/tmp/sst3.sst");

    manager.add_sstable(0, sst1);
    manager.add_sstable(0, sst2);
    manager.add_sstable(1, sst3);

    assert(manager.get_level(0).size() == 2);
    assert(manager.get_level(1).size() == 1);
    assert(manager.num_levels() == 3);

    std::cout << "add_sstable: OK\n";

    auto sst4 = make_sst("/tmp/sst4.sst");
    manager.replace_level({sst1, sst2}, {sst4}, 2);

    assert(manager.get_level(0).empty());
    assert(manager.get_level(1).size() == 1);
    assert(manager.get_level(2).size() == 1);

    std::cout << "replace_level: OK\n";
    std::cout << "all tests passed\n";

    std::filesystem::remove("/tmp/sst1.sst");
    std::filesystem::remove("/tmp/sst2.sst");
    std::filesystem::remove("/tmp/sst3.sst");
    std::filesystem::remove("/tmp/sst4.sst");

    return 0;
}