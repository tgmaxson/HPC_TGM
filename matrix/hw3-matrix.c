#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define N 512

void matrix_multiply(double a[N][N], double b[N][N], double result[N][N]) {
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            double sum = 0.0;
            #pragma omp simd
            for (int k = 0; k < N; k++) {
                sum += a[i][k] * b[k][j];
            }
            result[i][j] = sum;
        }
    }
}

int main() {
    srand(time(NULL));
    double (*a)[N] = malloc(sizeof(double[N][N]));
    double (*b)[N] = malloc(sizeof(double[N][N]));
    double (*result)[N] = malloc(sizeof(double[N][N]));

    // Initialize matrices a and b with some values
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            a[i][j] = (double)rand() / RAND_MAX;
            b[i][j] = (double)rand() / RAND_MAX;
            result[i][j] = 0.0;
        }
    }

    // Perform matrix multiplication
    matrix_multiply(a, b, result);

    /*
    // Print
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            printf("%f ", result[i][j]);
        }
        printf("\n");
    }
    */

    free(a);
    free(b);
    free(result);

    return 0;
}
