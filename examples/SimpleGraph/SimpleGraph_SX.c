#include "SimpleGraph.h"

/* Put an item to the X collection */
void SimpleGraph_SX(cncTag_t x, SimpleGraphCtx *ctx) {
    int *X = cncItemAlloc(sizeof(*X));
    *X = x;
    cncPut_X(X, ctx);
}
