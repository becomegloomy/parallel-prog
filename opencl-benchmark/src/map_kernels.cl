// Map M1
__kernel void map_m1(__global double* M1, int N) {
    int i = get_global_id(0);
    if (i < N) {
        double x  = sqrt(M1[i]);
        double ex = exp(x);
        double em = exp(-x);
        M1[i] = (ex + em) / (ex - em);
    }
}

// Map M2
__kernel void map_m2(__global double* M2,
                     __global const double* M2_prev,
                     int N) {
    int i = get_global_id(0);
    if (i < N) {
        double prev = (i == 0) ? 0.0 : M2_prev[i - 1];
        M2[i] = sqrt((M2_prev[i] + prev) * exp(1.0));
    }
}
