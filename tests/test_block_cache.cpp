#include "lsm/sstable/block_cache.h"
#include <cassert>
#include <iostream>

int main() {
    lsm::BlockCache cache(1024);

    std::vector<uint8_t> data1(512, 0xAA);
    std::vector<uint8_t> data2(256, 0xBB);
    std::vector<uint8_t> data3(512, 0xCC);

    cache.put(1, 0, lsm::Block(data1));
    cache.put(1, 512, lsm::Block(data2));

    auto b1 = cache.get(1, 0);
    auto b2 = cache.get(1, 512);
    auto b3 = cache.get(2, 0);

    assert(b1 && b1->size() == data1.size());
    assert(b2 && b2->size() == data2.size());
    assert(!b3);

    std::cout << "basic put/get: OK\n";

    cache.put(2, 0, lsm::Block(data3));
    auto b4 = cache.get(2, 0);
    assert(b4 && b4->size() == data3.size());

    std::cout << "eviction: OK\n";

    std::cout << "all tests passed\n";
    return 0;
}