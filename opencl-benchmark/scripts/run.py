#!/usr/bin/env python3

import csv
import os
import re
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR   = os.path.dirname(SCRIPT_DIR)
BIN        = os.path.join(ROOT_DIR, "opencl-bench")
OUT        = os.path.join(ROOT_DIR, "results.csv")

N_LIST   = [10000, 50000, 100000, 200000, 400000, 522000]
REPEATS  = 10
STAGES   = ["T_upload", "T_map_m1", "T_map_m2", "T_download", "T_total"]
TOTAL_RE = re.compile(r"Milliseconds passed:\s*(\d+)")


def run_one(N: int) -> dict:
    proc = subprocess.run([BIN, str(N)], capture_output=True, text=True)
    if proc.returncode != 0:
        print(f"Error: {BIN} exited {proc.returncode} for N={N}", file=sys.stderr)
        print(proc.stderr, file=sys.stderr)
        sys.exit(1)
    out = proc.stdout
    rec = {}
    for stage in STAGES:
        m = re.search(rf"^{stage}=([0-9.]+)$", out, re.MULTILINE)
        if not m:
            print(f"Error: cannot parse {stage} for N={N}", file=sys.stderr)
            print(out, file=sys.stderr)
            sys.exit(1)
        rec[stage] = float(m.group(1))
    m = TOTAL_RE.search(out)
    if not m:
        print(f"Error: cannot parse Total for N={N}", file=sys.stderr)
        print(out, file=sys.stderr)
        sys.exit(1)
    rec["Total_ms"] = int(m.group(1))
    return rec


def main():
    if not os.path.isfile(BIN):
        print(f"Error: {BIN} not found (run 'make' first)", file=sys.stderr)
        sys.exit(1)

    with open(OUT, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["N", "Repeat", "T_upload", "T_map_m1",
                    "T_map_m2", "T_download", "T_total", "Total_ms"])
        for N in N_LIST:
            for r in range(REPEATS):
                print(f"Running N={N}, rep={r+1}/{REPEATS} ...")
                rec = run_one(N)
                w.writerow([N, r + 1,
                            f"{rec['T_upload']:.6f}",
                            f"{rec['T_map_m1']:.6f}",
                            f"{rec['T_map_m2']:.6f}",
                            f"{rec['T_download']:.6f}",
                            f"{rec['T_total']:.6f}",
                            rec["Total_ms"]])

    print(f"Results saved to {OUT}")


if __name__ == "__main__":
    main()
