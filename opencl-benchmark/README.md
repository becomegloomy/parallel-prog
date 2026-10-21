# OpenCL benchmark

Сравнение CPU- и GPU-версий C-программы через **OpenCL**: этапы `Map M1` и `Map M2` вынесены в GPU-ядра, остальное на CPU. Время замеряется по частям: **сколько заняла передача данных на GPU** (`T_upload`), **сколько сами ядра** (`T_map_m1`, `T_map_m2`), **сколько передача результата обратно** (`T_download`), и **общее** (`T_total`). Если GPU в системе нет - код автоматически переключается на CPU-устройство OpenCL.

Цель - показать, что GPU не дает автоматическое ускорение: для маленьких `N` и задач с последовательными этапами передача данных доминирует, и GPU проигрывает CPU.

### Распараллеливание

**На GPU параллельно:** `map_m1`, `map_m2` - независимые итерации.

**На CPU последовательно:**
- **Generate** - `rand_r` в OpenCL C нет, плюс он занимает микросекунды;
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
make bench      # замер времени по 10 раз для каждого `N` → results.csv
make plots      # графики → upload_vs_kernel.png, throughput.png
```

- `upload_vs_kernel.png` - сколько процентов времени ушло на передачу данных, а сколько - на вычисления;
- `throughput.png` - зависимость времени каждого этапа от `N`.

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

**Передача данных (`upload` + `download`) занимает ~75% времени**, а сами ядра (`T_map_m1` + `T_map_m2`) только ~25%, поэтому:
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
    ├── run.py                 # results.csv
    └── plot.py                # upload_vs_kernel.png, throughput.png
```
