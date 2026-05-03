# lsm_tree

A concurrent Log-Structured Merge Tree implemented in C++20, built as a master's dissertation project at Politehnica Timișoara.

## Build

```bash
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja
```

Requires: GCC 13+, CMake 3.20+, Ninja, SSE4.2-capable CPU.

## Tests

```bash
cd build
ctest --output-on-failure
```

Or run individual test binaries directly from `build/tests/`.

## Benchmark

```bash
cd build
./bench/lsm_bench
```

Results are written to `results/` in the working directory.

To generate charts:

```bash
bash bench/plot.sh results/summary.csv
```

Requires Python 3 (a virtualenv is created automatically on first run).

## Architecture

```
put(key, value)
    │
    ├── WAL::append()              sequential write, crash recovery
    ├── MemTable::put()            ConcurrentSkipList, lock-free reads
    │
    └── (when full) flush_memtable()
            │
            ├── SSTableBuilder     sorted, immutable file on disk
            ├── LevelManager       level hierarchy (L0–L6)
            └── CompactionScheduler → ThreadPool
```

**Read path:** MemTable → immutable MemTables → L0 SSTables → L1..LN SSTables.  
Each SSTable lookup goes through a BloomFilter before touching disk.

### Components

| Component | Description |
|-----------|-------------|
| `WAL` | Append-only write-ahead log with CRC32 checksums |
| `MemTable` | In-memory write buffer backed by `ConcurrentSkipList` |
| `ConcurrentSkipList` | Single-writer mutex + lock-free reads, 12 levels |
| `SSTable` | Immutable on-disk sorted file: data blocks + index block + bloom filter + footer |
| `BlockCache` | Thread-safe LRU cache for decompressed blocks |
| `BloomFilter` | Per-SSTable probabilistic filter, double-hashing (Kirsch-Mitzenmacher) |
| `LevelManager` | Manages level hierarchy, thread-safe with `shared_mutex` |
| `CompactionScheduler` | Background compaction via `ThreadPool`, pluggable strategy |
| `LeveledCompaction` | Score = actual\_size / (base × multiplier^level) |
| `TieredCompaction` | Score = num\_files / size\_ratio |
| `MergingIterator` | Min-heap over N child iterators, used in compaction and scan |

### SSTable File Layout

```
[ Data Block 0 ] [ Data Block 1 ] ... [ Index Block ] [ Filter Block ] [ Footer (24B) ]
```

Each data block: `[key_len|val_len|key|value|...|CRC32]`  
Footer: `[index_offset (8B) | filter_offset (8B) | magic (8B)]`

### Compaction

Two pluggable strategies selectable via `LSMOptions::CompactionStyle`:

- **Leveled** — low read amplification, higher write amplification. Size limit per level: `base × multiplier^i`.
- **Tiered** — low write amplification, higher space amplification. Compacts a level when file count ≥ `size_ratio`.

Tombstones (`\xFF` sentinel) are filtered during compaction and in `get()`.

## C++20 Features Used

| Feature | Where |
|---------|-------|
| `std::jthread` | `ThreadPool` workers |
| `std::atomic<shared_ptr<T>>` | Lock-free MemTable swap during flush |
| `std::format` | SSTable filename generation |
| `std::span` | Zero-copy buffer views in WAL and BloomFilter |
| `std::shared_mutex` | `LevelManager`, immutable MemTable list |

## Benchmark Results

YCSB-inspired workloads, 100k keys (24B key / 128B value), 50k ops after 5k warmup, LSM memtable 32MB.

| Workload | LSM Leveled | LSM Tiered | SQLite |
|----------|------------|------------|--------|
| A 50R/50W | 223k ops/s | 234k ops/s | 19.8k ops/s |
| B 95R/5W  | 222k ops/s | 280k ops/s | 80k ops/s  |
| C 100R/0W | 274k ops/s | 292k ops/s | 140k ops/s |
| D 5R/95W  | 183k ops/s | 189k ops/s | 10.6k ops/s |

SQLite configured with WAL mode, `synchronous=NORMAL`, prepared statements, batched writes (100 writes/txn).

## Known Limitations

- No WAL truncation after flush (WAL grows unbounded)
- No snapshot isolation / MVCC
- `scan()` does not deduplicate across SSTable levels for the same key at different sequence numbers
- No block compression
