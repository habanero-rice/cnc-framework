#include "Combinations.h"

/*
 */
void Combinations_addToLeftEdge(cncTag_t row, cncTag_t col, CombinationsCtx *ctx) {

    // Put "out" items
    u64 *out = cncItemAlloc(sizeof(*out));
    *out = 1;
    cncPut_cells(out, row, col, ctx);

    if (row < ctx->n) {
        // Prescribe "addToLeftEdge" steps
        cncPrescribe_addToLeftEdge(row+1, col, ctx);
    }


}
