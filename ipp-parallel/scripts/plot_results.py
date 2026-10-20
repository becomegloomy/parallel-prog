import os
import sys
import numpy as np
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR   = os.path.dirname(SCRIPT_DIR)
CSV_IN     = os.path.join(ROOT_DIR, "results", "results.csv")
OUT_DIR    = os.path.join(ROOT_DIR, "results")
os.makedirs(OUT_DIR, exist_ok=True)

if not os.path.isfile(CSV_IN):
    print(f"Error: {CSV_IN} not found (run run_all.sh first)", file=sys.stderr)
    sys.exit(1)

df = pd.read_csv(CSV_IN)

pivot = df.pivot_table(index="N", columns="M", values="Milliseconds", aggfunc="mean")
pivot = pivot.sort_index()
N_values = pivot.index.to_numpy()
M_values = sorted(pivot.columns)

if 1 not in pivot.columns:
    print("Error: baseline M=1 not found in results.csv", file=sys.stderr)
    sys.exit(1)

baseline = pivot[1].to_numpy()

# 1. Время выполнения
plt.figure(figsize=(8, 6))
for M in M_values:
    plt.plot(N_values, pivot[M].to_numpy(), marker="o", label=f"M={M}")
plt.xlabel("N (размер массива)")
plt.ylabel("Время выполнения, ms")
plt.title("Время выполнения программы")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(os.path.join(OUT_DIR, "time_execution.png"), dpi=150)
plt.close()

# 2. Параллельное ускорение
plt.figure(figsize=(8, 6))
for M in M_values:
    if M == 1:
        speedup = np.ones_like(baseline, dtype=float)
    else:
        tM = pivot[M].to_numpy()
        # Защита от деления на 0
        speedup = np.divide(baseline, tM,
                            out=np.full_like(baseline, np.nan, dtype=float),
                            where=tM > 0)
    plt.plot(N_values, speedup, marker="o", label=f"M={M}")
plt.xlabel("N (размер массива)")
plt.ylabel("Параллельное ускорение S(N) = T1 / TM")
plt.title("Параллельное ускорение")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(os.path.join(OUT_DIR, "speedup.png"), dpi=150)
plt.close()

# 3. Параллельная эффективность
plt.figure(figsize=(8, 6))
for M in M_values:
    if M == 1:
        eff = np.ones_like(baseline, dtype=float)
    else:
        tM = pivot[M].to_numpy()
        speedup = np.divide(baseline, tM,
                            out=np.full_like(baseline, np.nan, dtype=float),
                            where=tM > 0)
        eff = speedup / M
    plt.plot(N_values, eff, marker="o", label=f"M={M}")
plt.xlabel("N (размер массива)")
plt.ylabel("Параллельная эффективность E(N) = S(N) / M")
plt.title("Параллельная эффективность")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(os.path.join(OUT_DIR, "efficiency.png"), dpi=150)
plt.close()

print(f"Графики сохранены в {OUT_DIR}/")
