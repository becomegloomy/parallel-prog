#!/bin/bash

N1=200
N2=550000
DELTA=$(( (N2 - N1)/10 ))

K_values=(1 2 4 8)

echo "N,seq,par-1,par-2,par-4,par-8"

for i in $(seq 0 10); do
    N=$(( N1 + i*DELTA ))

    # Последовательная версия
    seq_ms=$(./lab1-seq $N | grep "Milliseconds passed" | awk '{print $4}')

    # Параллельные версии
    ms_list=()
    for K in "${K_values[@]}"; do
        export OMP_NUM_THREADS=$K
        ms=$(./lab1-par-$K $N | grep "Milliseconds passed" | awk '{print $4}')
        ms_list+=($ms)
    done

    echo "$N,$seq_ms,${ms_list[0]},${ms_list[1]},${ms_list[2]},${ms_list[3]}"
done
