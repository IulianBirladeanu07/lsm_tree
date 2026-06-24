#!/usr/bin/env bash
# Usage: bash bench/run_bench.sh [N] [binary]
# Examples:
#   bash bench/run_bench.sh 10
#   bash bench/run_bench.sh 10 build/bench/lsm_bench

set -e

N="${1:-10}"
BINARY="${2:-build/bench/lsm_bench}"
RESULTS_DIR="results"
RUNS_DIR="$RESULTS_DIR/runs"

echo "=== Running benchmark $N times ==="
mkdir -p "$RUNS_DIR"

for i in $(seq 1 "$N"); do
    echo ""
    echo "--- Run $i / $N ---"
    "$BINARY"
    cp "$RESULTS_DIR/summary.csv" "$RUNS_DIR/summary_run_$i.csv"
done

echo ""
echo "=== Computing averages across $N runs ==="

python3 - "$RUNS_DIR" "$N" "$RESULTS_DIR/summary_avg.csv" << 'PYEOF'
import sys, csv, os
from collections import defaultdict
from statistics import mean, stdev

runs_dir = sys.argv[1]
n        = int(sys.argv[2])
out_path = sys.argv[3]

# impl -> workload -> metric -> [values]
data = defaultdict(lambda: defaultdict(lambda: defaultdict(list)))

for i in range(1, n + 1):
    path = os.path.join(runs_dir, f"summary_run_{i}.csv")
    with open(path) as f:
        for row in csv.DictReader(f):
            impl = row["impl"]
            wl   = row["workload"]
            for metric in ["throughput_ops_per_sec", "read_p50_us", "read_p99_us",
                           "write_p50_us", "write_p99_us"]:
                data[impl][wl][metric].append(float(row[metric]))

# Write summary_avg.csv (consumed by plot.py)
with open(out_path, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(["impl", "workload", "throughput_ops_per_sec",
                     "read_p50_us", "read_p99_us", "write_p50_us", "write_p99_us"])
    for impl in sorted(data):
        for wl in sorted(data[impl]):
            d = data[impl][wl]
            writer.writerow([
                impl, wl,
                round(mean(d["throughput_ops_per_sec"])),
                round(mean(d["read_p50_us"]),  2),
                round(mean(d["read_p99_us"]),  2),
                round(mean(d["write_p50_us"]), 2),
                round(mean(d["write_p99_us"]), 2),
            ])

# Print summary table with mean and std dev
impls     = ["lsm_leveled", "lsm_tiered", "sqlite"]
workloads = ["A", "B", "C", "D"]

print(f"\n{'Impl':<14} {'WL':<4} {'Throughput':>12}  {'±':>8}  {'Rp50':>6}  {'Rp99':>6}  {'Wp50':>6}  {'Wp99':>8}")
print("-" * 80)
for impl in impls:
    for wl in workloads:
        if wl not in data[impl]:
            continue
        d   = data[impl][wl]
        thr = d["throughput_ops_per_sec"]
        print(f"{impl:<14} {wl:<4} "
              f"{round(mean(thr)):>12,}  "
              f"±{round(stdev(thr)):>7,}  "
              f"{mean(d['read_p50_us']):>6.1f}  "
              f"{mean(d['read_p99_us']):>6.1f}  "
              f"{mean(d['write_p50_us']):>6.1f}  "
              f"{mean(d['write_p99_us']):>8.1f}")

print(f"\nAverages saved to: {out_path}")
PYEOF

echo ""
echo "=== Generating plots from averages ==="
bash bench/plot.sh "$RESULTS_DIR/summary_avg.csv"

echo ""
echo "Plot:      $RESULTS_DIR/benchmark.png"
echo "Averages:  $RESULTS_DIR/summary_avg.csv"
echo "Raw runs:  $RESULTS_DIR/runs/"