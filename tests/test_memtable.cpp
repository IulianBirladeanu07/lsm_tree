#include "lsm/memtable/memtable.h"
#include <iostream>
#include <cassert>

int main() {
    lsm::MemTable memtable(1024);

    memtable.put("alpha", "1");
    memtable.put("beta", "2");
    memtable.put("gamma", "3");

    auto r1 = memtable.get("alpha");
    auto r2 = memtable.get("gamma");
    auto r3 = memtable.get("missing");

    assert(r1 && *r1 == "1");
    assert(r2 && *r2 == "3");
    assert(!r3);

    std::cout << "basic get: OK\n";

    assert(memtable.contains("beta"));
    assert(!memtable.contains("delta"));

    std::cout << "contains: OK\n";

    std::cout << "is_full: " << memtable.is_full() << "\n";
    std::cout << "is_mutable: " << memtable.is_mutable() << "\n";

    std::cout << "all tests passed\n";
    return 0;
}