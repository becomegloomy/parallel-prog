# ipp-omp-benchmark

Сравнение производительности двух реализаций одной задачи обработки массивов:

- **`ipp-parallel`** — последовательная версия на Intel IPP (`ippsSqrt_64f`, `ippsExp_64f`, `ippsDiv_64f`, …)
- **`omp-parallel`** — параллельная версия на OpenMP (`#pragma omp parallel for`, `reduction`)

Плюс профилирование загрузки CPU через `mpstat` при разном числе потоков OpenMP. Цель — измерить реальное ускорение от OpenMP относительно IPP-baseline, оценить Amdahl-ограничение и увидеть, как насыщаются ядра при `M > K`.

## Задача

5 независимых экспериментов (`ITERATIONS = 5`). В каждом над массивами `M1` (размер `N`) и `M2` (размер `N/2`) выполняются этапы:

| Этап | Формула / примечание |
|---|---|
| **Generate** | `M1[j] ∈ [1, A]`, `M2[j] ∈ [A, 10A]`, `A = 360`, `rand_r` с `seed = iter` |
| **Map (M1)** | `M1[j] = coth(sqrt(M1[j]))` |
| **Map (M2)** | `M2[j] = sqrt((M2_copy[j] + M2_copy[j-1]) * e)`, `M2_copy[-1] = 0` |
| **Merge** | `M2[j] = M1[j] / M2[j]` для `j < min(N, N/2)` |
| **Sort** | Comb Sort, `gap /= 1.2473309` |
| **Reduce** | `X = Σ sin(M2[j])`, если `(int)(M2[j] / min_nonzero) % 2 == 0` |

Итог: `X_final = Σ X_iter` по 5 экспериментам.

## Что параллелится в `omp-parallel`

| Этап | Реализация | Почему |
|---|---|---|
| Generate | последовательно | `rand_r` с общим `seed` даёт гонку; для воспроизводимости `X` с baseline — последовательно |
| Map (M1) | `#pragma omp parallel for` | независимые итерации |
| Map (M2) | `#pragma omp parallel for` | **ключевой момент**: `prev = M2_copy[j-1]`, а `M2_copy` не меняется → гонки нет. Если бы было `M2[j-1]` — была бы зависимость по данным |
| Merge | `#pragma omp parallel for` | независимые итерации |
| Sort | **последовательно** | Comb Sort плохо параллелится (swap с зависимостью). Это узкое место — Amdahl-ограничение |
| Reduce | `#pragma omp parallel for reduction(+:X)` | стандартный `reduction` |
| `min_nonzero` | последовательно | первый ненулевой; на `N/2 = 260k` быстрее последовательно |

## Требования

- GCC ≥ 9 с OpenMP
- Intel oneAPI Base Toolkit (IPP) — <https://www.intel.com/content/www/us/en/developer/tools/oneapi/base-toolkit-download.html>
- Python 3 (`pandas`, `matplotlib`, `numpy`), `sysstat` (`mpstat`) — для профилирования

## Запуск

```bash
make            # сборка ipp-parallel и omp-parallel
make bench      # → results/results.csv
make overhead   # → results/cpu_M{4,8,12,16,20}.csv (требует mpstat)
make plots      # → time_execution.png, speedup.png, efficiency.png, cpu_usage.png
```

- `make bench`: `N` от 300 до 522000 (11 точек), `M` от 1 до 8.
- `make overhead`: `M ∈ {4, 8, 12, 16, 20}`, `mpstat -P ALL 1 20` в фоне + `omp-parallel` при `N = 463500`.

## Ожидаемые результаты

Типичные цифры (8 ядер, `N = 522000`):

| `M` | Время, ms | Speedup | Efficiency |
|---|---|---|---|
| 1 | ~280 | 1.00 | 1.00 |
| 2 | ~150 | ~1.85 | ~0.93 |
| 4 | ~90  | ~3.10 | ~0.78 |
| 8 | ~70  | ~4.00 | ~0.50 |

Speedup насыщается при `M > числа ядер` — Amdahl (Sort последовательный). Цифры зависят от железа; форма кривой сохранится.

## Ограничения

1. **Sort последовательный** → Amdahl: при больших `N` Sort доминирует, speedup насыщается.
2. **`N = 300` → 0 ms** → `speedup` не определён (`NaN` в `plot_results.py`, скрипт не падает).
3. **`ipp-parallel` — baseline.** `OMP_NUM_THREADS` на него не влияет; реальное ускорение — в `omp-parallel`.
4. **`M > K` — переподписка**: все ядра ≈ 100 %, но ускорения нет.
5. **`N2` разный**: `run_all.sh` — 522000, `run_overhead.sh` — 463500.

## Структура

```
ipp-omp-benchmark/
├── README.md
├── Makefile
├── .gitignore
├── src/
│   ├── ipp-parallel.c     # последовательная (IPP)
│   └── omp-parallel.c     # параллельная (OpenMP)
└── scripts/
    ├── run_all.sh         # бенчмарк seq vs M
    ├── run_overhead.sh    # профилирование CPU
    ├── parse.py           # mpstat .txt → .csv
    ├── plot_results.py    # time_execution / speedup / efficiency
    └── plot_results2.py   # cpu_usage

```

