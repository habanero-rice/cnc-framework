#include "Cholesky.h"
#include <math.h>

/*
 * typeof data is double *
 */
void Cholesky_sequentialStep(cncTag_t k, double *data1D, CholeskyCtx *ctx) {

    int t = ctx->tileSize;
    double (*data)[t] = (double(*)[t])data1D;

    // Allocate new tile
    double *lBlock1D = cncItemAlloc(sizeof(*lBlock1D) * t*t);
    double (*lBlock)[t] = (double(*)[t])lBlock1D;

    // Calculate tile values
    int kB, jB, jBB, iB;
    for (kB = 0 ; kB < t ; kB++) {
        CNC_REQUIRE(data[ kB ][ kB ] > 0.0,
                    "[%ld][%d] Error: Not a symmetric positive definite (SPD) matrix\n", k, kB);
        lBlock[ kB ][ kB ] = sqrt( data[ kB ][ kB ] );

        for (jB = kB + 1; jB < t ; jB++)
            lBlock[ jB ][ kB ] = data[ jB ][ kB ] / lBlock[ kB ][ kB ];

        for (jBB= kB + 1; jBB < t ; jBB++)
            for (iB = jBB ; iB < t ; iB++)
                data[ iB ][ jBB ] = data[ iB ][ jBB ] - ( lBlock[ iB ][ kB ] * lBlock[ jBB ][ kB ] );
    }

    cncPut_data(lBlock1D, k, k, k+1, ctx);

    int tagResult = (k)*(k+1)/2 + k;
    cncPut_results(lBlock1D, tagResult, ctx);

}
