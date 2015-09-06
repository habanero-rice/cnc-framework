#include "EvenOddSums.h"

int cncMain(int argc, char *argv[]) {
    CNC_REQUIRE(argc == 2, "Requires one argument N.\n");

    // Create a new graph context
    EvenOddSumsCtx *context = EvenOddSums_create();

    // Set up arguments for new graph instantiation
    EvenOddSumsArgs *args = cncItemAlloc(sizeof(*args));
    args->n = atoi(argv[1]);

    // Launch the graph for execution
    EvenOddSums_launch(args, context);

    // Exit when the graph execution completes
    CNC_SHUTDOWN_ON_FINISH(context);

    return 0;
}
