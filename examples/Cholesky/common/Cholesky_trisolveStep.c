#include "Cholesky.h"

/*
 * typeof data is double *
 */
void Cholesky_trisolveStep(cncTag_t k, cncTag_t j, double *dataA1D, double *dataB1D, CholeskyCtx *ctx) {
    int t = ctx->tileSize;
    double (*dataA)[t] = (double(*)[t])dataA1D;
    double (*dataB)[t] = (double(*)[t])dataB1D;

    // Allocate new tile
    double *loBlock1D = cncItemAlloc(sizeof(*loBlock1D) * t*t);
    double (*loBlock)[t] = (double(*)[t])loBlock1D;

    int kB, iB, jB;
    for( kB = 0; kB < t ; kB++ ) {
        for( iB = 0; iB < t ; iB++ )
            loBlock[ iB ][ kB ] = dataA[ iB ][ kB ] / dataB[ kB ][ kB ];

        for( jB = kB + 1 ; jB < t ; jB++ )
            for( iB = 0; iB < t ; iB++ )
                dataA[ iB ][ jB ] -= dataB[ jB ][ kB ] * loBlock[ iB ][ kB ];
    }

    cncPut_data(loBlock1D, j, k, k+1, ctx);

    int tagResult = (j)*(j+1)/2 + k;
    cncPut_results(loBlock1D, tagResult, ctx);
}
