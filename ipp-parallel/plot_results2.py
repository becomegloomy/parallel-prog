import os
import sys
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

# Список используемых потоков
M_values = [4, 8, 12, 16, 20]

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR   = os.path.dirname(SCRIPT_DIR)
RESULTS_DIR = os.path.join(ROOT_DIR, "results")
OUT_PNG      = os.path.join(RESULTS_DIR, "cpu_usage.png")

plt.figure(figsize=(12, 6))

plotted = 0
missing = []

for M in M_values:
    csv_path = os.path.join(RESULTS_DIR, f"cpu_M{M}.csv")
    if not os.path.isfile(csv_path):
        missing.append(M)
        continue

    df = pd.read_csv(csv_path)
    if "CPU" not in df.columns or "Time" not in df.columns or "Usage" not in df.columns:
        print(f"Warning: {csv_path} has unexpected columns: {list(df.columns)}",
              file=sys.stderr)
        continue

    cpu_all_df = df[df["CPU"] == "all"]
    if cpu_all_df.empty:
        print(f"Warning: {csv_path} has no CPU='all' rows", file=sys.stderr)
        continue

    plt.plot(cpu_all_df["Time"], cpu_all_df["Usage"],
             marker="o", label=f"M={M}")
    plotted += 1

if plotted == 0:
    print("Error: no data to plot (run run_overhead.sh first)", file=sys.stderr)
    sys.exit(1)

if missing:
    print(f"Warning: skipped missing files for M={missing}", file=sys.stderr)

plt.xlabel("Время (измерения)")
plt.ylabel("Загрузка CPU (%)")
plt.title("Суммарная загрузка всех ядер при N=N2 для разных M")
plt.legend(fontsize=10)
plt.grid(True)
plt.tight_layout()
plt.savefig(OUT_PNG, dpi=150)
plt.close()
print(f"График сохранён: {OUT_PNG}")
