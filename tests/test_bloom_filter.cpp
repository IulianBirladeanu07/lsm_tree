#include <cassert>
#include <iostream>
#include "lsm/util/bloom_filter.h"

int main() {
    lsm::BloomFilter bf(100, 0.01);

    bf.add("foo");
    bf.add("bar");
    bf.add("baz");

    std::cout << "may_contain(foo): " << bf.may_contain("foo") << "\n";
    std::cout << "may_contain(bar): " << bf.may_contain("bar") << "\n";
    std::cout << "may_contain(baz): " << bf.may_contain("baz") << "\n";
    std::cout << "may_contain(xyz): " << bf.may_contain("xyz") << "\n";
    std::cout << "may_contain(qwe): " << bf.may_contain("qwe") << "\n";

    auto bytes = bf.serialize();
    auto bf2 = lsm::BloomFilter::deserialize(bytes);

    std::cout << "\nafter serialize/deserialize:\n";
    std::cout << "may_contain(foo): " << bf2.may_contain("foo") << "\n";
    std::cout << "may_contain(bar): " << bf2.may_contain("bar") << "\n";
    std::cout << "may_contain(xyz): " << bf2.may_contain("xyz") << "\n";

    assert(bf.may_contain("foo"));
    assert(bf.may_contain("bar"));
    assert(bf.may_contain("baz"));
    assert(!bf.may_contain("xyz"));
    assert(bf2.may_contain("foo"));
    assert(!bf2.may_contain("xyz"));

    std::cout << "\nall tests passed\n";
    return 0;
}