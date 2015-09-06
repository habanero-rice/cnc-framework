#include "Cholesky.h"

/*
 */
void Cholesky_kjiComputeStep(cncTag_t k, cncTag_t j, CholeskyCtx *ctx) {
    int i;
    for(i = k+1; i < j+1; i++){
        cncPrescribe_updateStep(k, j, i, ctx);
    }
}
