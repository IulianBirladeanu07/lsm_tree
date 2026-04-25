#include <cassert>
#include <iostream>
#include <filesystem>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include "lsm/sstable/sstable_builder.h"
#include "lsm/sstable/sstable.h"

int main() {
    std::filesystem::path path = "/tmp/test.sst";

    {
        lsm::SSTableBuilder builder(path, 4096, 100, 0.01);
        builder.add("alpha",   "1");
        builder.add("beta",    "2");
        builder.add("delta",   "4");
        builder.add("epsilon", "5");
        builder.add("gamma",   "3");

        auto sst = builder.finish();

        auto r1 = sst.get("alpha");
        auto r2 = sst.get("gamma");
        auto r3 = sst.get("missing");

        assert(r1 && *r1 == "1");
        assert(r2 && *r2 == "3");
        assert(!r3);

        std::cout << "basic get: OK\n";
        std::cout << "file_size: " << sst.file_size() << "\n";
        std::cout << "smallest:  " << sst.smallest_key() << "\n";
        std::cout << "largest:   " << sst.largest_key() << "\n";
    }

    {
        auto sst = lsm::SSTable::open(path);

        auto r1 = sst.get("beta");
        auto r2 = sst.get("epsilon");
        auto r3 = sst.get("zzz");

        assert(r1 && *r1 == "2");
        assert(r2 && *r2 == "5");
        assert(!r3);

        std::cout << "reopen + get: OK\n";
    }

    std::filesystem::remove(path);
    std::cout << "all tests passed\n";
    return 0;
}