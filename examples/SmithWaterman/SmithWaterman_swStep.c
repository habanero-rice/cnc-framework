#include "SmithWaterman.h"
#include <stdlib.h>

static inline int max_score(int x, int y) {
    return (x > y) ? x : y;
}

/**
 * Step function defintion for "swStep"
 */
void SmithWaterman_swStep(cncTag_t i, cncTag_t j, SeqData *data, int *above, int *left, SmithWatermanCtx *ctx) {
    int ii, jj;
    //assert(!above || !left || above[0] == left[0] && "Diagonal values should match");

    /* Allocate a haloed local matrix for calculating 'this' tile*/
    /* 2D-ify it for readability */
    size_t entryCount = (1+ctx->tw)*(1+ctx->th);
    int (*curr_tile)[ctx->tw+1] = cncLocalAlloc(sizeof(int)*entryCount);

    /* Set local_tile[i+1][0] (left column) from the right column of the left tile */
    if (left) {
        for (ii = 0; ii <= ctx->th; ii++) {
            curr_tile[ii][0] = left[ii];
        }
    }
    else {
        for (ii = 0; ii <= ctx->th; ii++) {
            curr_tile[ii][0] = GAP_PENALTY * (i*ctx->th + ii);
        }
    }

    /* Set local_tile[0][j+1] (top row) from the bottom row of the above tile */
    if (above) {
        for (jj = 0; jj <= ctx->tw; ++jj) {
            curr_tile[0][jj] = above[jj];
        }
    }
    else {
        for (jj = 0; jj <= ctx->th; jj++) {
            curr_tile[0][jj] = GAP_PENALTY * (j*ctx->tw + jj);
        }
    }

    /* Run a smith-waterman on the local tile */
    for (ii = 1; ii <= ctx->th; ii++) {
        for (jj = 1; jj <= ctx->tw; jj++) {
            int char_from_1 = TSEQ1(data)[j][jj-1];
            int char_from_2 = TSEQ2(data)[i][ii-1];

            /* Get score from northwest, north and west */
            int diag_score = curr_tile[ii-1][jj-1] + data->score_matrix[char_from_2][char_from_1];
            int left_score = curr_tile[ii  ][jj-1] + data->score_matrix[char_from_1][GAP];
            int  top_score = curr_tile[ii-1][jj  ] + data->score_matrix[GAP][char_from_2];

            /* Set the local tile[i][j] to the maximum value of northwest, north and west */
            curr_tile[ii][jj] = max_score(left_score, max_score(top_score, diag_score));
        }
    }

    int *right = cncItemAlloc(sizeof(*right) * (ctx->th+1));
    for (ii = 0; ii <= ctx->th; ii++) {
        right[ii] = curr_tile[ii][ctx->tw];
    }
    cncPut_left(right, i, j+1, ctx);

    int *below = cncItemAlloc(sizeof(*below) * (ctx->tw+1));
    for (jj = 0; jj <= ctx->tw; jj++) {
        below[jj] = curr_tile[ctx->th][jj];
    }
    cncPut_above(below, i+1, j, ctx);

    /* Prescribe step for same column, next row */
    if (i+1 < ctx->nth) {
        cncPrescribe_swStep(i+1, j, ctx);
    }

    /* Cleanup */
    cncLocalFree(curr_tile);
    cncItemFree(above);
    cncItemFree(left);
}
