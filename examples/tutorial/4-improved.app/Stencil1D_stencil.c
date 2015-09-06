#include "Stencil1D.h"

#define STENCIL(left, center, right) (0.5f*(center) + 0.25f*((left) + (right)))

/**
 * Step function defintion for "stencil"
 */
void Stencil1D_stencil(cncTag_t i, cncTag_t t, float *tile, float *fromLeft, float *fromRight, Stencil1DCtx *ctx) {

    // Put "newTile" items
    s32 j;
    const s32 lastJ = ctx->tileSize - 1;
    float *newTile = cncItemAlloc(sizeof(*newTile) * ctx->tileSize);

    // first (conditional, default=1)
    const float first = fromLeft ? *fromLeft : 1;
    newTile[0] = STENCIL(first, tile[0], tile[1]);
    // inner
    for (j=1; j<lastJ; j++) {
        newTile[j] = STENCIL(tile[j-1], tile[j], tile[j+1]);
    }
    // last (conditional, default=1)
    const float last = fromRight ? *fromRight : 1;
    newTile[lastJ] = STENCIL(tile[lastJ-1], tile[lastJ], last);

    cncPut_tile(newTile, i, t, ctx);

    if (fromRight) {
        // Put "toRight" items
        float *toRight = fromRight; // reuse
        *toRight = newTile[lastJ];
        cncPut_fromLeft(toRight, i+1, t, ctx);
    }

    if (fromLeft) {
        // Put "toLeft" items
        float *toLeft = fromLeft; // reuse
        *toLeft = newTile[0];
        cncPut_fromRight(toLeft, i-1, t, ctx);
    }

    if (t < ctx->lastTimestep) {
        // Prescribe "stencil" steps
        cncPrescribe_stencil(i, t+1, ctx);
    }

    // free old tile memory
    cncItemFree(tile);
}
