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
              << color::red << "NOT FOUND -- DATE PIERDUTE!" << color::reset << "\n";
}

static void put_log(const std::string& key, const std::string& val) {
    std::cout << color::magenta << "  [PUT] " << color::reset
              << color::white << key << color::reset
              << " -> "
              << color::green << val << color::reset << "\n";
}

static const fs::path DB_PATH = "/tmp/demo_recovery_db";

static void phase_write() {
    std::cout << color::bold << color::cyan
              << "\n+==========================================+"
              << "\n|  LSM Tree -- Recovery Demo  [FAZA 1/2]  |"
              << "\n|        Scriem date + simulam crash       |"
              << "\n+==========================================+"
              << color::reset << "\n";

    lsm::LSMOptions opts;
    opts.memtable_size = 64 * 1024 * 1024;
    lsm::LSMTree tree(DB_PATH, opts);

    separator("PUT -- scriem 6 chei (raman doar in WAL + MemTable)");
    const char* keys[] = {"alfa", "beta", "gamma", "delta", "epsilon", "zeta"};
    const char* vals[] = {"1111", "2222", "3333",  "4444",  "5555",    "6666"};
    for (int i = 0; i < 6; i++) {
        tree.put(keys[i], vals[i]);
        put_log(keys[i], vals[i]);
    }

    std::cout << "\n" << color::yellow << color::bold
              << "  MemTable-ul NU a atins pragul de flush (64MB).\n"
              << "  Datele exista doar in WAL pe disc si in RAM.\n"
              << "  Simulam crash cu quick_exit() -- destructorul nu se apeleaza,\n"
              << "  niciun SSTable nu e scris."
              << color::reset << "\n\n";

    std::quick_exit(0);
}

static void phase_recover() {
    std::cout << color::bold << color::cyan
              << "\n+==========================================+"
              << "\n|  LSM Tree -- Recovery Demo  [FAZA 2/2]  |"
              << "\n|        Repornire -- recovery din WAL     |"
              << "\n+==========================================+"
              << color::reset << "\n";

    separator("Fisiere pe disc inainte de recovery");
    std::cout << "  " << color::cyan << DB_PATH.string() << color::reset << ":\n";
    for (auto& e : fs::directory_iterator(DB_PATH))
        std::cout << color::yellow << "    " << e.path().filename().string()
                  << color::reset << "  (" << e.file_size() << " bytes)\n";

    std::cout << "\n  " << color::cyan
              << "Niciun SSTable -- datele sunt doar in WAL."
              << color::reset << "\n";

    std::cout << "\n  " << color::cyan << "WAL hex dump:" << color::reset << "\n";
    (void)std::system("xxd /tmp/demo_recovery_db/wal.log | head -5");

    separator("Construim LSMTree -- WAL recovery automat la startup");
    std::cout << "  " << color::yellow
              << "Citim WAL, verificam CRC32c per inregistrare,\n"
              << "  reaplicam operatiile in MemTable..."
              << color::reset << "\n";

    lsm::LSMOptions opts;
    opts.memtable_size = 64 * 1024 * 1024;
    lsm::LSMTree tree(DB_PATH, opts);

    std::cout << "  " << color::green << "Recovery complet." << color::reset << "\n";

    separator("GET dupa recovery");
    const char* keys[] = {"alfa", "beta", "gamma", "delta", "epsilon", "zeta"};
    for (auto& k : keys) {
        auto v = tree.get(k);
        if (v) ok(k, *v);
        else   not_found(k);
    }

    separator("SCAN [alfa, zeta]");
    auto results = tree.scan("alfa", "zeta");
    std::cout << "  " << color::cyan << results.size()
              << " chei recuperate din WAL:" << color::reset << "\n";
    for (auto& kv : results)
        ok(kv.key, kv.value);

    std::cout << "\n" << color::bold << color::green
              << "  Nicio scriere confirmata nu s-a pierdut."
              << color::reset << "\n\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: demo_recovery [write|recover]\n";
        return 1;
    }

    std::string mode = argv[1];
    if (mode == "write") {
        fs::remove_all(DB_PATH);
        phase_write();
    } else if (mode == "recover") {
        if (!fs::exists(DB_PATH)) {
            std::cerr << color::red
                      << "Nu exista DB. Ruleaza mai intai: demo_recovery write\n"
                      << color::reset;
            return 1;
        }
        phase_recover();
    } else {
        std::cerr << "Mod necunoscut: " << mode << "\n";
        return 1;
    }
    return 0;
}