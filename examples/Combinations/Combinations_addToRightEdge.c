#include "Combinations.h"

/*
 */
void Combinations_addToRightEdge(cncTag_t row, cncTag_t col, CombinationsCtx *ctx) {

    // Put "out" items
    u64 *out = cncItemAlloc(sizeof(*out));
    *out = 1;
    cncPut_cells(out, row, col, ctx);

    if (row < ctx->n) {
        // Prescribe "addToInside" steps
        cncPrescribe_addToInside(row+1, col, ctx);

        // Prescribe "addToRightEdge" steps
        cncPrescribe_addToRightEdge(row+1, col+1, ctx);
    }

}
