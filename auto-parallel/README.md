# Auto-parallelization (GCC)

Сравнение последовательной и параллельной версий C-программы, где параллелизм включается компилятором GCC через `-ftree-parallelize-loops`. Плюс оценка ускорения от числа потоков `K`.

Цель - измерить, насколько автоматическая параллелизация GCC ускоряет программу с 5 этапами обработки массивов, и сравнить поведение с ручной параллелизацией.

## Что внутри

- **`seq`** - последовательная версия;
- **`par-K`** - параллельная версия, собранная с `-ftree-parallelize-loops=K` (число потоков K = 1, 2, 4, 8);
- **GCC auto-parallelization** - механизм для параллельной версии - компилятор сам выбирает, какие циклы распараллелить.

### Распараллеливание GCC

GCC сам решает, какие циклы безопасно распараллелить. На практике:
- **Map (M1)**, **Map (M2)**, **Merge** и частично **Reduce** - распараллеливаются, так как имеют независимые итерации;
- этапы **Generate** (общий `seed` у `rand_r`) и **Sort** (обмены зависят друг от друга) остаются последовательными - это ограничивает итоговое ускорение.

## Требования

- GCC ≥ 9 с поддержкой auto-parallelization (флаги `-floop-parallelize-all`, `-ftree-parallelize-loops=K`);
- OpenMP runtime (`libgomp`);
- Python 3 с пакетами `pandas`, `matplotlib`;
- Bash.

Установка Python-зависимостей:

```bash
pip install -r requirements.txt
```

## Запуск

```bash
make                      # сборка seq (без параллелизации) и par-{1,2,4,8} (auto-parallelization)
make test                 # прогон всех N и K и фиксация времени → sample_results.csv
python3 plot_results.py   # графики зависимости → time_execution.png (время от `N`), speedup.png (ускорение от `N`)
```

### Ручная сборка

```bash
gcc -O3 -Wall -Werror -lm -o seq auto-parallel.c

for K in 1 2 4 8; do
    gcc -O3 -Wall -Werror -lm \
        -floop-parallelize-all -ftree-parallelize-loops=$K \
        auto-parallel.c -o par-$K
done
```

## Что ожидать на графиках

4 ядра, Linux; примерные значения для `N = 550000`:

| Потоков `K` | Время, ms | Ускорение |
|---|---|---|
| seq | ~234 | 1.00 |
| 1 | ~233 | ~1.00 |
| 2 | ~238 | ~0.98 |
| 4 | ~224 | ~1.04 |
| 8 | ~232 | ~1.01 |

**Ускорение** - во сколько раз параллеьная версия `par-K` быстрее последовательной `seq`: `S_K(N) = T_seq(N) / T_par_K(N)`.

Ключевое наблюдение: **auto-parallelization GCC даёт малое ускорение** на этой задаче. Причины:

- значительная часть работы последовательная (Generate, Sort, поиск `min_nonzero`);
- GCC не всегда распараллеливает цикл с рекуррентной зависимостью - на практике Map (M2) часто остаётся последовательным;
- накладные расходы на создание потоков заметны на малых `N`.

## Структура проекта

```
auto-parallel/
├── README.md
├── Makefile
├── requirements.txt
├── auto-parallel.c       # исходник (общий для seq и par-K)
├── run_all.sh            # замер времени seq vs par-K → sample_results.csv
└── plot_results.py       # графики времени и ускорения
```
