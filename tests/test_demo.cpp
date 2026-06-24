#include "lsm/lsm_tree.h"
#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <fstream>
#include <cstring>

static const std::string DB_PATH = "./demo_data";

void separator(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

void dump_wal_hex(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) { std::cout << "  Nu pot deschide WAL\n"; return; }

    f.seekg(0, std::ios::end);
    size_t file_size = f.tellg();
    f.seekg(0, std::ios::beg);

    size_t to_read = std::min(file_size, size_t(128));
    std::vector<uint8_t> buf(to_read);
    f.read(reinterpret_cast<char*>(buf.data()), to_read);

    std::cout << "\nContiut brut WAL (primii " << to_read << " bytes in hex):\n";
    for (size_t i = 0; i < to_read; i++) {
        if (i % 16 == 0) std::cout << std::format("\n  {:04x}: ", i);
        std::cout << std::format("{:02x} ", buf[i]);
    }
    std::cout << "\n";
}

void dump_wal_parsed(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return;

    std::cout << "\nAcelasi WAL, interpretat:\n";
    std::cout << "  idx | tip     | cheie            | valoare\n";
    std::cout << "  ----|---------|------------------|------------------\n";

    int idx = 0;
    while (true) {
        uint32_t crc, key_len, val_len;
        if (!f.read(reinterpret_cast<char*>(&crc),     4)) break;
        if (!f.read(reinterpret_cast<char*>(&key_len), 4)) break;
        if (!f.read(reinterpret_cast<char*>(&val_len), 4)) break;

        std::string key(key_len, '\0');
        f.read(key.data(), key_len);

        bool is_delete = (val_len == 0xFFFFFFFF);
        std::string value;
        if (!is_delete) {
            value.resize(val_len);
            f.read(value.data(), val_len);
            if (value.size() > 20) value = value.substr(0, 20) + "...";
        }

        std::cout << std::format("  {:>3} | {:<7} | {:<16} | {}\n",
            idx++,
            is_delete ? "DELETE" : "PUT",
            key,
            is_delete ? "[tombstone 0xFFFFFFFF]" : value);
    }

    std::cout << "\n  Structura unei inregistrari:\n";
    std::cout << "  [4 bytes CRC32c] [4 bytes key_len] [4 bytes val_len] [key] [value]\n";
    std::cout << "  DELETE: val_len = 0xFFFFFFFF, campul value lipseste\n";
}

void demo_basic_ops() {
    separator("1. Operatii de baza: put, get, del");

    lsm::LSMOptions opts;
    opts.memtable_size = 64 * 1024 * 1024;
    opts.sync_writes   = true;

    lsm::LSMTree db(DB_PATH, opts);

    db.put("user:1001", "Alice");
    db.put("user:1002", "Bob");
    db.put("user:1003", "Carol");
    std::cout << "put: user:1001 -> Alice\n";
    std::cout << "put: user:1002 -> Bob\n";
    std::cout << "put: user:1003 -> Carol\n";

    auto v = db.get("user:1002");
    std::cout << "get user:1002 -> " << (v ? *v : "nullopt") << "\n";

    db.del("user:1001");
    std::cout << "del user:1001\n";

    auto deleted = db.get("user:1001");
    std::cout << "get user:1001 dupa del -> " << (deleted ? *deleted : "nullopt") << "\n";

    std::cout << "\nFisiere pe disc dupa operatii:\n";
    for (const auto& entry : std::filesystem::directory_iterator(DB_PATH))
        std::cout << "  " << entry.path().filename() << "  ("
                  << entry.file_size() << " bytes)\n";

    dump_wal_hex(DB_PATH + "/wal.log");
    dump_wal_parsed(DB_PATH + "/wal.log");
}

void demo_scan() {
    separator("2. Scan pe interval");

    lsm::LSMOptions opts;
    opts.memtable_size = 64 * 1024 * 1024;

    lsm::LSMTree db(DB_PATH, opts);

    for (int i = 1; i <= 10; i++)
        db.put("product:" + std::to_string(i), "value_" + std::to_string(i * 100));

    std::cout << "Inserate product:1 .. product:10\n";

    auto results = db.scan("product:3", "product:7");
    std::cout << "scan(product:3, product:7) -> " << results.size() << " rezultate:\n";
    for (const auto& [k, v] : results)
        std::cout << "  " << k << " -> " << v << "\n";
}

void demo_flush_visible() {
    separator("3. Flush vizibil pe disc");

    std::filesystem::remove_all("./demo_flush");

    {
        lsm::LSMOptions opts;
        opts.memtable_size = 64 * 1024;

        lsm::LSMTree db("./demo_flush", opts);

        std::cout << "MemTable size: 64KB — scriem date pana la flush...\n";

        int i = 0;
        while (true) {
            std::string key   = "key_" + std::string(20 - std::to_string(i).size(), '0') + std::to_string(i);
            std::string value = std::string(100, 'x');
            db.put(key, value);
            i++;

            bool has_sst = false;
            for (const auto& e : std::filesystem::directory_iterator("./demo_flush"))
                if (e.path().extension() == ".sst") { has_sst = true; break; }

            if (has_sst) {
                std::cout << "Dupa " << i << " scrieri, fisiere pe disc:\n";
                for (const auto& e : std::filesystem::directory_iterator("./demo_flush"))
                    std::cout << "  " << e.path().filename() << "  (" << e.file_size() << " bytes)\n";
                std::cout << "\nMemTable-ul a atins 64KB — flush asincron declansat.\n";
                std::cout << "sst_000000.sst contine toate cheile sortate lexicografic.\n";
                std::cout << "wal.log va fi trunchiat dupa ce flush-ul se confirma.\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                break;
            }
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::filesystem::remove_all("./demo_flush");
}

void demo_wal_recovery() {
    separator("4. Recuperare WAL dupa crash simulat");

    std::filesystem::remove_all("./demo_wal");

    {
        lsm::LSMOptions opts;
        opts.memtable_size = 64 * 1024 * 1024;
        opts.sync_writes   = true;

        lsm::LSMTree db("./demo_wal", opts);
        db.put("session:abc", "user_42");
        db.put("session:xyz", "user_99");
        std::cout << "Scris in WAL: session:abc -> user_42\n";
        std::cout << "Scris in WAL: session:xyz -> user_99\n";
        std::cout << "Simulam crash — obiectul db e distrus fara flush\n";
    }

    std::cout << "\nFisiere ramase pe disc:\n";
    for (const auto& e : std::filesystem::directory_iterator("./demo_wal"))
        std::cout << "  " << e.path().filename() << "  (" << e.file_size() << " bytes)\n";

    dump_wal_hex("./demo_wal/wal.log");
    dump_wal_parsed("./demo_wal/wal.log");

    std::cout << "\nRepornire — LSMTree reconstruieste din WAL:\n";
    {
        lsm::LSMOptions opts;
        opts.memtable_size = 64 * 1024 * 1024;

        lsm::LSMTree db("./demo_wal", opts);

        auto v1 = db.get("session:abc");
        auto v2 = db.get("session:xyz");
        std::cout << "get session:abc -> " << (v1 ? *v1 : "nullopt") << "\n";
        std::cout << "get session:xyz -> " << (v2 ? *v2 : "nullopt") << "\n";
    }

    std::filesystem::remove_all("./demo_wal");
}

int main() {
    std::filesystem::remove_all(DB_PATH);

    demo_basic_ops();
    demo_scan();
    demo_flush_visible();
    demo_wal_recovery();

    std::filesystem::remove_all(DB_PATH);

    std::cout << "\n=== Demo complet ===\n";
    return 0;
}