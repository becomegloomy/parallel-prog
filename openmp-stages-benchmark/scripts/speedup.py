#!/usr/bin/env python3
"""Замер времени omp-parallel-stages при разном числе потоков → results.csv."""

import csv
import os
import re
import subprocess
import sys

BIN         = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "omp-parallel-stages")
BIN         = os.path.normpath(BIN)
N           = 1_000_000
THREADS     = [1, 2, 4, 8]
OUT         = "results.csv"

TIME_RE = re.compile(r"Milliseconds passed:\s*(\d+)")


def run(threads: int) -> int:
    env = os.environ.copy()
    env["OMP_NUM_THREADS"] = str(threads)
    proc = subprocess.run(
        [BIN, str(N)],
        capture_output=True, text=True, env=env,
    )
    if proc.returncode != 0:
        print(f"Error: {BIN} exited with {proc.returncode}", file=sys.stderr)
        print(proc.stderr, file=sys.stderr)
        sys.exit(1)
    m = TIME_RE.search(proc.stdout)
    if not m:
        print(f"Error: cannot parse time for M={threads}", file=sys.stderr)
        print(proc.stdout, file=sys.stderr)
        sys.exit(1)
    return int(m.group(1))


def main():
    if not os.path.isfile(BIN):
        print(f"Error: {BIN} not found (run 'make' first)", file=sys.stderr)
        sys.exit(1)

    results = {}
    for m in THREADS:
        print(f"Running with M={m} threads ...")
        ms = run(m)
        results[m] = ms
        print(f"  time = {ms} ms")

    t1 = results[1]
    with open(OUT, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["Threads", "Time_ms", "Speedup"])
        for m in THREADS:
            speedup = t1 / results[m] if results[m] > 0 else float("nan")
            w.writerow([m, results[m], f"{speedup:.4f}"])

    print(f"\nResults saved to {OUT}")


if __name__ == "__main__":
    main()
