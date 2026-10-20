#!/usr/bin/env python3
"""Замер загрузки CPU во время работы omp-parallel-stages → cpu_usage.csv."""

import csv
import os
import subprocess
import sys
import time

import psutil

BIN     = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "omp-parallel-stages")
BIN     = os.path.normpath(BIN)
N       = 1_000_000
THREADS = 2           
OUT     = "cpu_usage.csv"
INTERVAL = 0.1        


def main():
    if not os.path.isfile(BIN):
        print(f"Error: {BIN} not found (run 'make' first)", file=sys.stderr)
        sys.exit(1)

    env = os.environ.copy()
    env["OMP_NUM_THREADS"] = str(THREADS)

    proc = subprocess.Popen([BIN, str(N)], env=env,
                            stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL)

    timestamps = []
    snapshots  = []
    start = time.time()
    while proc.poll() is None:
        snapshots.append(psutil.cpu_percent(interval=INTERVAL, percpu=True))
        timestamps.append(time.time() - start)

    proc.wait()

    if not snapshots:
        print("Warning: no CPU samples collected", file=sys.stderr)
        sys.exit(1)

    n_cores = len(snapshots[0])
    with open(OUT, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["Time"] + [f"Core{i}" for i in range(n_cores)])
        for t, cpu in zip(timestamps, snapshots):
            w.writerow([f"{t:.3f}"] + [f"{c:.1f}" for c in cpu])

    print(f"CPU usage saved to {OUT} ({len(snapshots)} samples, {n_cores} cores)")


if __name__ == "__main__":
    main()
