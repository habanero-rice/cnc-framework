#include "Stencil1D.h"


void Stencil1D_cncInitialize(Stencil1DArgs *args, Stencil1DCtx *ctx) {

    s64 i, j;

    // Put "tile" items
    for (i = 0; i < ctx->numTiles; i++) {
        float *tile = cncItemAlloc(sizeof(*tile) * ctx->tileSize);
        for (j = 0; j < ctx->tileSize; j++) {
            tile[j] = 0;
        }
        cncPut_tile(tile, i, 0, ctx);
    }

    // Put "fromLeft" items
    for (i = 1; i < ctx->numTiles; i++) {
        float *fromLeft = cncItemAlloc(sizeof(*fromLeft));
        *fromLeft = 0;
        cncPut_fromLeft(fromLeft, i, 0, ctx);
    }

    // Put "fromRight" items
    for (i = 0; i < ctx->numTiles-1; i++) {
        float *fromRight = cncItemAlloc(sizeof(*fromRight));
        *fromRight = 0;
        cncPut_fromRight(fromRight, i, 0, ctx);
    }

    // Prescribe "stencil" steps
    for (i = 0; i < ctx->numTiles; i++) {
        cncPrescribe_stencil(i, 1, ctx);
    }

    // Set finalizer function's tag
    Stencil1D_await(ctx);

}


void Stencil1D_cncFinalize(float **tile, Stencil1DCtx *ctx) {

    int i, j;
    double total = 2; // first and last are always 1

    for (i = 0; i < ctx->numTiles; i++) {
        for (j = 0; j < ctx->tileSize; j++) {
            total += tile[i][j];
        }
    }

    double count = 2 + ctx->tileSize * ctx->numTiles;
    printf("avg = %f\n", total/count);

}


