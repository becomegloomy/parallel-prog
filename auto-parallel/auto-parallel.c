#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>

#define A 360

// Функция гиперболического котангенса
double coth(double x) {
    double ex = exp(x);
    double e_minus_x = exp(-x);
    return (ex + e_minus_x) / (ex - e_minus_x);
}

// Функция сортировки расчёской (Comb Sort)
void comb_sort(double *arr, int n) {
    int gap = n;
    int swapped = 1;
    while (gap > 1 || swapped) {
        if (gap > 1)
            gap = (int)(gap / 1.2473309); // Коэффициент уменьшения
        swapped = 0;
        for (int i = 0; i + gap < n; i++) {
            if (arr[i] > arr[i + gap]) {
                double tmp = arr[i];
                arr[i] = arr[i + gap];
                arr[i + gap] = tmp;
                swapped = 1;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s N\n", argv[0]);
        return 1;
    }

    int i, j;
    int N = atoi(argv[1]);
    struct timeval T1, T2;
    long delta_ms;

    gettimeofday(&T1, NULL); // Начало замера времени

    double X_final = 0.0;    // Для суммарного X после всех экспериментов

    for (i = 0; i < 5; i++) { // Уменьшиение числа экспериментов до 5
        unsigned int seed = i;
        double *M1 = (double*) malloc(N * sizeof(double));
        double *M2 = (double*) malloc((N/2) * sizeof(double));
        double *M2_copy = (double*) malloc((N/2) * sizeof(double));

        // Этап Generate
        for (j = 0; j < N; j++) {
            M1[j] = ((double)rand_r(&seed) / RAND_MAX) * (A - 1) + 1;
        }

        for (j = 0; j < N/2; j++) {
            M2[j] = ((double)rand_r(&seed) / RAND_MAX) * (10*A - A) + A;
            M2_copy[j] = M2[j];
        }

        // Этап Map
        for (j = 0; j < N; j++) {
            M1[j] = coth(sqrt(M1[j]));
        }

        for (j = 0; j < N/2; j++) {
            double prev = (j == 0) ? 0 : M2_copy[j-1];
            M2[j] = sqrt((M2_copy[j] + prev) * exp(1.0));
        }

        // Этап Merge
        int min_size = (N < N/2) ? N : N/2;
        for (j = 0; j < min_size; j++) {
            M2[j] = M1[j] / M2[j];
        }

        // Этап Sort
        comb_sort(M2, N/2);

        // Этап Reduce
        double min_nonzero = -1;
        for (j = 0; j < N/2; j++) {
            if (M2[j] != 0) {
                min_nonzero = M2[j];
                break;
            }
        }

        double X = 0.0;
        for (j = 0; j < N/2; j++) {
            int quotient = (int)(M2[j] / min_nonzero);
            if (quotient % 2 == 0) {
                X += sin(M2[j]);
            }
        }

        // Вывод значения X на каждой итерации
        printf("Iteration %d: X = %.6f\n", i, X);

        X_final += X; // Накопление для финального значения

        free(M1);
        free(M2);
        free(M2_copy);
    }

    // Вывод итогового X после 5 экспериментов
    printf("Final X after 5 experiments: %.6f\n", X_final);

    gettimeofday(&T2, NULL); // Конец замера времени
    delta_ms = (T2.tv_sec - T1.tv_sec) * 1000 + (T2.tv_usec - T1.tv_usec) / 1000;
    printf("\nN=%d. Milliseconds passed: %ld\n", N, delta_ms);

    return 0;
}
