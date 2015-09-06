#include "Stencil1D.h"


void Stencil1D_cncInitialize(Stencil1DArgs *args, Stencil1DCtx *ctx) {

    s64 i, j, t;

    // Put "tile" items
    for (i = 0; i < NUM_TILES; i++) {
        float *tile = cncItemAlloc(sizeof(*tile) * TILE_SIZE);
        for (j = 0; j < TILE_SIZE; j++) {
            tile[j] = 0;
        }
        cncPut_tile(tile, i, 0, ctx);
    }

    // Put "fromLeft" items
    for (i = 1; i < NUM_TILES; i++) {
        float *fromLeft = cncItemAlloc(sizeof(*fromLeft));
        *fromLeft = 0;
        cncPut_fromLeft(fromLeft, i, 0, ctx);
    }

    // Put "fromRight" items
    for (i = 0; i < NUM_TILES-1; i++) {
        float *fromRight = cncItemAlloc(sizeof(*fromRight));
        *fromRight = 0;
        cncPut_fromRight(fromRight, i, 0, ctx);
    }

    // Prescribe "stencil" steps
    for (i = 0; i < NUM_TILES; i++) {
        for (t = 1; t < LAST_TIMESTEP+1; t++) {
            cncPrescribe_stencil(i, t, ctx);
        }
    }

    // Set finalizer function's tag
    Stencil1D_await(ctx);

}


void Stencil1D_cncFinalize(float **tile, Stencil1DCtx *ctx) {

    int i, j;
    double total = 2; // first and last are always 1

    for (i = 0; i < NUM_TILES; i++) {
        for (j = 0; j < TILE_SIZE; j++) {
            total += tile[i][j];
        }
    }

    double count = 2 + TILE_SIZE * NUM_TILES;
    printf("avg = %f\n", total/count);

}


