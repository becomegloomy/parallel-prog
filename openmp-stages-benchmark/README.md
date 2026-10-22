# OpenMP per-stage benchmark

Сравнение последовательной и параллельной версий C-программы с **per-stage таймингами**: время замеряется отдельно на каждом этапе обработки массивов, поэтому видно, какие этапы ускоряются от OpenMP, а какие остаются последовательными и ограничивают общий выигрыш.

Цель - измерить ускорение от OpenMP, разложить его по этапам.

### Распараллеливание

**Параллельно:** Generate M1, Generate M2, Map (M1), Merge, Reduce - независимые итерации, `#pragma omp parallel for`.

**Последовательно:**
- **Map (M2)** - рекуррентная зависимость;
- **Sort** - Comb Sort: обмены зависят друг от друга;
- **поиск `min_nonzero`** - первый ненулевой элемент, простой проход.

Именно эти последовательные этапы ограничивают общее ускорение - это и видно на per-stage таймингах.

## Требования

- GCC ≥ 9 с поддержкой OpenMP (`-fopenmp`);
- Python 3 с пакетами `psutil`, `matplotlib`.

Установка Python-зависимостей:

```bash
pip install -r requirements.txt
```

## Запуск

```bash
make            # сборка omp-parallel-stages
make bench      # замер времени при M = 1, 2, 4, 8 → results.csv
make cpu        # замер загрузки CPU → cpu_usage.csv
make plots      # графики зависимости → time_execution.png, speedup.png, cpu_usage.png
```

- `time_execution.png` - время от числа потоков `M`;
- `speedup.png` - ускорение `S(M) = T(1)/T(M)` от `M`;
- `cpu_usage.png` - загрузка CPU во времени.

### Ручная сборка

```bash
gcc -O2 -fopenmp -Wall -Wextra -std=c11 src/omp-parallel-stages.c -lm -o omp-parallel-stages
```

### Ручной запуск

```bash
OMP_NUM_THREADS=4 ./omp-parallel-stages 1000000
./omp-parallel-stages 1000000 4
```

## Что ожидать на графиках

2 ядра, `N = 1000000`:

| Потоков `M` | Время, ms | Ускорение |
|---|---|---|
| 1 | ~405 | 1.00 |
| 2 | ~389 | ~1.04 |
| 4 | ~512 | ~0.79 |
| 8 | ~905 | ~0.45 |

**Ускорение** - во сколько раз `M` потоков быстрее одного: `S(M) = T(1) / T(M)`.

Ключевое наблюдение: **общее ускорение малое**, потому что:

- **Map (M2)** и **Sort** выполняются последовательно и занимают много времени;
- на 2 ядрах ускорение насыщается уже при `M = 2`, а при `M > числа ядер` **деградирует** из-за накладных расходов на потоки.

## Структура проекта

```
openmp-stages-benchmark/
├── README.md
├── Makefile
├── requirements.txt
├── src/
│   └── omp-parallel-stages.c   # исходник с per-stage таймингами
└── scripts/
    ├── speedup.py              # M = 1,2,4,8 → results.csv
    ├── cpu.py                  # загрузка CPU → cpu_usage.csv
    └── plot_results.py         # графики → *.png
```
