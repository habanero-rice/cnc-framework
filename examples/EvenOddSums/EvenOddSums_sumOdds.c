#include "EvenOddSums.h"

/**
 * Step function defintion for "sumOdds"
 */
void EvenOddSums_sumOdds(cncTag_t n, int *odds, EvenOddSumsCtx *ctx) {

    //
    // INPUTS
    //

    int sum = 0;
    { // Access "odds" inputs
        s64 _i;
        for (_i = 0; _i < n; _i++) {
            sum += odds[_i];
        }
    }


    //
    // OUTPUTS
    //

    // Put "oddsTotal" items
    int *oddsTotal = cncItemAlloc(sizeof(*oddsTotal));
    *oddsTotal = sum;
    cncPut_oddsTotal(oddsTotal, ctx);


}
