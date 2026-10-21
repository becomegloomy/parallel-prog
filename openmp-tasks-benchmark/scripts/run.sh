#!/bin/bash
set -euo pipefail

EXE="${1:-./omp-tasks-bench}"
OUTFILE="results_2.csv"
Ns=(10000 50000 100000 200000 400000 522000)
ThreadsList=(1 2 4 8 16)
Iterations=10

if [ ! -x "$EXE" ]; then
    echo "Error: $EXE not found (run 'make' first)" >&2
    exit 1
fi

echo -n "N,Threads" > "$OUTFILE"
for i in $(seq 1 "$Iterations"); do
    echo -n ",Time$i" >> "$OUTFILE"
done
echo "" >> "$OUTFILE"

for N in "${Ns[@]}"; do
    for T in "${ThreadsList[@]}"; do
        export OMP_NUM_THREADS=$T
        TIMES=()
        for i in $(seq 1 "$Iterations"); do
            MS=$("$EXE" "$N" "$T" | sed -n 's/.*Milliseconds passed: *\([0-9]\+\).*/\1/p')
            if [ -z "$MS" ]; then
                echo "Error: cannot parse time for N=$N, T=$T" >&2
                exit 1
            fi
            TIMES+=("$MS")
        done
        echo -n "$N,$T" >> "$OUTFILE"
        for t in "${TIMES[@]}"; do
            echo -n ",$t" >> "$OUTFILE"
        done
        echo "" >> "$OUTFILE"
        echo "Done N=$N Threads=$T"
    done
done

echo "Experiments finished. Results saved in $OUTFILE"
