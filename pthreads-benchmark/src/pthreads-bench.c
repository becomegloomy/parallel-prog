#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <math.h>
#include <pthread.h>
#include <time.h>

#define A 360
#define ITERATIONS 5

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

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
    while (i < mid && j < n) tmp[k++] = (arr[i] < arr[j]) ? arr[i++] : arr[j++];
    while (i < mid) tmp[k++] = arr[i++];
    while (j < n)   tmp[k++] = arr[j++];
    for (i = 0; i < n; i++) arr[i] = tmp[i];
}

// Задачи для pthreads

typedef struct {
    double       *arr;
    int           start, end;
    unsigned int  seed_offset;
} gen_arg_t;

static void* generate_array(void *arg) {
    gen_arg_t *ga = (gen_arg_t*)arg;
    for (int j = ga->start; j < ga->end; j++) {
        unsigned int seed = ga->seed_offset * (unsigned int)(j + 1);
        ga->arr[j] = ((double)rand_r(&seed) / RAND_MAX) * (A - 1) + 1;
    }
    return NULL;
}

typedef struct {
    double *arr;
    int     start, end;
} map_arg_t;

static void* map_coth(void *arg) {
    map_arg_t *ma = (map_arg_t*)arg;
    for (int j = ma->start; j < ma->end; j++)
        ma->arr[j] = coth(sqrt(ma->arr[j]));
    return NULL;
}

typedef struct {
    double *dst, *src1, *src2;
    int     start, end;
} merge_arg_t;

static void* map_merge(void *arg) {
    merge_arg_t *ma = (merge_arg_t*)arg;
    for (int j = ma->start; j < ma->end; j++)
        ma->dst[j] = ma->src1[j] / ma->src2[j];
    return NULL;
}

typedef struct {
    double *arr;
    int     start, end;
    double *min_nonzero;
    double *X;
    pthread_mutex_t *mtx;
} reduce_arg_t;

static void* reduce_x(void *arg) {
    reduce_arg_t *ra = (reduce_arg_t*)arg;
    double local_x = 0.0;
    double min_nz = *(ra->min_nonzero);
    if (min_nz == 0.0) return NULL;
    for (int j = ra->start; j < ra->end; j++) {
        int q = (int)(ra->arr[j] / min_nz);
        if (q % 2 == 0) local_x += sin(ra->arr[j]);
    }
    pthread_mutex_lock(ra->mtx);
    *(ra->X) += local_x;
    pthread_mutex_unlock(ra->mtx);
    return NULL;
}

typedef struct {
    double *arr;
    int     n;
} sort_arg_t;

static void* sort_half(void *arg) {
    sort_arg_t *sa = (sort_arg_t*)arg;
    comb_sort(sa->arr, sa->n);
    return NULL;
}

