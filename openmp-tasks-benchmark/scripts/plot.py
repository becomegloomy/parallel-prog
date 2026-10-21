#!/usr/bin/env python3
"""График ускорения с доверительными интервалами CI95% по results_2.csv."""

import csv
import os
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR   = os.path.dirname(SCRIPT_DIR)
CSV_IN     = os.path.join(ROOT_DIR, "results_2.csv")
OUT_PNG    = os.path.join(ROOT_DIR, "speedup_ci.png")


def read_results(path):
    """→ dict[(N, T)] = list[times_ms]."""
    data = {}
    with open(path, newline="") as f:
        reader = csv.reader(f)
        header = next(reader)
        time_cols = [i for i, name in enumerate(header) if name.startswith("Time")]
        for row in reader:
            N = int(row[0])
            T = int(row[1])
            times = [float(row[i]) for i in time_cols if row[i] != ""]
            data[(N, T)] = times
    return data


def ci95(values):
    """95% доверительный интервал по t-распределению (без scipy)."""
    n = len(values)
    mean = np.mean(values)
    if n < 2:
        return mean, mean, mean
    std_err = np.std(values, ddof=1) / np.sqrt(n)
    t_crit = 2.262 if n == 10 else 2.0
    return mean, mean - t_crit * std_err, mean + t_crit * std_err


def main():
    if not os.path.isfile(CSV_IN):
        print(f"Error: {CSV_IN} not found (run scripts/run.sh first)", file=sys.stderr)
        sys.exit(1)

    data = read_results(CSV_IN)
    Ns = sorted({N for (N, _) in data})
    threads = sorted({T for (_, T) in data})

    plt.figure(figsize=(12, 6))
    for T in threads:
        S_mean, S_lower, S_upper = [], [], []
        for N in Ns:
            seq_times = data.get((N, 1))
            par_times = data.get((N, T))
            if not seq_times or not par_times:
                S_mean.append(np.nan); S_lower.append(np.nan); S_upper.append(np.nan)
                continue
            T_seq = np.mean(seq_times)
            mean, lo, hi = ci95(par_times)
            S_mean.append(T_seq / mean)
            S_lower.append(T_seq / hi)
            S_upper.append(T_seq / lo)

        plt.plot(Ns, S_mean, marker="o", label=f"{T} threads (mean)")
        plt.fill_between(Ns, S_lower, S_upper, alpha=0.2)

    plt.axhline(1.0, color="gray", linestyle="--", label="S = 1 (без ускорения)")
    plt.xlabel("Размер массива N")
    plt.ylabel("Параллельное ускорение S(N) = T_seq / T_par")
    plt.title("Параллельное ускорение: среднее и CI95%")
    plt.xscale("log")
    plt.grid(True, which="both", ls="--", alpha=0.5)
    plt.legend(title="Потоки")
    plt.tight_layout()
    plt.savefig(OUT_PNG, dpi=150)
    plt.close()
    print(f"Saved: {OUT_PNG}")


if __name__ == "__main__":
    main()
