#include "Cholesky.h"
#include <math.h>

struct timeval startTime;

void Cholesky_cncInitialize(CholeskyArgs *args, CholeskyCtx *ctx) {
    const int N = ctx->numTiles;
    int t = ctx->tileSize;
    int n = N * t;

    // Read input matrix from file
    // Make sure the matrix side lengths are square numbers
    int root = sqrt(n);
    CNC_REQUIRE(root * root == n, "Generated matrices must have square dimensions\n");

    // Create tiles from source matrix
    int i = 0, r, c;
    for (r = 0; r < N; r++) {
        for (c = 0; c <= r; c++) {
            int A_i, A_j, T_i, T_j;
            double *temp1D = cncItemAlloc(sizeof(*temp1D) * t*t);
            // The 1D array of tile entries maps to a
            // 2D array corresponding to a t-by-t matrix tile
            double (*temp)[t] = (double(*)[t])temp1D;
            // Copy this tile's data from the input matrix
            for (A_i = r * t, T_i = 0 ; T_i < t ; A_i++, T_i++) {
                for (A_j = c * t, T_j = 0 ; T_j < t ; A_j++, T_j++) {
                    double *cell = &temp[T_i][T_j];
                    if (A_i == A_j) {
                        *cell = 4;
                    }
                    else if (A_i == A_j + root) {
                        *cell = -1;
                    }
                    else if (A_i == A_j + 1 && (A_j + 1) % n != 0) {
                        *cell = -1;
                    }
                    else {
                        *cell = 0;
                    }
                }
            }
            // Put the initialized tile
            cncPut_MU(temp1D, i, r, c, ctx);
        }
    }

    for (i=0; i<N; i++) {
        if (i != 0) {
            cncPrescribe_C(i, ctx);
        }
        for (r=i+1; r<N; r++) {
            cncPrescribe_T(i, r, ctx);
            for (c=i+1; c<=r; c++) {
                cncPrescribe_U(i, r, c, ctx);
            }
        }
    }

#if !CNCOCR_TG
    // Record starting time
    gettimeofday(&startTime, 0);
#endif

    // START!
    cncPrescribe_C(0, ctx);

    // Set up finalizer
    Cholesky_await(ctx);
}


/*
 * typeof results is double *
 */
void Cholesky_cncFinalize(double *results1D, CholeskyCtx *ctx) {
#if !CNCOCR_TG
    // Report the total running time
    struct timeval endTime;
    gettimeofday(&endTime, 0);
    double secondsRun = endTime.tv_sec - startTime.tv_sec;
    secondsRun += (endTime.tv_usec - startTime.tv_usec) / 1000000.0;
    printf("The computation took %f seconds\n", secondsRun);
#endif
    // Print the result matrix row-by-row (requires visiting each tile t times)
    const int t = ctx->tileSize;
    u64 checksum = 0;
    union { double d; u64 u; } tmpUnion;
    double (*results)[t] = (double(*)[t])results1D;
    int r, c;
    for (r = 0; r < t; r++) {
        for (c = 0; c <= r; c++) {
            tmpUnion.d = results[r][c];
            checksum += tmpUnion.u;
        }
    }
    printf("Result matrix checksum: %lx\n", checksum);
}

