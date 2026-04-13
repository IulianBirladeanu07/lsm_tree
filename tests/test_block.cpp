#include <iostream>
#include "lsm/sstable/block_builder.h"
#include "lsm/sstable/block.h"

int main() {
    lsm::BlockBuilder builder(4096);
    builder.add("apple", "fruit");
    builder.add("banana", "yellow");
    builder.add("cherry", "red");

    auto data = builder.finish();
    lsm::Block block(std::move(data));

    auto r1 = block.get("apple");
    auto r2 = block.get("banana");
    auto r3 = block.get("missing");

    std::cout << "apple: "   << (r1 ? *r1 : "NOT FOUND") << "\n";
    std::cout << "banana: "  << (r2 ? *r2 : "NOT FOUND") << "\n";
    std::cout << "missing: " << (r3 ? *r3 : "NOT FOUND") << "\n";

    return 0;
}