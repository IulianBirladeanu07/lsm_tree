#include <iostream>
#include "lsm/sstable/block_builder.h"

int main() {
    lsm::BlockBuilder builder(4096);

    builder.add("apple", "fruit");
    builder.add("banana", "yellow");
    builder.add("cherry", "red");

    std::cout << "entries added: 3\n";
    std::cout << "current size: " << builder.current_size() << " bytes\n";
    std::cout << "is full: " << (builder.is_full() ? "yes" : "no") << "\n";

    auto data = builder.finish();
    std::cout << "finished block size: " << data.size() << " bytes\n";

    // print raw bytes ca hex
    std::cout << "first 32 bytes: ";
    for (int i = 0; i < 32 && i < (int)data.size(); i++) {
        printf("%02x ", data[i]);
    }
    std::cout << "\n";

    return 0;
}