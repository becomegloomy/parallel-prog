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
OUT_UPLOAD = os.path.join(ROOT_DIR, "upload_vs_kernel.png")
OUT_TIME   = os.path.join(ROOT_DIR, "throughput.png")

STAGES = ["T_upload", "T_map_m1", "T_map_m2", "T_download"]


def read_results(path):
    """→ dict[N] = {stage: mean_seconds}."""
    data = defaultdict(lambda: {s: [] for s in STAGES + ["T_total"]})
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            N = int(row["N"])
            for s in STAGES + ["T_total"]:
                data[N][s].append(float(row[s]))
    return {N: {s: float(np.mean(v)) for s, v in d.items()} for N, d in data.items()}


def plot_upload_vs_kernel(data):
    Ns = sorted(data)
    fractions = {s: [] for s in STAGES}
    for N in Ns:
        total = data[N]["T_total"] or 1.0
        for s in STAGES:
            fractions[s].append(data[N][s] / total)

    plt.figure(figsize=(10, 6))
    plt.stackplot(Ns, [fractions[s] for s in STAGES], labels=STAGES)
    plt.xlabel("Размер массива N")
    plt.ylabel("Доля времени в T_total")
    plt.title("Доли T_upload / T_map_m1 / T_map_m2 / T_download")
    plt.xscale("log")
    plt.legend(loc="upper left")
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig(OUT_UPLOAD, dpi=150)
    plt.close()
    print(f"Saved: {OUT_UPLOAD}")


def plot_throughput(data):
    Ns = sorted(data)
    t  = [data[N]["T_total"] for N in Ns]

    plt.figure(figsize=(10, 6))
    plt.plot(Ns, t, marker="o", label="T_total")
    plt.plot(Ns, [data[N]["T_upload"]   for N in Ns], marker="s", label="T_upload")
    plt.plot(Ns, [data[N]["T_map_m1"]   for N in Ns], marker="^", label="T_map_m1")
    plt.plot(Ns, [data[N]["T_map_m2"]   for N in Ns], marker="v", label="T_map_m2")
    plt.plot(Ns, [data[N]["T_download"] for N in Ns], marker="d", label="T_download")
    plt.xlabel("Размер массива N")
    plt.ylabel("Время, с")
    plt.title("Время этапов OpenCL vs N")
    plt.xscale("log")
    plt.yscale("log")
    plt.grid(True, which="both", ls="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()
    plt.savefig(OUT_TIME, dpi=150)
    plt.close()
    print(f"Saved: {OUT_TIME}")


def main():
    if not os.path.isfile(CSV_IN):
        print(f"Error: {CSV_IN} not found (run scripts/run.py first)", file=sys.stderr)
        sys.exit(1)
    data = read_results(CSV_IN)
    plot_upload_vs_kernel(data)
    plot_throughput(data)


if __name__ == "__main__":
    main()
