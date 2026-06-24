#include "lsm/lsm_tree.h"
#include <iostream>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

namespace color {
    constexpr const char* reset   = "\033[0m";
    constexpr const char* bold    = "\033[1m";
    constexpr const char* green   = "\033[32m";
    constexpr const char* red     = "\033[31m";
    constexpr const char* yellow  = "\033[33m";
    constexpr const char* cyan    = "\033[36m";
    constexpr const char* magenta = "\033[35m";
    constexpr const char* white   = "\033[37m";
}

static void separator(const char* title) {
    std::cout << "\n" << color::bold << color::cyan
              << "--- " << title << " ---"
              << color::reset << "\n";
}

static void ok(const std::string& key, const std::string& val) {
    std::cout << color::green << "  [OK]  " << color::reset
              << color::white << key << color::reset
              << " -> "
              << color::green << val << color::reset << "\n";
}

static void not_found(const std::string& key) {
    std::cout << color::red << "  [--]  " << color::reset
              << color::white << key << color::reset
              << " -> "
              << color::red << "NOT FOUND" << color::reset << "\n";
}

static void tombstone(const std::string& key) {
    std::cout << color::yellow << "  [DEL] " << color::reset
              << color::white << key << color::reset
              << " -> "
              << color::yellow << "TOMBSTONE (sters logic)" << color::reset << "\n";
}

static void put_log(const std::string& key, const std::string& val) {
    std::cout << color::magenta << "  [PUT] " << color::reset
              << color::white << key << color::reset
              << " -> "
              << color::green << val << color::reset << "\n";
}

int main() {
    const fs::path db_path = "/tmp/demo_db";
    fs::remove_all(db_path);

    lsm::LSMOptions opts;
    opts.memtable_size    = 4 * 1024 * 1024;
    opts.compaction_style = lsm::LSMOptions::CompactionStyle::Leveled;

    std::cout << color::bold << color::cyan
              << "\n+======================================+"
              << "\n|       LSM Tree -- Demo Functional    |"
              << "\n+======================================+"
              << color::reset << "\n";

    lsm::LSMTree tree(db_path, opts);

    separator("PUT -- scriem 5 chei");
    for (int i = 1; i <= 5; i++) {
        std::string key = "key_00" + std::to_string(i);
        std::string val = "valoare_" + std::to_string(i);
        tree.put(key, val);
        put_log(key, val);
    }

    separator("GET -- citim cateva chei");
    for (auto& k : {"key_001", "key_003", "key_005", "key_999"}) {
        auto v = tree.get(k);
        if (v) ok(k, *v);
        else   not_found(k);
    }

    separator("DELETE -- stergem key_003");
    tree.del("key_003");
    tombstone("key_003");
    auto after_del = tree.get("key_003");
    if (after_del) ok("key_003", *after_del);
    else           not_found("key_003");

    separator("UPDATE -- suprascriem key_001");
    tree.put("key_001", "valoare_ACTUALIZATA");
    put_log("key_001", "valoare_ACTUALIZATA");
    auto updated = tree.get("key_001");
    if (updated) ok("key_001", *updated);
    else         not_found("key_001");

    separator("SCAN [key_001, key_005]");
    auto results = tree.scan("key_001", "key_005");
    std::cout << "  " << color::cyan << results.size()
              << " rezultate" << color::reset << " (key_003 filtrat -- tombstone):\n";
    for (auto& kv : results)
        ok(kv.key, kv.value);

    separator("FISIERE PE DISC");
    std::cout << "  " << color::cyan << db_path.string() << color::reset << ":\n";
    for (auto& entry : fs::directory_iterator(db_path))
        std::cout << color::yellow << "    " << entry.path().filename().string()
                  << color::reset << "  (" << entry.file_size() << " bytes)\n";

    std::cout << "\n  " << color::cyan << "WAL hex dump:" << color::reset << "\n";
    (void)std::system("xxd /tmp/demo_db/wal.log | head -5");

    std::cout << "\n" << color::bold << color::green
              << "  Demo finalizat." << color::reset << "\n\n";
    return 0;
}