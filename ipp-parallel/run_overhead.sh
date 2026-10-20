#!/bin/bash

set -euo pipefail

# Параметры
N2=463500
INTERVAL=1
MEASUREMENTS=20
M_values=(4 8 12 16 20)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BIN="${ROOT_DIR}/omp-parallel"
RESULTS_DIR="${ROOT_DIR}/results"

if [ ! -x "$BIN" ]; then
    echo "Error: binary not found: $BIN (run 'make omp-parallel' first)" >&2
    exit 1
fi
if ! command -v mpstat >/dev/null 2>&1; then
    echo "Error: mpstat not found (install sysstat)" >&2
    exit 1
fi

mkdir -p "$RESULTS_DIR"

for M in "${M_values[@]}"; do
    echo "=== Запуск omp-parallel с M=$M ... ==="
    export OMP_NUM_THREADS=$M

    txt="${RESULTS_DIR}/cpu_M${M}.txt"
    csv="${RESULTS_DIR}/cpu_M${M}.csv"
    rm -f "$txt" "$csv"

    mpstat -P ALL "$INTERVAL" "$MEASUREMENTS" > "$txt" &
    MPSTAT_PID=$!

    sleep 1

    "$BIN" "$N2"

    wait "$MPSTAT_PID"

    python3 "${SCRIPT_DIR}/parse.py" "$txt"
done

echo "Все эксперименты завершены! Результаты в ${RESULTS_DIR}/"
