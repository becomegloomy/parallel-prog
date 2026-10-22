# Pthreads benchmark

Сравнение последовательной и параллельной версий C-программы, где параллелизм реализован **вручную через потоки - pthreads**. Плюс поэтапные per-stage тайминги и график stackplot долей времени по этапам.

Цель - показать, как ручные потоки справляются с задачей, и что даёт распараллеливание только части этапов.

### Распараллеливание

**Параллельно:**
- **Generate, Map (M1), Merge, Reduce** - массив делится на `T` кусков (`split()`), каждый поток обрабатывает свой;
- **Sort** - две половины `M2` сортируются двумя pthread-задачами.

**Последовательно:**
- **копия `M2 → M2_copy`** - простой цикл;
- **Map (M2)** - рекуррентная зависимость;
- **поиск `min_nonzero`** - первый ненулевой элемент, простой проход;
- **`merge_halves`** - слияние двух отсортированных половин, последовательный алгоритм.

## Требования

- GCC ≥ 9 с поддержкой pthreads;
- Python 3 с пакетами `matplotlib`, `numpy`;
- Bash .

Установка Python-зависимостей:

```bash
pip install -r requirements.txt
```

## Запуск

```bash
make            # сборка pthreads-bench
make bench      # прогон сетки (N, T), N - размер  массива, T - кол-во потоков → results.csv
make plots      # графики → speedup.png, stages_fraction.png
```

- `speedup.png` - ускорение `S(N, T) = Total_ms(N,1) / Total_ms(N,T)` от `T` для каждого `N`;
- `stages_fraction.png` - stackplot долей времени этапов для `T = max`.

### Ручная сборка

```bash
gcc -O2 -Wall -Wextra -std=c11 -pthread src/pthreads-bench.c -lm -pthread -o pthreads-bench
```

### Ручной запуск

```bash
./pthreads-bench 522000 4      
OMP_NUM_THREADS=4 ./pthreads-bench 522000
```

## Что ожидать на графиках

2 ядра, `N = 522000`:

| Потоков `T` | Время, ms | Ускорение |
|---|---|---|
| 1 | ~420 | 1.00 |
| 2 | ~400 | ~1.05 |
| 4 | ~430 | ~0.98 |
| 8 | ~480 | ~0.88 |
| 16 | ~520 | ~0.81 |

**Ускорение** - во сколько раз `T` потоков быстрее одного: `S(T) = T(1) / T(T)`.

Ключевое наблюдение: **ускорение маленькое (S ≈ 1)**, потому что:

- **Map (M2)**, **поиск `min_nonzero`**, **`merge_halves`** и **копия `M2 → M2_copy`** последовательны, занимают много времени;
- **pthread-overhead** - создание и ожидание потоков само стоит времени,  при малом `N` этих накладных расходов больше, чем полезной работы;
- на 2 ядрах ускорение насыщается при `T = 2`, при `T > 2` - деградация из-за oversubscription.

## Структура проекта

```
pthreads-benchmark/
├── README.md
├── Makefile
├── requirements.txt
├── .gitignore
├── src/
│   └── pthreads-bench.c        # исходник с pthreads + per-stage таймингами
└── scripts/
    ├── run.py                  # сетка (N, T) → results.csv
    └── plot.py                 # speedup.png, stages_fraction.png
```
