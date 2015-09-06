#include "EvenOddSums.h"


void EvenOddSums_cncInitialize(EvenOddSumsArgs *args, EvenOddSumsCtx *ctx) {


    { // Put "naturals" items
        s64 _i;
        for (_i = 0; _i < args->n; _i++) {
            int *naturals = cncItemAlloc(sizeof(*naturals));
            *naturals = _i;
            cncPut_naturals(naturals, _i, ctx);
        }
    }

    // Prescribe "sumEvens" steps
    cncPrescribe_sumEvens(((args->n/2)) + (((args->n%2))), ctx);

    // Prescribe "sumOdds" steps
    cncPrescribe_sumOdds(args->n/2, ctx);

    // Set finalizer function's tag
    EvenOddSums_await(ctx);

}


void EvenOddSums_cncFinalize(int evensTotal, int oddsTotal, EvenOddSumsCtx *ctx) {

    printf("Even total = %d\n", evensTotal);

    printf("Odd total = %d\n", oddsTotal);

}


/* Mapping odds onto naturals */
naturalsItemKey oddsToNaturals(cncTag_t i, EvenOddSumsCtx *ctx) {
    naturalsItemKey _result;
    _result.i = i*2 + 1;
    return _result;
}


