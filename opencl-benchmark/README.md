# OpenCL benchmark

Сравнение CPU- и GPU-версий C-программы через **OpenCL**: этапы `Map M1` и `Map M2` вынесены в GPU-ядра, замеряются **отдельно** `T_upload`, `T_map_m1`, `T_map_m2`, `T_download` и `T_total`. Плюс fallback GPU → CPU, если GPU нет.

Цель - показать, что GPU не дает автоматическое ускорение: для маленьких `N` и задач с последовательными этапами передача данных доминирует, и GPU проигрывает CPU.

## Задача

Программа делает **5 независимых экспериментов**. В каждом эксперименте над двумя массивами - `M1` (размер `N`) и `M2` (размер `N/2`) - выполняются этапы:

| Этап | Что происходит |
|---|---|
| **Generate** | Заполнение массивов случайными числами |
| **Map (M1)** | Каждый элемент `M1` = `coth(sqrt(x))`` |
| **Map (M2)** | Каждый элемент `M2` = `sqrt((M2_copy[j] + M2_copy[j-1]) * e)` |
| **Reduce** | Сумма синусов тех элементов `M2`, у которых частное `M2[j] / min_nonzero` - чётное, `min_nonzero`- первый ненулевой элемент после сортировки |

Итоговое значение `X` суммируется по всем 5 экспериментам.

### Что считается на GPU и почему

**На GPU:** `map_m1`, `map_m2` - независимые итерации.

**На CPU:**
- **Generate** -  `rand_r` в OpenCL C нет, плюс он занимает микросекунды;
- **Reduce** - поиск `min_nonzero` и сумма `sin` - последовательные и простые, выносить на GPU невыгодно при таких `N`.

## Требования

- GCC ≥ 9;
- OpenCL ICD loader (`libOpenCL.so`) — `sudo apt install ocl-icd-opencl-dev`;
- Драйвер OpenCL для GPU (Mesa / NVIDIA / AMD);
- Python 3 с пакетами `matplotlib`, `numpy`;
- Bash.

Установка Python-зависимостей:

```bash
pip install -r requirements.txt
```

## Запуск

```bash
make            # сборка opencl-bench
make bench      # сетка N, 10 повторов → results.csv
make plots      # графики → upload_vs_kernel.png, throughput.png
```

### Ручная сборка

```bash
gcc -O2 -Wall -Wextra -std=c11 src/opencl-bench.c -lm -lOpenCL -o opencl-bench
```

### Ручной запуск

```bash
./opencl-bench 522000
```

## Что ожидать на графиках

2 ядра / встроенный GPU, `N = 522000`:

| Этап | Время, с | Доля |
|---|---|---|
| T_upload | ~0.020 | ~40% |
| T_map_m1 | ~0.008 | ~16% |
| T_map_m2 | ~0.004 | ~8% |
| T_download | ~0.018 | ~36% |
| **T_total** | ~0.050 | 100% |

**Ключевое наблюдение:** **передача данных (`upload` + `download`) занимает ~75% времени**, а сами ядра (`T_map_m1` + `T_map_m2`) только ~25%, поэтому:

- для маленьких `N` GPU **медленнее** CPU: передача данных больше самой работы;
- для больших `N` GPU может обогнать CPU, но только если ядра действительно тяжёлые;
- `map_m2` на GPU считает **другую формулу** (без рекуррентности), поэтому `X` не совпадает с CPU-версиями.

## Структура проекта

```
opencl-benchmark/
├── README.md
├── Makefile
├── requirements.txt
├── .gitignore
├── src/
│   ├── opencl-bench.c         # хост-код OpenCL
│   └── map_kernels.cl         # два OpenCL-ядра
└── scripts/
    ├── run.py                 # сетка N × 10 повторов → results.csv
    └── plot.py                # upload_vs_kernel.png, throughput.png
```
