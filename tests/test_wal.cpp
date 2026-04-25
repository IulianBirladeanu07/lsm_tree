#include <cassert>
#include <iostream>
#include <filesystem>
#include "lsm/wal/wal.h"

int main() {
    std::filesystem::path path = "/tmp/test.wal";
    std::filesystem::remove(path);

    {
        lsm::WAL wal(path);
        wal.append("alpha", "1");
        wal.append("beta", "2");
        wal.append("gamma", "3");
        wal.remove("beta");
        wal.close();
    }

    {
        lsm::WAL wal(path);
        auto entries = wal.recover();

        assert(entries.size() == 4);

        assert(entries[0].key == "alpha" && entries[0].value == "1" && !entries[0].is_delete);
        assert(entries[1].key == "beta"  && entries[1].value == "2" && !entries[1].is_delete);
        assert(entries[2].key == "gamma" && entries[2].value == "3" && !entries[2].is_delete);
        assert(entries[3].key == "beta"  && entries[3].is_delete);

        std::cout << "recover: OK\n";

        wal.append("delta", "4");
        wal.close();
    }

    {
        lsm::WAL wal(path);
        auto entries = wal.recover();

        assert(entries.size() == 5);
        assert(entries[4].key == "delta" && entries[4].value == "4");

        std::cout << "append after recover: OK\n";
        wal.close();
    }

    std::filesystem::remove(path);
    std::cout << "all tests passed\n";
    return 0;
}