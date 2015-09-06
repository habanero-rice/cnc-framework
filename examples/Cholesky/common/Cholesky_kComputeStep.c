#include "Cholesky.h"

/*
 */
void Cholesky_kComputeStep(CholeskyCtx *ctx) {
    int k;
    for(k = 0; k < ctx->numTiles; k++){
        cncPrescribe_kjComputeStep(k, ctx);
        cncPrescribe_sequentialStep(k, ctx);
    }
}
