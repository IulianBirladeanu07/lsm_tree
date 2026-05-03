import sys
import csv
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path

IMPL_LABELS = {
    "lsm_leveled": "LSM Leveled",
    "lsm_tiered":  "LSM Tiered",
    "sqlite":       "SQLite",
}
IMPL_COLORS = {
    "lsm_leveled": "#4C72B0",
    "lsm_tiered":  "#DD8452",
    "sqlite":       "#55A868",
}
WORKLOADS   = ["A", "B", "C", "D"]
WORKLOAD_DESC = {
    "A": "A — 50R/50W",
    "B": "B — 95R/5W",
    "C": "C — 100R/0W",
    "D": "D — 5R/95W",
}

def load_summary(path):
    rows = {}
    with open(path) as f:
        for row in csv.DictReader(f):
            key = (row["impl"], row["workload"])
            rows[key] = {
                "throughput": float(row["throughput_ops_per_sec"]),
                "read_p50":   float(row["read_p50_us"]),
                "read_p99":   float(row["read_p99_us"]),
                "write_p50":  float(row["write_p50_us"]),
                "write_p99":  float(row["write_p99_us"]),
            }
    return rows

def grouped_bar(ax, data, impls, workloads, title, ylabel, fmt="{:.0f}"):
    x      = np.arange(len(workloads))
    width  = 0.25
    offset = -(len(impls) - 1) / 2

    for i, impl in enumerate(impls):
        vals = [data.get((impl, wl), {}).get(title.lower().replace(" ", "_"), 0)
                for wl in workloads]
        bars = ax.bar(x + (offset + i) * width, vals,
                      width, label=IMPL_LABELS[impl],
                      color=IMPL_COLORS[impl], edgecolor="white", linewidth=0.5)
        for bar, v in zip(bars, vals):
            if v > 0:
                ax.text(bar.get_x() + bar.get_width() / 2,
                        bar.get_height() * 1.01,
                        fmt.format(v),
                        ha="center", va="bottom", fontsize=6.5, rotation=45)

    ax.set_xticks(x)
    ax.set_xticklabels([WORKLOAD_DESC[w] for w in workloads], fontsize=9)
    ax.set_ylabel(ylabel, fontsize=9)
    ax.set_title(title, fontsize=10, fontweight="bold")
    ax.legend(fontsize=8)
    ax.yaxis.grid(True, linestyle="--", alpha=0.6)
    ax.set_axisbelow(True)

def main():
    summary_path = sys.argv[1] if len(sys.argv) > 1 else "results/summary.csv"
    out_dir      = Path(summary_path).parent

    data  = load_summary(summary_path)
    impls = ["lsm_leveled", "lsm_tiered", "sqlite"]

    fig, axes = plt.subplots(2, 3, figsize=(16, 9))
    fig.suptitle("LSM Tree vs SQLite — YCSB-inspired Benchmark", fontsize=13, fontweight="bold")

    metrics = [
        ("throughput",  axes[0][0], "Throughput (ops/s)",       "{:.0f}"),
        ("read_p50",    axes[0][1], "Read Latency p50 (µs)",    "{:.1f}"),
        ("read_p99",    axes[0][2], "Read Latency p99 (µs)",    "{:.1f}"),
        ("write_p50",   axes[1][0], "Write Latency p50 (µs)",   "{:.1f}"),
        ("write_p99",   axes[1][1], "Write Latency p99 (µs)",   "{:.1f}"),
    ]

    for metric, ax, ylabel, fmt in metrics:
        x      = np.arange(len(WORKLOADS))
        width  = 0.25
        offset = -(len(impls) - 1) / 2

        for i, impl in enumerate(impls):
            vals = [data.get((impl, wl), {}).get(metric, 0) for wl in WORKLOADS]
            bars = ax.bar(x + (offset + i) * width, vals,
                          width, label=IMPL_LABELS[impl],
                          color=IMPL_COLORS[impl], edgecolor="white", linewidth=0.5)
            for bar, v in zip(bars, vals):
                if v > 0:
                    ax.text(bar.get_x() + bar.get_width() / 2,
                            bar.get_height() * 1.01,
                            fmt.format(v),
                            ha="center", va="bottom", fontsize=6.5, rotation=45)

        ax.set_xticks(x)
        ax.set_xticklabels([WORKLOAD_DESC[w] for w in WORKLOADS], fontsize=8)
        ax.set_ylabel(ylabel, fontsize=9)
        ax.set_title(ylabel, fontsize=10, fontweight="bold")
        ax.legend(fontsize=7)
        ax.yaxis.grid(True, linestyle="--", alpha=0.6)
        ax.set_axisbelow(True)

    axes[1][2].axis("off")
    note = (
        "Workloads (YCSB-inspired):\n"
        "  A — Balanced        50% read / 50% write\n"
        "  B — Read-heavy      95% read /  5% write\n"
        "  C — Read-only      100% read /  0% write\n"
        "  D — Write-heavy      5% read / 95% write\n\n"
        f"Keys: 100,000 × 24B key / 128B value\n"
        f"Benchmark ops: 50,000 (after 5,000 warmup)\n"
        f"LSM memtable: 32 MB\n"
        f"SQLite: WAL mode, synchronous=NORMAL, prepared stmts, batch=100 writes/txn"
    )
    axes[1][2].text(0.05, 0.95, note, transform=axes[1][2].transAxes,
                    fontsize=8.5, verticalalignment="top",
                    fontfamily="monospace",
                    bbox=dict(boxstyle="round", facecolor="wheat", alpha=0.4))

    plt.tight_layout()
    out_path = out_dir / "benchmark.png"
    plt.savefig(out_path, dpi=150, bbox_inches="tight")
    print(f"Saved: {out_path}")

if __name__ == "__main__":
    main()