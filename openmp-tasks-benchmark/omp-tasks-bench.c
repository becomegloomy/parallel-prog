#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <math.h>
#include <sys/time.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#define A 360
#define ITERATIONS 5

static double coth(double x) {
    double ex = exp(x);
    double em = exp(-x);
    return (ex + em) / (ex - em);
}

// Comb Sort
static void comb_sort(double *arr, int n) {
    int gap = n, swapped = 1;
    while (gap > 1 || swapped) {
        if (gap > 1) gap = (int)(gap / 1.2473309);
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

// Слияние двух отсортированных половин
static void merge_halves(double *arr, int n, double *tmp) {
    int mid = n / 2;
    int i = 0, j = mid, k = 0;
    while (i < mid && j < n)
        tmp[k++] = (arr[i] < arr[j]) ? arr[i++] : arr[j++];
    while (i < mid) tmp[k++] = arr[i++];
    while (j < n)   tmp[k++] = arr[j++];
    for (i = 0; i < n; i++) arr[i] = tmp[i];
}

static long diff_ms(struct timeval a, struct timeval b) {
    return (b.tv_sec - a.tv_sec) * 1000L + (b.tv_usec - a.tv_usec) / 1000L;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s N [threads]\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    if (N <= 0) {
        printf("Error: N must be positive\n");
        return 1;
    }

#ifdef _OPENMP
    if (argc >= 3) omp_set_num_threads(atoi(argv[2]));
    printf("Max threads: %d\n", omp_get_max_threads());
#endif

    double X_final = 0.0;
    struct timeval T1, T2;
    gettimeofday(&T1, NULL);

    for (int i = 0; i < ITERATIONS; i++) {
        double *M1      = (double*)malloc(N       * sizeof(double));
        double *M2      = (double*)malloc((N / 2) * sizeof(double));
        double *M2_copy = (double*)malloc((N / 2) * sizeof(double));
        double *sort_tmp = (double*)malloc((N / 2) * sizeof(double));

        if (!M1 || !M2 || !M2_copy || !sort_tmp) {
            fprintf(stderr, "Error: memory allocation failed\n");
            return 1;
        }

        // Generate M1
#pragma omp parallel for default(none) shared(M1, N, i)
        for (int j = 0; j < N; j++) {
            unsigned int local_seed = (unsigned int)(i + 1) * (unsigned int)(j + 1);
            M1[j] = ((double)rand_r(&local_seed) / RAND_MAX) * (A - 1) + 1;
        }

        // Generate M2
#pragma omp parallel for default(none) shared(M2, M2_copy, N, i)
        for (int j = 0; j < N / 2; j++) {
            unsigned int local_seed = (unsigned int)(i + 1) * (unsigned int)(j + 1001);
            M2[j]      = ((double)rand_r(&local_seed) / RAND_MAX) * (10 * A - A) + A;
            M2_copy[j] = M2[j];
        }

        // Map M1
#pragma omp parallel for default(none) shared(M1, N)
        for (int j = 0; j < N; j++) {
            M1[j] = coth(sqrt(M1[j]));
        }

        // Map M2
        for (int j = 0; j < N / 2; j++) {
            double prev = (j == 0) ? 0.0 : M2_copy[j - 1];
            M2[j] = sqrt((M2_copy[j] + prev) * exp(1.0));
        }

        // Merge
#pragma omp parallel for default(none) shared(M1, M2, N)
        for (int j = 0; j < N / 2; j++) {
            M2[j] = M1[j] / M2[j];
        }

        // Sort
#ifdef _OPENMP
#pragma omp parallel default(none) shared(M2, N)
        {
#pragma omp single nowait
            {
#pragma omp task shared(M2)
                comb_sort(M2, N / 4);
#pragma omp task shared(M2)
                comb_sort(M2 + N / 4, N / 4);
#pragma omp taskwait
            }
        }
#else
        comb_sort(M2, N / 4);
        comb_sort(M2 + N / 4, N / 4);
#endif
        merge_halves(M2, N / 2, sort_tmp);

        // Reduce
        double min_nonzero = 0.0;
        for (int j = 0; j < N / 2; j++) {
            if (M2[j] != 0.0) { min_nonzero = M2[j]; break; }
        }

        double X = 0.0;
        if (min_nonzero != 0.0) {
#pragma omp parallel for default(none) shared(M2, N, min_nonzero) reduction(+:X)
            for (int j = 0; j < N / 2; j++) {
                int q = (int)(M2[j] / min_nonzero);
                if (q % 2 == 0) X += sin(M2[j]);
            }
        }

        printf("Iteration %d: X = %.6f\n", i, X);
        X_final += X;

        free(M1);
        free(M2);
        free(M2_copy);
        free(sort_tmp);
    }

    gettimeofday(&T2, NULL);

    printf("Final X after %d experiments: %.6f\n", ITERATIONS, X_final);
    printf("N=%d. Milliseconds passed: %ld\n", N, diff_ms(T1, T2));

    return 0;
}