// разбиение на T частей
static void split(int n, int T, int *starts, int *ends) {
    int base = n / T, rem = n % T;
    int pos = 0;
    for (int t = 0; t < T; t++) {
        int len = base + (t < rem ? 1 : 0);
        starts[t] = pos;
        ends[t]   = pos + len;
        pos += len;
    }
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

    int T = 4;
    if (argc >= 3) {
        T = atoi(argv[2]);
    } else {
        const char *env = getenv("OMP_NUM_THREADS");
        if (env) T = atoi(env);
    }
    if (T < 1) T = 1;

    printf("Threads: %d\n", T);

    double X_final = 0.0;
    double t_start_total = now();

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
        double t0 = now();
        {
            pthread_t th[T];
            gen_arg_t args[T];
            int starts[T], ends[T];
            split(N, T, starts, ends);
            for (int t = 0; t < T; t++) {
                args[t].arr = M1;
                args[t].start = starts[t];
                args[t].end   = ends[t];
                args[t].seed_offset = (unsigned int)(i + 1);
                pthread_create(&th[t], NULL, generate_array, &args[t]);
            }
            for (int t = 0; t < T; t++) pthread_join(th[t], NULL);
        }
        double t_gen_m1 = now() - t0;

        // Generate M2
        t0 = now();
        {
            pthread_t th[T];
            int starts[T], ends[T];
            split(N / 2, T, starts, ends);
            gen_arg_t args[T];
            for (int t = 0; t < T; t++) {
                args[t].arr = M2;
                args[t].start = starts[t];
                args[t].end   = ends[t];
                args[t].seed_offset = (unsigned int)(i + 1) * 1000u;
                pthread_create(&th[t], NULL, generate_array, &args[t]);
            }
            for (int t = 0; t < T; t++) pthread_join(th[t], NULL);
            for (int j = 0; j < N / 2; j++) M2_copy[j] = M2[j];
        }
        double t_gen_m2 = now() - t0;

        // Map M1
        t0 = now();
        {
            pthread_t th[T];
            map_arg_t args[T];
            int starts[T], ends[T];
            split(N, T, starts, ends);
            for (int t = 0; t < T; t++) {
                args[t].arr = M1;
                args[t].start = starts[t];
                args[t].end   = ends[t];
                pthread_create(&th[t], NULL, map_coth, &args[t]);
            }
            for (int t = 0; t < T; t++) pthread_join(th[t], NULL);
        }
        double t_map_m1 = now() - t0;

        // Map M2
        t0 = now();
        for (int j = 0; j < N / 2; j++) {
            double prev = (j == 0) ? 0.0 : M2_copy[j - 1];
            M2[j] = sqrt((M2_copy[j] + prev) * exp(1.0));
        }
        double t_map_m2 = now() - t0;

        // Merge
        t0 = now();
        {
            pthread_t th[T];
            merge_arg_t args[T];
            int starts[T], ends[T];
            split(N / 2, T, starts, ends);
            for (int t = 0; t < T; t++) {
                args[t].dst = M2;
                args[t].src1 = M1;
                args[t].src2 = M2;
                args[t].start = starts[t];
                args[t].end   = ends[t];
                pthread_create(&th[t], NULL, map_merge, &args[t]);
            }
            for (int t = 0; t < T; t++) pthread_join(th[t], NULL);
        }
        double t_merge = now() - t0;

        // Sort
        t0 = now();
        {
            pthread_t th[2];
            sort_arg_t a0 = { M2, N / 4 };
            sort_arg_t a1 = { M2 + N / 4, N / 4 };
            pthread_create(&th[0], NULL, sort_half, &a0);
            pthread_create(&th[1], NULL, sort_half, &a1);
            pthread_join(th[0], NULL);
            pthread_join(th[1], NULL);
            merge_halves(M2, N / 2, sort_tmp);
        }
        double t_sort = now() - t0;

        // Reduce
        t0 = now();
        double min_nonzero = 0.0;
        for (int j = 0; j < N / 2; j++) {
            if (M2[j] != 0.0) { min_nonzero = M2[j]; break; }
        }

        double X = 0.0;
        if (min_nonzero != 0.0) {
            pthread_t th[T];
            reduce_arg_t args[T];
            int starts[T], ends[T];
            pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
            split(N / 2, T, starts, ends);
            for (int t = 0; t < T; t++) {
                args[t].arr = M2;
                args[t].start = starts[t];
                args[t].end   = ends[t];
                args[t].min_nonzero = &min_nonzero;
                args[t].X = &X;
                args[t].mtx = &mtx;
                pthread_create(&th[t], NULL, reduce_x, &args[t]);
            }
            for (int t = 0; t < T; t++) pthread_join(th[t], NULL);
            pthread_mutex_destroy(&mtx);
        }
        double t_reduce = now() - t0;

        printf("Iteration %d: X = %.6f\n", i, X);
        printf("Generate_M1: %.6f\n", t_gen_m1);
        printf("Generate_M2: %.6f\n", t_gen_m2);
        printf("Map_M1: %.6f\n", t_map_m1);
        printf("Map_M2: %.6f\n", t_map_m2);
        printf("Merge: %.6f\n", t_merge);
        printf("Sort: %.6f\n", t_sort);
        printf("Reduce: %.6f\n", t_reduce);

        X_final += X;

        free(M1);
        free(M2);
        free(M2_copy);
        free(sort_tmp);
    }

    double t_end_total = now() - t_start_total;

    printf("Final X after %d experiments: %.6f\n", ITERATIONS, X_final);
    printf("N=%d. Milliseconds passed: %ld\n", N, (long)(t_end_total * 1000.0));

    return 0;
}
