#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <libgen.h>

#define A 360
#define ITERATIONS 5

#define CHECK(err) do { \
    if ((err) != CL_SUCCESS) { \
        fprintf(stderr, "OpenCL error %d at %s:%d\n", (err), __FILE__, __LINE__); \
        exit(1); \
    } \
} while (0)

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static char* load_kernel(const char* argv0) {
    char path[4096];
    snprintf(path, sizeof(path), "%s", argv0);
    char* dir = dirname(path);

    char full[4096];
    snprintf(full, sizeof(full), "%s/map_kernels.cl", dir);

    FILE* f = fopen(full, "r");
    if (!f) { perror(full); exit(1); }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char* src = (char*)malloc((size_t)size + 1);
    size_t n = fread(src, 1, (size_t)size, f);
    src[n] = '\0';
    fclose(f);
    return src;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s N\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    if (N <= 0 || N % 2 != 0) {
        printf("Error: N must be positive and even\n");
        return 1;
    }
    int N2 = N / 2;

    double* M1      = (double*)malloc(sizeof(double) * N);
    double* M1_out  = (double*)malloc(sizeof(double) * N);
    double* M2      = (double*)malloc(sizeof(double) * N2);
    double* M2_prev = (double*)malloc(sizeof(double) * N2);
    double* M2_out  = (double*)malloc(sizeof(double) * N2);

    if (!M1 || !M1_out || !M2 || !M2_prev || !M2_out) {
        fprintf(stderr, "Error: memory allocation failed\n");
        return 1;
    }

    // Generate
    for (int i = 0; i < N; i++) {
        unsigned int seed = (unsigned int)(i + 1);
        M1[i] = ((double)rand_r(&seed) / RAND_MAX) * (A - 1) + 1;
    }
    for (int i = 0; i < N2; i++) {
        unsigned int seed = (unsigned int)(i + 1001);
        M2[i]      = ((double)rand_r(&seed) / RAND_MAX) * (10 * A - A) + A;
        M2_prev[i] = M2[i];
    }

    // OpenCL setup
    cl_platform_id platform;
    cl_device_id   device;
    cl_int err;

    CHECK(clGetPlatformIDs(1, &platform, NULL));
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS)
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, NULL);
    CHECK(err);

    cl_context       context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    CHECK(err);
    cl_command_queue queue   = clCreateCommandQueue(context, device, 0, &err);
    CHECK(err);

    char* src = load_kernel(argv[0]);
    cl_program program = clCreateProgramWithSource(context, 1,
                              (const char**)&src, NULL, &err);
    CHECK(err);
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        char log[8192];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                              sizeof(log), log, NULL);
        fprintf(stderr, "%s\n", log);
        exit(1);
    }

    cl_kernel k_map_m1 = clCreateKernel(program, "map_m1", &err); CHECK(err);
    cl_kernel k_map_m2 = clCreateKernel(program, "map_m2", &err); CHECK(err);

    cl_mem buf_M1      = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                        sizeof(double) * N,  NULL, &err); CHECK(err);
    cl_mem buf_M1_out  = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                        sizeof(double) * N,  NULL, &err); CHECK(err);
    cl_mem buf_M2      = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                        sizeof(double) * N2, NULL, &err); CHECK(err);
    cl_mem buf_M2_prev = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                        sizeof(double) * N2, NULL, &err); CHECK(err);
    cl_mem buf_M2_out  = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                        sizeof(double) * N2, NULL, &err); CHECK(err);

    // Замер: upload
    double t_up0 = now();
    CHECK(clEnqueueWriteBuffer(queue, buf_M1, CL_TRUE, 0,
                               sizeof(double) * N, M1, 0, NULL, NULL));
    CHECK(clEnqueueWriteBuffer(queue, buf_M2, CL_TRUE, 0,
                               sizeof(double) * N2, M2, 0, NULL, NULL));
    CHECK(clEnqueueWriteBuffer(queue, buf_M2_prev, CL_TRUE, 0,
                               sizeof(double) * N2, M2_prev, 0, NULL, NULL));
    double t_upload = now() - t_up0;

    // Kernel Map M1
    CHECK(clSetKernelArg(k_map_m1, 0, sizeof(cl_mem), &buf_M1));
    CHECK(clSetKernelArg(k_map_m1, 1, sizeof(int),    &N));

    size_t global1 = (size_t)N;
    double t_k1_0 = now();
    CHECK(clEnqueueNDRangeKernel(queue, k_map_m1, 1, NULL, &global1, NULL, 0, NULL, NULL));
    CHECK(clFinish(queue));
    double t_kernel_m1 = now() - t_k1_0;

    // Kernel Map M2
    CHECK(clSetKernelArg(k_map_m2, 0, sizeof(cl_mem), &buf_M2));
    CHECK(clSetKernelArg(k_map_m2, 1, sizeof(cl_mem), &buf_M2_prev));
    CHECK(clSetKernelArg(k_map_m2, 2, sizeof(int),    &N2));

    size_t global2 = (size_t)N2;
    double t_k2_0 = now();
    CHECK(clEnqueueNDRangeKernel(queue, k_map_m2, 1, NULL, &global2, NULL, 0, NULL, NULL));
    CHECK(clFinish(queue));
    double t_kernel_m2 = now() - t_k2_0;

    // Замер: download
    double t_dn0 = now();
    CHECK(clEnqueueReadBuffer(queue, buf_M1, CL_TRUE, 0,
                              sizeof(double) * N, M1_out, 0, NULL, NULL));
    CHECK(clEnqueueReadBuffer(queue, buf_M2, CL_TRUE, 0,
                              sizeof(double) * N2, M2_out, 0, NULL, NULL));
    double t_download = now() - t_dn0;

    // Подсчет X по результатам GPU (проверка корректности и сравнение с CPU)
    double min_nonzero = 0.0;
    for (int j = 0; j < N2; j++) {
        if (M2_out[j] != 0.0) { min_nonzero = M2_out[j]; break; }
    }
    double X = 0.0;
    if (min_nonzero != 0.0) {
        for (int j = 0; j < N2; j++) {
            int q = (int)(M2_out[j] / min_nonzero);
            if (q % 2 == 0) X += sin(M2_out[j]);
        }
    }

    double t_total = t_upload + t_kernel_m1 + t_kernel_m2 + t_download;

    printf("N=%d\n", N);
    printf("T_upload=%.6f\n",   t_upload);
    printf("T_map_m1=%.6f\n",   t_kernel_m1);
    printf("T_map_m2=%.6f\n",   t_kernel_m2);
    printf("T_download=%.6f\n", t_download);
    printf("T_total=%.6f\n",    t_total);
    printf("X=%.6f\n", X);
    printf("N=%d. Milliseconds passed: %ld\n", N, (long)(t_total * 1000.0));

    // Cleanup
    clReleaseMemObject(buf_M1);
    clReleaseMemObject(buf_M1_out);
    clReleaseMemObject(buf_M2);
    clReleaseMemObject(buf_M2_prev);
    clReleaseMemObject(buf_M2_out);
    clReleaseKernel(k_map_m1);
    clReleaseKernel(k_map_m2);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    free(M1); free(M1_out);
    free(M2); free(M2_prev); free(M2_out);
    free(src);

    return 0;
}
