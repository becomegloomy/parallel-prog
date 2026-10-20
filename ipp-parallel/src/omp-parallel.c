#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <omp.h>

#define A 360
#define ITERATIONS 5

// Сортировка расчёской
static void comb_sort(double *arr, int n) {
    int gap = n;
    int swapped = 1;
    while (gap > 1 || swapped) {
        if (gap > 1)
            gap = (int)(gap / 1.2473309);
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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s N\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    if (N <= 0) {
        printf("Error: N must be positive\n");
        return 1;
    }

    struct timeval T1, T2;
    gettimeofday(&T1, NULL);

    double X_final = 0.0;

    for (int iter = 0; iter < ITERATIONS; iter++) {

        unsigned int seed = (unsigned int)iter;

        double *M1      = (double*) malloc(N       * sizeof(double));
        double *M2      = (double*) malloc((N / 2) * sizeof(double));
        double *M2_copy = (double*) malloc((N / 2) * sizeof(double));

        if (!M1 || !M2 || !M2_copy) {
            fprintf(stderr, "Error: memory allocation failed\n");
            return 1;
        }

        // Generate
        for (int j = 0; j < N; j++) {
            M1[j] = ((double)rand_r(&seed) / RAND_MAX) * (A - 1) + 1;
        }
        for (int j = 0; j < N / 2; j++) {
            M2[j]      = ((double)rand_r(&seed) / RAND_MAX) * (10 * A - A) + A;
            M2_copy[j] = M2[j];
        }

        // Map (M1): coth(sqrt(x))
        #pragma omp parallel for
        for (int j = 0; j < N; j++) {
            double x  = sqrt(M1[j]);
            double ep = exp(x);
            double en = exp(-x);
            M1[j] = (ep + en) / (ep - en);
        }

        // Map (M2)
        const double e = exp(1.0);
        #pragma omp parallel for
        for (int j = 0; j < N / 2; j++) {
            double prev = (j == 0) ? 0.0 : M2_copy[j - 1];
            M2[j] = sqrt((M2_copy[j] + prev) * e);
        }

        // Merge
        int min_size = (N < N / 2) ? N : N / 2;
        #pragma omp parallel for
        for (int j = 0; j < min_size; j++) {
            M2[j] = M1[j] / M2[j];
        }

        // Sort
        comb_sort(M2, N / 2);

        // Reduce
        double min_nonzero = 0.0;
        for (int j = 0; j < N / 2; j++) {
            if (M2[j] != 0.0) {
                min_nonzero = M2[j];
                break;
            }
        }

        double X = 0.0;
        if (min_nonzero != 0.0) {
            #pragma omp parallel for reduction(+:X)
            for (int j = 0; j < N / 2; j++) {
                int q = (int)(M2[j] / min_nonzero);
                if (q % 2 == 0) {
                    X += sin(M2[j]);
                }
            }
        }

        printf("Iteration %d: X = %.6f\n", iter, X);
        X_final += X;

        free(M1);
        free(M2);
        free(M2_copy);
    }

    printf("Final X after %d experiments: %.6f\n", ITERATIONS, X_final);

    gettimeofday(&T2, NULL);
    long delta_ms = (T2.tv_sec - T1.tv_sec) * 1000 +
                    (T2.tv_usec - T1.tv_usec) / 1000;

    printf("\nN=%d. Milliseconds passed: %ld\n", N, delta_ms);
    return 0;
}
