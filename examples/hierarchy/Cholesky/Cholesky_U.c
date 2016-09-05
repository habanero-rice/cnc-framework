#include "Cholesky.h"

/*
 * typeof data is double *
 */
void Cholesky_U(cncTag_t k, cncTag_t j, cncTag_t i, double *dataA1D, double *dataB1D, double *dataC1D, CholeskyCtx *ctx) {
    int t = ctx->tileSize;
    double (*dataA)[t] = (double(*)[t])dataA1D;
    double (*dataB)[t] = (double(*)[t])dataB1D;
    double (*dataC)[t] = (double(*)[t])dataC1D;

    double temp;
    int jB, kB, iB;
    for (jB = 0; jB < t ; jB++ ) {
        for (kB = 0; kB < t ; kB++) {
            temp = 0 - dataC[ jB ][ kB ];
            if (i != j)
                for (iB = 0; iB < t ; iB++)
                    dataA[ iB ][ jB ] += temp * dataB[ iB ][ kB ];
            else
                for (iB = jB; iB < t; iB++)
                    dataA[ iB ][ jB ] += temp * dataC[ iB ][ kB ];
        }
    }

    cncPut_MU(dataA1D, k+1, j, i, ctx);
}
