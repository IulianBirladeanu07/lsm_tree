#include "lsm/lsm_tree.h"
#include <cassert>
#include <iostream>
#include <filesystem>

int main() {
    std::filesystem::path dir = "/tmp/test_lsm";
    std::filesystem::remove_all(dir);

    {
        lsm::LSMTree tree(dir);

        tree.put("a", "1");
        tree.put("b", "2");
        tree.put("c", "3");

        auto r1 = tree.get("a");
        auto r2 = tree.get("b");
        auto r3 = tree.get("missing");

        assert(r1 && *r1 == "1");
        assert(r2 && *r2 == "2");
        assert(!r3);

        std::cout << "basic put/get: OK\n";

        tree.put("a", "updated");
        auto r4 = tree.get("a");
        assert(r4 && *r4 == "updated");
        std::cout << "update: OK\n";

        tree.del("b");
        auto r5 = tree.get("b");
        assert(!r5 || *r5 == "\xFF");
        std::cout << "delete: OK\n";
    }

    std::filesystem::remove_all(dir);
    std::cout << "all tests passed\n";
    return 0;
}