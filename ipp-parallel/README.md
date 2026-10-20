# ipp-parallel

Параллельная реализация лабораторной работы: сравнение **Intel IPP** (последовательный baseline) и **OpenMP** (параллельная версия) на одной и той же задаче обработки массивов.

---

## Задача

Программа выполняет 5 независимых экспериментов (`ITERATIONS = 5`). В каждом эксперименте над массивами `M1` (размер `N`) и `M2` (размер `N/2`) выполняются этапы:

| Этап | Что делает | Формула / примечание |
|---|---|---|
| **Generate** | Заполнение `M1` и `M2` случайными числами | `M1[j] ∈ [1, A]`, `M2[j] ∈ [A, 10A]`, `A = 360`, `rand_r` с `seed = iter` |
| **Map (M1)** | Гиперболический котангенс от корня | `M1[j] = coth(sqrt(M1[j]))` |
| **Map (M2)** | Зависимость от предыдущего элемента **исходного** массива | `M2[j] = sqrt((M2_copy[j] + M2_copy[j-1]) * e)`, `M2_copy[-1] = 0` |
| **Merge** | Поэлементное деление | `M2[j] = M1[j] / M2[j]` для `j < min(N, N/2)` |
| **Sort** | Сортировка расчёской | Comb Sort, `gap /= 1.2473309` |
| **Reduce** | Сумма синусов при чётном частном | `X = Σ sin(M2[j])`, если `(int)(M2[j] / min_nonzero) % 2 == 0` |

Итоговое значение: `X_final = Σ X_iter` по 5 экспериментам.

---

## Две реализации

| Бинарник | Исходник | Технология | Назначение |
|---|---|---|---|
| `ipp-parallel` | `src/ipp-parallel.c` | Intel IPP (`ippsSqrt_64f`, `ippsExp_64f`, `ippsDiv_64f`, …) | Последовательный **baseline** |
| `omp-parallel`  | `src/omp-parallel.c`  | OpenMP (`#pragma omp parallel for`, `reduction`) | **Параллельная** версия |

> **Важно:** `ipp-parallel` — последовательный. `OMP_NUM_THREADS` на него **не влияет** (IPP сам решает, сколько потоков использовать; для `ipps*_64f` — 1). Реальное ускорение — в `omp-parallel`.

---

## Требования

- **GCC** ≥ 9 с поддержкой OpenMP
- **Intel oneAPI Base Toolkit** (для IPP)
  — установка: <https://www.intel.com/content/www/us/en/developer/tools/oneapi/base-toolkit-download.html>
- **Python 3** + пакеты: `pandas`, `matplotlib`, `numpy`
- **sysstat** (`mpstat`) — для профилирования загрузки CPU
- (опционально) `LC_ALL=C` при запуске `mpstat` — чтобы `parse.py` не спотыкался о локализованное `Average:`

Проверить IPP:

```bash
find / -name "ipps.h" 2>/dev/null
```

Если oneAPI установлен не в `/opt/intel/oneapi`, поправь `IPP_INC` и `IPP_LIB` в `Makefile`.

---

## Сборка

```bash
make            # собирает ipp-parallel и omp-parallel
```

Или по отдельности:

```bash
make ipp-parallel
make omp-parallel
```

---

## Запуск

### Бенчмарк: время выполнения (seq baseline vs M потоков)

```bash
make bench      # → results/results.csv  (~25 сек)
```

Что делает `scripts/run_all.sh`:
- `N` от `300` до `522000`, 11 точек (`POINTS = 10`)
- `M` от `1` до `8` (`K = 8`)
- для каждой пары `(N, M)` запускает `ipp-parallel`, парсит `Milliseconds passed`, пишет в CSV

> ⚠️ `ipp-parallel` — последовательный, поэтому `speedup` в `results.csv` будет ≈ 1 для всех `M`. Это **baseline**. Реальное ускорение смотри в `omp-parallel` (см. ниже).

### Профилирование загрузки CPU (OpenMP-версия)

```bash
make overhead   # → results/cpu_M{4,8,12,16,20}.csv  (~100 сек)
```

Что делает `scripts/run_overhead.sh`:
- для `M ∈ {4, 8, 12, 16, 20}` (больше числа ядер — **переподписка**)
- запускает `mpstat -P ALL 1 20` в фоне
- параллельно запускает `omp-parallel` при `N = 463500`
- парсит вывод `mpstat` через `parse.py` → `results/cpu_M{M}.csv`

> Требует `mpstat` (пакет `sysstat`). Если его нет — `make overhead` упадёт с `Error: mpstat not found`.

