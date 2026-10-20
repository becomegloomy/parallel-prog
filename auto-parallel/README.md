# Auto-parallelization (GCC)

Сравнение последовательной и параллельной (GCC `-ftree-parallelize-loops`)
версий C-программы с 5 этапами:
- **Generate:** `rand_r`, массивы M1 (N) и M2 (N/2)
- **Map:** `coth(sqrt(x))` для M1, рекуррентная формула для M2
- **Merge:** `M1[i] / M2[i]`
- **Sort:** Comb Sort (собственная реализация)
- **Reduce:** сумма `sin` для элементов, дающих чётное при делении на min

## Сборка

```bash
make
```

**Или** вручную:

```bash
# Последовательная версия
gcc -O3 -Wall -Werror -lm -o lab1-seq auto-parallel.c

# Параллельные версии (K = 1, 2, 4, 8)
for K in 1 2 4 8; do
    gcc -O3 -Wall -Werror -lm \
        -floop-parallelize-all -ftree-parallelize-loops=$K \
        auto-parallel.c -o lab1-par-$K
done
```

## Запуск

```bash
./lab1-seq 10000
```

## Эксперименты

```bash
./run_all.sh > sample_results.csv
```

`run_all.sh` запускает `lab1-seq` и `lab1-par-K` для N от 200 до 550000 (11 точек),
собирает время в CSV.

## Анализ

```bash
pip install -r requirements.txt
python3 plot_results.py
```

Графики: `time_execution.png`, `speedup.png`.

## Результаты

| N      | seq (ms) | par-1 | par-2 | par-4 | par-8 |
|--------|----------|-------|-------|-------|-------|
| 200    | 0        | 0     | 0     | 0     | 0     |
| 55180  | 19       | 18    | 18    | 24    | 19    |
| 110160 | 39       | 38    | 39    | 39    | 38    |
| 165140 | 62       | 71    | 61    | 59    | 62    |
| 220120 | 83       | 81    | 80    | 86    | 92    |
| 275100 | 118      | 104   | 104   | 118   | 115   |
| 330080 | 130      | 141   | 138   | 143   | 134   |
| 385060 | 157      | 149   | 164   | 175   | 151   |
| 440040 | 174      | 200   | 204   | 219   | 187   |
| 495020 | 210      | 217   | 240   | 216   | 213   |
| 550000 | 234      | 233   | 238   | 224   | 232   |

> **Примечание:** данные с конкретной машины (4 ядра, Linux).
> Для воспроизведения запустите `./run_all.sh` локально.

## Стек и ключевые концепции
- **C** — `rand_r`, `gettimeofday`, `malloc`/`free`, `math.h`
- **GCC auto-parallelization** — `-floop-parallelize-all`,
  `-ftree-parallelize-loops=K` (K = 1, 2, 4, 8)
- **Comb Sort** — собственная реализация (без `qsort`)
- **Python** — pandas, matplotlib (анализ результатов)
- **Bash** — `run_all.sh` (автоматизация экспериментов)
