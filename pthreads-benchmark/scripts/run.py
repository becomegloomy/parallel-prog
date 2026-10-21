#!/usr/bin/env python3
"""Прогон pthreads-bench по сетке (N, T) → results.csv."""

import csv
import os
import re
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR   = os.path.dirname(SCRIPT_DIR)
BIN        = os.path.join(ROOT_DIR, "pthreads-bench")
OUT        = os.path.join(ROOT_DIR, "results.csv")

N_LIST      = [10000, 50000, 100000, 200000, 400000, 522000]
THREADS     = [1, 2, 4, 8, 16]
STAGES      = ["Generate_M1", "Generate_M2", "Map_M1", "Map_M2",
               "Merge", "Sort", "Reduce"]
TIME_RE     = re.compile(r"Milliseconds passed:\s*(\d+)")


def run_one(N: int, T: int) -> dict:
    """Возвращает {stage: seconds, 'Total_ms': int} для одного запуска."""
    proc = subprocess.run([BIN, str(N), str(T)],
                          capture_output=True, text=True)
    if proc.returncode != 0:
        print(f"Error: {BIN} exited {proc.returncode} for N={N}, T={T}",
              file=sys.stderr)
        print(proc.stderr, file=sys.stderr)
        sys.exit(1)

    out = proc.stdout
    result = {}
    for stage in STAGES:
        m = re.search(rf"^{stage}:\s*([0-9.]+)$", out, re.MULTILINE)
        result[stage] = float(m.group(1)) if m else 0.0

    m = TIME_RE.search(out)
    if not m:
        print(f"Error: cannot parse Total for N={N}, T={T}", file=sys.stderr)
        print(out, file=sys.stderr)
        sys.exit(1)
    result["Total_ms"] = int(m.group(1))
    return result


def main():
    if not os.path.isfile(BIN):
        print(f"Error: {BIN} not found (run 'make' first)", file=sys.stderr)
        sys.exit(1)

    with open(OUT, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["N", "Threads", "Generate_M1", "Generate_M2", "Map_M1",
                    "Map_M2", "Merge", "Sort", "Reduce", "Total_ms"])
        for N in N_LIST:
            for T in THREADS:
                print(f"Running N={N}, T={T} ...")
                r = run_one(N, T)
                w.writerow([N, T,
                            f"{r['Generate_M1']:.6f}",
                            f"{r['Generate_M2']:.6f}",
                            f"{r['Map_M1']:.6f}",
                            f"{r['Map_M2']:.6f}",
                            f"{r['Merge']:.6f}",
                            f"{r['Sort']:.6f}",
                            f"{r['Reduce']:.6f}",
                            r["Total_ms"]])

    print(f"Results saved to {OUT}")


if __name__ == "__main__":
    main()