### Построение графиков

```bash
make plots      # → results/time_execution.png, speedup.png, efficiency.png, cpu_usage.png
```

Или по отдельности:

```bash
python3 scripts/plot_results.py    # time_execution.png, speedup.png, efficiency.png
python3 scripts/plot_results2.py   # cpu_usage.png
```

---

## Ожидаемые результаты

| График | Что показывает | На что смотреть |
|---|---|---|
| `time_execution.png` | Время выполнения от `N` для разных `M` | Рост с `N`, слабое падение с `M` (Sort доминирует) |
| `speedup.png` | `S(N) = T1 / TM` | Насыщается при `M > числа ядер` — Amdahl-ограничение |
| `efficiency.png` | `E(N) = S(N) / M` | Падает с ростом `M` — переподписка и последовательный Sort |
| `cpu_usage.png` | Загрузка `CPU=all` (%) во времени | При `M > K` все ядра ≈ 100 % (переподписка) |

**Типичные цифры** (на 8-ядерной машине, `N = 522000`):

| `M` | Время, ms | Speedup | Efficiency |
|---|---|---|---|
| 1 | ~280 | 1.00 | 1.00 |
| 2 | ~150 | ~1.85 | ~0.93 |
| 4 | ~90  | ~3.10 | ~0.78 |
| 8 | ~70  | ~4.00 | ~0.50 |

> Цифры зависят от железа. Если у тебя другое число ядер — графики будут другими, но форма (насыщение speedup) сохранится.

---

## Ограничения и известные особенности

1. **Comb Sort — последовательный.** Comb Sort плохо параллелится (зависимость по данным в swap). При больших `N` он доминирует, поэтому `speedup` насыщается — это **ожидаемое** поведение (Amdahl).
2. **`N = 300` → 0 ms.** `gettimeofday` даёт миллисекунды, а `N = 300` отрабатывает быстрее 1 ms. Для этой точки `speedup` математически не определён (`0/0`); `plot_results.py` через `np.divide(where=tM>0)` даёт `NaN` и не падает.
3. **`ipp-parallel` — baseline.** `OMP_NUM_THREADS` на него не влияет. Если хочешь честное сравнение seq vs par в одном CSV — надо переделать `run_all.sh`, чтобы он гонял оба бинарника и писал колонку `impl`.
4. **`N2` разный.** В `run_all.sh` — `N2 = 522000`, в `run_overhead.sh` — `N2 = 463500`. Профилирование CPU делалось при `N = 463500`, близком к `N2`; на графике времени максимум `N = 522000`.
5. **`M > K` — переподписка.** В `run_overhead.sh` `M = 4..20`, что больше числа ядер `K`. При `M > K` `OMP_NUM_THREADS` создаёт больше потоков, чем ядер; загрузка `CPU=all` ≈ 100 %, но ускорения нет. Это иллюстрирует, что параллелизм не бесплатен.
6. **`sysstat` версия.** `parse.py` берёт `%idle` **по имени столбца** из заголовка `mpstat`. Это работает для `sysstat` ≥ 11 (порядок столбцов разный в старых версиях). Зафиксируй версию: `mpstat -V`.

---

## Структура репозитория

```
ipp-parallel/
├── README.md              # этот файл
├── Makefile               # сборка ipp-parallel и omp-parallel
├── .gitignore             # бинарники, *.o, results/cpu_M*.txt
├── src/
│   ├── ipp-parallel.c     # последовательная версия (Intel IPP)
│   └── omp-parallel.c     # параллельная версия (OpenMP)
├── scripts/
│   ├── run_all.sh         # бенчмарк seq vs M потоков
│   ├── run_overhead.sh    # профилирование CPU через mpstat
│   ├── parse.py           # mpstat .txt → .csv
│   ├── plot_results.py    # time_execution / speedup / efficiency
│   └── plot_results2.py   # cpu_usage
└── results/
    ├── results.csv        # бенчмарк (генерируется make bench)
    ├── cpu_M*.csv         # профилирование (генерируется make overhead)
    └── *.png              # графики (генерируется make plots)
```

---

## Воспроизведение с нуля

```bash
# 1. Собрать
make

# 2. Бенчмарк времени (быстро, ~25 сек)
make bench

# 3. Профилирование CPU (медленнее, ~100 сек, требует mpstat)
make overhead

# 4. Построить графики
make plots

# 5. Открыть results/*.png
```

---

## Лицензия

Учебный проект. Intel IPP — проприетарный, распространяется по лицензии Intel (см. установщик oneAPI). Остальной код — свободно для учебных целей.
