#include "SimpleGraph.h"

/* Put an item to the Y collection */
void SimpleGraph_SY(cncTag_t y, SimpleGraphCtx *ctx) {
    int *Y = cncItemAlloc(sizeof(*Y));
    *Y = y;
    cncPut_Y(Y, ctx);
}
