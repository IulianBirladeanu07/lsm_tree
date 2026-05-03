#include "lsm/lsm_tree.h"

#include <sqlite3.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

static constexpr int    kNumKeys      = 100'000;
static constexpr int    kKeySize      = 24;
static constexpr int    kValueSize    = 128;
static constexpr int    kWarmupOps    = 5'000;
static constexpr int    kBenchOps     = 50'000;
static constexpr size_t kMemtableSize = 32 * 1024 * 1024;

using Clock = std::chrono::steady_clock;
using Nanos = std::chrono::nanoseconds;

struct Sample {
    std::string op;
    int64_t     latency_ns;
};

struct WorkloadDef {
    std::string name;
    double      read_ratio;
    double      write_ratio;
};

static const WorkloadDef kWorkloads[] = {
    {"A", 0.50, 0.50},
    {"B", 0.95, 0.05},
    {"C", 1.00, 0.00},
    {"D", 0.05, 0.95},
};

std::string make_key(int id) {
    return std::format("key_{:0{}d}", id, kKeySize - 4);
}

std::string make_value(int id) {
    std::string v(kValueSize, 'v');
    auto s = std::to_string(id);
    std::copy(s.begin(), s.end(), v.begin());
    return v;
}

std::vector<int> build_key_population(int n) {
    std::vector<int> keys(n);
    std::iota(keys.begin(), keys.end(), 0);
    return keys;
}

std::vector<std::pair<std::string, int>> build_ops(
    const WorkloadDef& wl, const std::vector<int>& population,
    int num_ops, std::mt19937& rng)
{
    std::uniform_real_distribution<double> coin(0.0, 1.0);
    std::uniform_int_distribution<int>     pick(0, static_cast<int>(population.size()) - 1);

    std::vector<std::pair<std::string, int>> ops;
    ops.reserve(num_ops);
    for (int i = 0; i < num_ops; ++i) {
        double r = coin(rng);
        ops.push_back({r < wl.read_ratio ? "read" : "write", population[pick(rng)]});
    }
    return ops;
}

double percentile(std::vector<int64_t>& v, double p) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    auto idx = static_cast<size_t>(p / 100.0 * static_cast<double>(v.size() - 1));
    return static_cast<double>(v[idx]) / 1000.0;
}

void write_csv(const std::string& path, const std::vector<Sample>& samples) {
    std::ofstream f(path);
    f << "op,latency_ns\n";
    for (const auto& s : samples)
        f << s.op << "," << s.latency_ns << "\n";
}

void write_summary_row(std::ofstream& f,
                       const std::string& impl,
                       const std::string& workload,
                       const std::vector<Sample>& samples,
                       double elapsed_sec)
{
    std::vector<int64_t> read_lat, write_lat;
    for (const auto& s : samples) {
        if (s.op == "read") read_lat.push_back(s.latency_ns);
        else                write_lat.push_back(s.latency_ns);
    }

    double throughput = static_cast<double>(samples.size()) / elapsed_sec;
    auto p50r = percentile(read_lat,  50.0);
    auto p99r = percentile(read_lat,  99.0);
    auto p50w = percentile(write_lat, 50.0);
    auto p99w = percentile(write_lat, 99.0);

    f << impl << "," << workload << ","
      << static_cast<int>(throughput) << ","
      << p50r << "," << p99r << ","
      << p50w << "," << p99w << "\n";

    std::cout << std::format(
        "  [{:<14}] wl={} | throughput={:>8.0f} ops/s | "
        "read p50={:>8.1f}us p99={:>8.1f}us | "
        "write p50={:>8.1f}us p99={:>8.1f}us\n",
        impl, workload, throughput, p50r, p99r, p50w, p99w);
}

void run_lsm_workload(const std::string& impl_name,
                      lsm::LSMTree& tree,
                      const WorkloadDef& wl,
                      const std::vector<int>& population,
                      std::mt19937& rng,
                      std::ofstream& summary,
                      const std::string& results_dir)
{
    auto warmup_ops = build_ops(wl, population, kWarmupOps, rng);
    for (auto& [op, id] : warmup_ops) {
        if (op == "write") tree.put(make_key(id), make_value(id));
        else               tree.get(make_key(id));
    }

    auto ops = build_ops(wl, population, kBenchOps, rng);
    std::vector<Sample> samples;
    samples.reserve(ops.size());

    auto t_start = Clock::now();
    for (auto& [op, id] : ops) {
        auto t0 = Clock::now();
        if (op == "write") tree.put(make_key(id), make_value(id));
        else               tree.get(make_key(id));
        auto t1 = Clock::now();
        samples.push_back({op, std::chrono::duration_cast<Nanos>(t1 - t0).count()});
    }
    auto t_end = Clock::now();
    double elapsed = std::chrono::duration<double>(t_end - t_start).count();

    write_csv(results_dir + "/" + impl_name + "_" + wl.name + ".csv", samples);
    write_summary_row(summary, impl_name, wl.name, samples, elapsed);
}

struct SqliteHandle {
    sqlite3*      db       = nullptr;
    sqlite3_stmt* put_stmt = nullptr;
    sqlite3_stmt* get_stmt = nullptr;

