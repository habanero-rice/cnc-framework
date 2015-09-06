#include "SimpleGraph.h"

void SimpleGraph_cncInitialize(SimpleGraphArgs *args, SimpleGraphCtx *ctx) {

    // Prescribe "SX" steps
    cncPrescribe_SX(1, ctx);

    // Prescribe "SY" steps
    cncPrescribe_SY(2, ctx);

    // Set finalizer function's tag
    SimpleGraph_await(ctx);

}

/*
 * typeof X is int
 * typeof Y is int
 */
void SimpleGraph_cncFinalize(int X, int Y, SimpleGraphCtx *ctx) {
    // Print results
    printf("X = %d\n", X);
    printf("Y = %d\n", Y);
}
