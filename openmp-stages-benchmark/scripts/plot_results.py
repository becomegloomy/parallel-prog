#!/usr/bin/env python3
"""Графики по results.csv и cpu_usage.csv → *.png."""

import csv
import os
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR   = os.path.dirname(SCRIPT_DIR)

RESULTS_CSV = os.path.join(ROOT_DIR, "results.csv")
CPU_CSV     = os.path.join(ROOT_DIR, "cpu_usage.csv")
OUT_DIR     = ROOT_DIR


def read_results(path):
    threads, times = [], []
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            threads.append(int(row["Threads"]))
            times.append(float(row["Time_ms"]))
    return threads, times


def plot_speedup(threads, times, out_png):
    t1 = times[threads.index(1)]
    speedup = [t1 / t for t in times]

    plt.figure(figsize=(8, 6))
    plt.plot(threads, speedup, marker="o", label="S(M) = T1 / TM")
    plt.plot(threads, threads, linestyle="--", color="gray", label="идеал: S(M) = M")
    plt.xlabel("Число потоков M")
    plt.ylabel("Ускорение S(M)")
    plt.title("Параллельное ускорение (OpenMP)")
    plt.xticks(threads)
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_png, dpi=150)
    plt.close()
    print(f"Saved: {out_png}")


def plot_time(threads, times, out_png):
    plt.figure(figsize=(8, 6))
    plt.plot(threads, times, marker="o")
    plt.xlabel("Число потоков M")
    plt.ylabel("Время выполнения, ms")
    plt.title("Время выполнения vs число потоков")
    plt.xticks(threads)
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(out_png, dpi=150)
    plt.close()
    print(f"Saved: {out_png}")


def plot_cpu(path, out_png):
    timestamps, avg = [], []
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        core_cols = [c for c in reader.fieldnames if c.startswith("Core")]
        for row in reader:
            timestamps.append(float(row["Time"]))
            vals = [float(row[c]) for c in core_cols]
            avg.append(sum(vals) / len(vals))

    plt.figure(figsize=(10, 5))
    plt.plot(timestamps, avg, label="Средняя загрузка по ядрам")
    plt.xlabel("Время, с")
    plt.ylabel("Загрузка CPU, %")
    plt.title("Загрузка CPU во время работы программы")
    plt.ylim(0, 105)
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_png, dpi=150)
    plt.close()
    print(f"Saved: {out_png}")


def main():
    if not os.path.isfile(RESULTS_CSV):
        print(f"Error: {RESULTS_CSV} not found (run 'make bench' first)", file=sys.stderr)
        sys.exit(1)

    threads, times = read_results(RESULTS_CSV)
    plot_time(threads, times,     os.path.join(OUT_DIR, "time_execution.png"))
    plot_speedup(threads, times,  os.path.join(OUT_DIR, "speedup.png"))

    if os.path.isfile(CPU_CSV):
        plot_cpu(CPU_CSV, os.path.join(OUT_DIR, "cpu_usage.png"))
    else:
        print(f"Warning: {CPU_CSV} not found, skipping CPU plot "
              f"(run 'make cpu' first)", file=sys.stderr)


if __name__ == "__main__":
    main()
