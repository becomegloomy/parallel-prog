#!/usr/bin/env python3
import csv
import os
import sys
from collections import defaultdict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR   = os.path.dirname(SCRIPT_DIR)
CSV_IN     = os.path.join(ROOT_DIR, "results.csv")
OUT_SPEED  = os.path.join(ROOT_DIR, "speedup.png")
OUT_STAGES = os.path.join(ROOT_DIR, "stages_fraction.png")

STAGES = ["Generate_M1", "Generate_M2", "Map_M1", "Map_M2",
          "Merge", "Sort", "Reduce"]


def read_results(path):
    """→ dict[(N, T)] = {'stage': sec, 'Total_ms': int}."""
    data = {}
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            N = int(row["N"])
            T = int(row["Threads"])
            rec = {s: float(row[s]) for s in STAGES}
            rec["Total_ms"] = int(row["Total_ms"])
            data[(N, T)] = rec
    return data


def plot_speedup(data):
    Ns = sorted({N for (N, _) in data})
    threads = sorted({T for (_, T) in data})

    plt.figure(figsize=(10, 6))
    for T in threads:
        S = []
        for N in Ns:
            base = data.get((N, 1))
            cur  = data.get((N, T))
            if not base or not cur or cur["Total_ms"] == 0:
                S.append(np.nan)
            else:
                S.append(base["Total_ms"] / cur["Total_ms"])
        plt.plot(Ns, S, marker="o", label=f"{T} threads")

    plt.axhline(1.0, color="gray", linestyle="--", label="S = 1")
    plt.xlabel("Размер массива N")
    plt.ylabel("Параллельное ускорение S(N) = T(1) / T(T)")
    plt.title("Параллельное ускорение (pthreads)")
    plt.xscale("log")
    plt.grid(True, which="both", ls="--", alpha=0.5)
    plt.legend(title="Потоки")
    plt.tight_layout()
    plt.savefig(OUT_SPEED, dpi=150)
    plt.close()
    print(f"Saved: {OUT_SPEED}")


def plot_stages(data):
    Ns = sorted({N for (N, _) in data})
    threads = sorted({T for (_, T) in data})
    T = max(threads)

    fractions = {s: [] for s in STAGES}
    for N in Ns:
        rec = data.get((N, T))
        if not rec:
            for s in STAGES:
                fractions[s].append(0.0)
            continue
        sum_stages = sum(rec[s] for s in STAGES) or 1.0
        for s in STAGES:
            fractions[s].append(rec[s] / sum_stages)

    plt.figure(figsize=(10, 6))
    plt.stackplot(Ns, [fractions[s] for s in STAGES], labels=STAGES)
    plt.xlabel("Размер массива N")
    plt.ylabel("Доля времени этапа")
    plt.title(f"Доли времени этапов ({T} потоков)")
    plt.xscale("log")
    plt.legend(loc="upper left")
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig(OUT_STAGES, dpi=150)
    plt.close()
    print(f"Saved: {OUT_STAGES}")


def main():
    if not os.path.isfile(CSV_IN):
        print(f"Error: {CSV_IN} not found (run scripts/run.py first)", file=sys.stderr)
        sys.exit(1)
    data = read_results(CSV_IN)
    plot_speedup(data)
    plot_stages(data)


if __name__ == "__main__":
    main()
