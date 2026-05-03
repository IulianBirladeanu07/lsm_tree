#include "lsm/iterator/block_iterator.h"
#include "lsm/sstable/block_builder.h"
#include "lsm/sstable/block.h"
#include <cassert>
#include <iostream>

int main() {
    lsm::BlockBuilder builder(4096);
    builder.add("a", "1");
    builder.add("b", "2");
    builder.add("c", "3");
    builder.add("d", "4");
    auto data = builder.finish();
    lsm::Block block(std::move(data));

    lsm::BlockIterator it(block);
    assert(it.valid());
    assert(it.key() == "a");
    assert(it.value() == "1");
    it.next();
    assert(it.key() == "b");
    it.next();
    assert(it.key() == "c");
    it.next();
    assert(it.key() == "d");
    it.next();
    assert(!it.valid());
    std::cout << "iteration: OK\n";

    lsm::BlockIterator it2(block);
    it2.seek("c");
    assert(it2.valid());
    assert(it2.key() == "c");
    std::cout << "seek: OK\n";

    lsm::BlockIterator it3(block);
    it3.seek("z");
    assert(!it3.valid());
    std::cout << "seek past end: OK\n";

    std::cout << "all tests passed\n";
    return 0;
}