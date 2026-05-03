# LSM Tree — C++20 Implementation

A concurrent Log-Structured Merge Tree implemented in C++20, built as a Master's dissertation project at Politehnica Timișoara. The implementation targets high write throughput with configurable read/space amplification tradeoffs via pluggable compaction strategies, and includes YCSB-inspired benchmarks compared against SQLite.

## Features

- **Concurrent reads and writes** — lock-free read path via `atomic<shared_ptr<MemTable>>`
- **Write-Ahead Log** — CRC32-verified records, configurable fsync, full crash recovery on startup
- **Bloom filters** — per SSTable, double-hashing (Kirsch-Mitzenmacher), serialized to disk
- **Block-level LRU cache** — thread-safe, reduces disk I/O for hot data
- **Pluggable compaction** — Leveled and Tiered strategies, background ThreadPool
- **Range scan** — `scan(start, end)` via MergingIterator over all data sources
- **SSE4.2 CRC32** — hardware-accelerated checksums
- **90 unit tests** — Google Test, zero external dependencies beyond the build

## Architecture

```
┌─────────────────────────────────────────────┐
│                  LSMTree                    │
│    put() / get() / del() / scan()           │
└────────┬──────────┬──────────┬──────────────┘
         │          │          │
    ┌────▼───┐  ┌───▼────┐  ┌─▼──────────────┐
    │  WAL   │  │MemTable│  │  LevelManager  │
    └────────┘  └───┬────┘  └───────┬────────┘
                    │               │
         ┌──────────▼───┐      ┌────▼──────┐
         │ConcurrentSkip│      │  SSTable  │
         │    List      │      │BloomFilter│
         └──────────────┘      │BlockCache │
                               └───────────┘
Background:
┌──────────────────────────────────────────────┐
│            CompactionScheduler               │
│    ThreadPool + ICompactionStrategy          │
│    LeveledCompaction | TieredCompaction      │
└──────────────────────────────────────────────┘
```

### Write Path

```
put(key, value)
  → WAL::append()              persisted first, CRC32-protected
  → MemTable::put()            ConcurrentSkipList, single-writer mutex + lock-free reads
  → flush_memtable()           when full: push to imm_, atomic swap, background SSTableBuilder
  → LevelManager::add(L0)      triggers CompactionScheduler::schedule()
```

### Read Path

```
get(key)
  → active MemTable            newest, O(log n)
  → immutable MemTables        newest-first
  → Level 0 SSTables           all files (may overlap), BloomFilter + BlockCache
  → Level 1..N SSTables        binary search on key range
```

### Scan Path

```
scan(start, end)
  → MergingIterator over:
      active MemTable (SkipListIterator)
      immutable MemTables
      all SSTable levels (SSTableIterator per file)
  → seek(start), iterate while key <= end
  → deduplicates keys, filters tombstones
```

### SSTable File Format

```
┌──────────────────────────────┐
│       Data Block 0           │  4KB default, sorted KV pairs + CRC32
├──────────────────────────────┤
│       Data Block 1           │
├──────────────────────────────┤
│           ...                │
├──────────────────────────────┤
│       Index Block            │  (first_key, last_key, offset, size) per block
├──────────────────────────────┤
│       Filter Block           │  serialized BloomFilter
├──────────────────────────────┤
│         Footer               │  index_offset, filter_offset, magic (24 bytes)
└──────────────────────────────┘
```

## Project Structure

```
include/lsm/
  util/         bloom_filter.h, crc32.h
  sstable/      block.h, block_builder.h, block_cache.h, footer.h,
                index_block.h, sstable.h, sstable_builder.h
  wal/          wal.h
  memtable/     concurrent_skip_list.h, memtable.h
  levels/       level_manager.h
  iterator/     iterator.h, block_iterator.h, skiplist_iterator.h,
                sstable_iterator.h, merging_iterator.h
  compaction/   thread_pool.h, compaction_job.h, compaction_strategy.h,
                leveled_compaction.h, tiered_compaction.h, compaction_scheduler.h
  lsm_tree.h

src/            Implementation files mirroring include/lsm/
tests/          90 Google Test cases, one executable per component
bench/          YCSB-inspired benchmark vs SQLite, Python plot script
docs/           Detail design document (AsciiDoc) + PlantUML class diagrams
```

## Build

### Requirements

- CMake 3.20+
- GCC 13+ or Clang 16+ with C++20 support
- Ninja (recommended)
- Python 3.8+ (for benchmark plots only)

**No other dependencies.** SQLite and Google Test are fetched automatically via CMake FetchContent.

