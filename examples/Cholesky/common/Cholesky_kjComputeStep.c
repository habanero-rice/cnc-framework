#include "Cholesky.h"

/*
 */
void Cholesky_kjComputeStep(cncTag_t k, CholeskyCtx *ctx) {
    int j;
    for(j = k+1; j < ctx->numTiles; j++){
        cncPrescribe_kjiComputeStep(k, j, ctx);
        cncPrescribe_trisolveStep(k, j, ctx);
    }
}