    explicit SqliteHandle(const std::string& path) {
        sqlite3_open(path.c_str(), &db);
        sqlite3_exec(db, "PRAGMA journal_mode=WAL;",      nullptr, nullptr, nullptr);
        sqlite3_exec(db, "PRAGMA synchronous=NORMAL;",    nullptr, nullptr, nullptr);
        sqlite3_exec(db, "PRAGMA cache_size=-8000;",      nullptr, nullptr, nullptr);
        sqlite3_exec(db,
            "CREATE TABLE IF NOT EXISTS kv (k TEXT PRIMARY KEY, v TEXT);",
            nullptr, nullptr, nullptr);
        sqlite3_prepare_v2(db,
            "INSERT OR REPLACE INTO kv(k,v) VALUES(?,?);", -1, &put_stmt, nullptr);
        sqlite3_prepare_v2(db,
            "SELECT v FROM kv WHERE k=?;", -1, &get_stmt, nullptr);
    }

    ~SqliteHandle() {
        if (put_stmt) sqlite3_finalize(put_stmt);
        if (get_stmt) sqlite3_finalize(get_stmt);
        if (db)       sqlite3_close(db);
    }

    void put(const std::string& key, const std::string& value) {
        sqlite3_bind_text(put_stmt, 1, key.c_str(),   -1, SQLITE_STATIC);
        sqlite3_bind_text(put_stmt, 2, value.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(put_stmt);
        sqlite3_reset(put_stmt);
    }

    void get(const std::string& key) {
        sqlite3_bind_text(get_stmt, 1, key.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(get_stmt);
        sqlite3_reset(get_stmt);
    }
};

void seed_lsm(lsm::LSMTree& tree, const std::vector<int>& population) {
    std::cout << "  seeding " << population.size() << " keys into LSM...\n";
    for (int id : population)
        tree.put(make_key(id), make_value(id));
}

void seed_sqlite(SqliteHandle& db, const std::vector<int>& population) {
    std::cout << "  seeding " << population.size() << " keys into SQLite...\n";
    sqlite3_exec(db.db, "BEGIN;", nullptr, nullptr, nullptr);
    for (int id : population)
        db.put(make_key(id), make_value(id));
    sqlite3_exec(db.db, "COMMIT;", nullptr, nullptr, nullptr);
}

void run_sqlite_workload(const WorkloadDef& wl,
                         const std::vector<int>& population,
                         std::mt19937& rng,
                         std::ofstream& summary,
                         const std::string& results_dir,
                         const std::string& db_path)
{
    SqliteHandle db(db_path);

    auto warmup_ops = build_ops(wl, population, kWarmupOps, rng);
    for (auto& [op, id] : warmup_ops) {
        if (op == "write") db.put(make_key(id), make_value(id));
        else               db.get(make_key(id));
    }

    auto ops = build_ops(wl, population, kBenchOps, rng);
    std::vector<Sample> samples;
    samples.reserve(ops.size());

    auto t_start = Clock::now();
    for (auto& [op, id] : ops) {
        auto t0 = Clock::now();
        if (op == "write") db.put(make_key(id), make_value(id));
        else               db.get(make_key(id));
        auto t1 = Clock::now();
        samples.push_back({op, std::chrono::duration_cast<Nanos>(t1 - t0).count()});
    }
    auto t_end = Clock::now();
    double elapsed = std::chrono::duration<double>(t_end - t_start).count();

    write_csv(results_dir + "/sqlite_" + wl.name + ".csv", samples);
    write_summary_row(summary, "sqlite", wl.name, samples, elapsed);
}

int main() {
    std::string results_dir = "results";
    std::filesystem::create_directories(results_dir);

    std::ofstream summary(results_dir + "/summary.csv");
    summary << "impl,workload,throughput_ops_per_sec,"
               "read_p50_us,read_p99_us,write_p50_us,write_p99_us\n";

    auto population = build_key_population(kNumKeys);
    std::mt19937 rng(42);

    lsm::LSMOptions leveled_opts;
    leveled_opts.memtable_size    = kMemtableSize;
    leveled_opts.compaction_style = lsm::LSMOptions::CompactionStyle::Leveled;

    lsm::LSMOptions tiered_opts;
    tiered_opts.memtable_size    = kMemtableSize;
    tiered_opts.compaction_style = lsm::LSMOptions::CompactionStyle::Tiered;

    for (const auto& wl : kWorkloads) {
        std::cout << "\n=== Workload " << wl.name
                  << " (read=" << wl.read_ratio * 100
                  << "% write=" << wl.write_ratio * 100 << "%) ===\n";

        {
            std::string dir = "/tmp/bench_lsm_leveled_" + wl.name;
            std::filesystem::remove_all(dir);
            lsm::LSMTree tree(dir, leveled_opts);
            seed_lsm(tree, population);
            run_lsm_workload("lsm_leveled", tree, wl, population, rng, summary, results_dir);
            std::filesystem::remove_all(dir);
        }

        {
            std::string dir = "/tmp/bench_lsm_tiered_" + wl.name;
            std::filesystem::remove_all(dir);
            lsm::LSMTree tree(dir, tiered_opts);
            seed_lsm(tree, population);
            run_lsm_workload("lsm_tiered", tree, wl, population, rng, summary, results_dir);
            std::filesystem::remove_all(dir);
        }

        {
            std::string db_path = "/tmp/bench_sqlite_" + wl.name + ".db";
            std::filesystem::remove(db_path);
            {
                SqliteHandle db(db_path);
                seed_sqlite(db, population);
            }
            run_sqlite_workload(wl, population, rng, summary, results_dir, db_path);
            std::filesystem::remove(db_path);
        }
    }

    std::cout << "\nResults written to " << results_dir << "/\n";
    std::cout << "Run: python3 bench/plot.py results/summary.csv\n";
    return 0;
}