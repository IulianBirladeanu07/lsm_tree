#include "lsm/memtable/concurrent_skip_list.h"
#include <cassert>
#include <iostream>

int main() {
    lsm::ConcurrentSkipList sl;
    sl.put("a", "1");
    assert(sl.get("a") == "1");

    lsm::ConcurrentSkipList skiplist;
    skiplist.put("b", "2");
    skiplist.put("c", "3");
    skiplist.put("d", "4");
    skiplist.put("e", "5");
    skiplist.put("f", "6");
    skiplist.put("g", "7");
    skiplist.put("h", "8");

    assert(skiplist.get("b") == "2");
    assert(skiplist.get("e") == "5");

    assert(skiplist.contains("c"));
    assert(!skiplist.contains("x"));

    std::cout << "all tests passed\n";
    return 0;
}