#!/bin/bash

set -euo pipefail

# Параметры
N1=300
N2=522000
K=8                    # до 8 потоков
POINTS=10              # 10 шагов по N

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="${SCRIPT_DIR}/ipp-parallel"
OUT="${SCRIPT_DIR}/results/results.csv"

if [ ! -x "$BIN" ]; then
    echo "Error: binary not found: $BIN (run 'make ipp-parallel' first)" >&2
    exit 1
fi

mkdir -p "$(dirname "$OUT")"

# Вычисление шага как целого числа
DELTA=$(( (N2 - N1) / POINTS ))

echo "N,Milliseconds,M" > "$OUT"

for M in $(seq 1 "$K"); do
    echo "=== Running experiments with M=$M threads ==="
    export OMP_NUM_THREADS=$M

    for N in $(seq "$N1" "$DELTA" "$N2"); do
        echo "Running N=$N, M=$M ..."
        ms=$("$BIN" "$N" | sed -n 's/.*Milliseconds passed: *\([0-9]\+\).*/\1/p')
        if [ -z "$ms" ]; then
            echo "Error: failed to parse time for N=$N, M=$M" >&2
            exit 1
        fi
        echo "$N,$ms,$M" >> "$OUT"
    done
done

echo "Experiments finished. Results saved in $OUT"
