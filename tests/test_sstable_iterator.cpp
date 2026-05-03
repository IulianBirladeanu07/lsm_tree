#include "lsm/iterator/sstable_iterator.h"
#include "lsm/sstable/sstable_builder.h"
#include "lsm/sstable/sstable.h"
#include <cassert>
#include <iostream>
#include <filesystem>

int main() {
    std::filesystem::path path = "/tmp/test_iter.sst";

    {
        lsm::SSTableBuilder builder(path, 4096, 100, 0.01);
        builder.add("a", "1");
        builder.add("b", "2");
        builder.add("c", "3");
        builder.add("d", "4");
        builder.add("e", "5");
        builder.finish();
    }

    auto sst = lsm::SSTable::open(path);
    lsm::SSTableIterator it(sst);

    std::vector<std::string> keys;
    while (it.valid()) {
        keys.push_back(std::string(it.key()));
        it.next();
    }

    assert(keys.size() == 5);
    assert(keys[0] == "a");
    assert(keys[4] == "e");
    std::cout << "iteration: OK\n";

    lsm::SSTableIterator it2(sst);
    it2.seek("c");
    assert(it2.valid());
    assert(it2.key() == "c");
    std::cout << "seek: OK\n";

    std::filesystem::remove(path);
    std::cout << "all tests passed\n";
    return 0;
}