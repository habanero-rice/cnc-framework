#include "Combinations.h"

int cncMain(int argc, char *argv[]) {

    CNC_REQUIRE(argc==3, "Compute n choose k\nUsage: %s n k\n", argv[0]);

    // Create a new graph context
    CombinationsCtx *context = Combinations_create();

    // initialize graph context parameters
    // u32 n, k;
    context->n = atoi(argv[1]);
    context->k = atoi(argv[2]);

    // Launch the graph for execution
    Combinations_launch(NULL, context);

    // Exit when the graph execution completes
    CNC_SHUTDOWN_ON_FINISH(context);

    return 0;
}