### Compile

```bash
mkdir build && cd build
cmake .. -G Ninja
ninja
```

> **WSL users:** build on the Linux filesystem to avoid NTFS permission issues with FetchContent:
> ```bash
> cmake -S . -B /tmp/lsm_build -G Ninja && ninja -C /tmp/lsm_build
> ```

### Run Tests

```bash
ctest --test-dir /tmp/lsm_build --output-on-failure
```

Or a specific suite:

```bash
/tmp/lsm_build/tests/test_lsm_tree
/tmp/lsm_build/tests/test_lsm_tree --gtest_filter="*Scan*"
```

### Run Benchmarks

```bash
/tmp/lsm_build/bench/lsm_bench
```

Results are written to `build/results/`. To generate plots:

```bash
./bench/plot.sh /tmp/lsm_build/results/summary.csv
```

The script creates a Python venv on first run and installs matplotlib/numpy automatically. Output: `results/benchmark.png`.

## API

```cpp
#include "lsm/lsm_tree.h"

lsm::LSMOptions opts;
opts.memtable_size    = 64 * 1024 * 1024;
opts.compaction_style = lsm::LSMOptions::CompactionStyle::Leveled;

lsm::LSMTree tree("/path/to/db", opts);

tree.put("key", "value");

std::optional<std::string> val = tree.get("key");

tree.del("key");

std::vector<lsm::KV> results = tree.scan("key_010", "key_020");
for (auto& kv : results) {
    // kv.key, kv.value
}
```

### Options

| Field | Default | Description |
|-------|---------|-------------|
| `memtable_size` | 64 MB | MemTable capacity before flush |
| `block_cache_size` | 8 MB | LRU block cache capacity |
| `num_levels` | 7 | Number of SSTable levels |
| `compaction_threads` | 2 | Background compaction thread count |
| `sync_writes` | false | fsync WAL after every write |
| `bloom_fpr` | 0.01 | Bloom filter false positive rate |
| `block_size` | 4096 | Target data block size in bytes |
| `compaction_style` | Leveled | `Leveled` or `Tiered` |

## Benchmark Results

Workloads are YCSB-inspired, 100,000 keys (24B key / 128B value), 50,000 ops after 5,000 warmup. SQLite configured with WAL mode, `synchronous=NORMAL`, prepared statements, batched writes (100 writes/txn).

### Throughput (ops/s)

| Impl | A (50R/50W) | B (95R/5W) | C (100R/0W) | D (5R/95W) |
|------|-------------|------------|-------------|------------|
| LSM Leveled | 223,305 | 222,857 | 274,405 | 183,286 |
| LSM Tiered  | 234,326 | 280,793 | 295,293 | 189,739 |
| SQLite      | 19,793  | 80,047  | 140,975 | 10,599  |

### Write Latency p50 (µs)

| Impl | A | B | D |
|------|---|---|---|
| LSM Leveled | 4.6 | 5.9 | 4.6 |
| LSM Tiered  | 4.7 | 5.0 | 4.7 |
| SQLite      | 24.7 | 25.9 | 23.5 |

LSM achieves 2–17x higher throughput than SQLite depending on workload. Write latency p50 is ~5µs for LSM vs ~25µs for SQLite.

The Leveled vs Tiered difference is small at this dataset size — compaction pressure is insufficient to expose the theoretical write amplification tradeoff. The difference becomes measurable at datasets larger than available RAM.

## RUM Conjecture Tradeoffs

| Strategy | Write Amplification | Read Amplification | Space Amplification |
|----------|--------------------|--------------------|---------------------|
| Leveled  | High (~10-30x)     | Low (~2-5x)        | Low (~1.1x)         |
| Tiered   | Low (~3-10x)       | High (~10-30x)     | High (~2-3x)        |

## C++20 Features

| Feature | Usage |
|---------|-------|
| `std::jthread` | Background flush and compaction threads with automatic join |
| `std::atomic<shared_ptr<T>>` | Lock-free MemTable swap during flush |
| `std::span` | Zero-copy buffer views in Block and WAL |
| `std::format` | SSTable filename generation, benchmark output formatting |
| `std::shared_mutex` | Reader-writer lock on immutable MemTable list and LevelManager |

## Known Limitations

- No MVCC / snapshot isolation — single-version store, last write wins
- WAL is truncated after each memtable flush rather than after full SSTable compaction — not durable across crashes mid-compaction
- Leveled vs Tiered compaction difference requires datasets larger than RAM to be observable in benchmarks
- No prefix compression in SSTable blocks