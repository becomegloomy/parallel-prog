import pandas as pd
import matplotlib.pyplot as plt

# Загрузка данных из CSV
df = pd.read_csv("sample_results.csv")

# Список K потоков (столбцы par-K)
K_values = [col for col in df.columns if col.startswith("par-")]

# График 1: Время выполнения
plt.figure(figsize=(10, 6))
plt.plot(df['N'], df['seq'], marker='o', label='seq')
for K in K_values:
    plt.plot(df['N'], df[K], marker='o', label=K)
plt.xlabel("Размер массива N")
plt.ylabel("Время выполнения (ms)")
plt.title("Время выполнения: seq vs par-K")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("time_execution.png", dpi=120)
plt.close()

# График 2: Параллельное ускорение
plt.figure(figsize=(10, 6))
for K in K_values:
    speedup = df['seq'] / df[K]
    plt.plot(df['N'], speedup, marker='o', label=f"Speedup {K}")
plt.xlabel("Размер массива N")
plt.ylabel("Параллельное ускорение S_K(N)")
plt.title("Параллельное ускорение")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("speedup.png", dpi=120)
plt.close()

# Сохранение ускорений в CSV
speedup_df = pd.DataFrame({'N': df['N']})
for K in K_values:
    speedup_df[f"S_{K}"] = df['seq'] / df[K]
speedup_df.to_csv("speedup.csv", index=False)
print("Графики построены: time_execution.png, speedup.png")
print("Ускорения сохранены: speedup.csv")
