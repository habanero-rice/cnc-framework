#include "Stencil1D.h"

int cncMain(int argc, char *argv[]) {

    CNC_REQUIRE(argc == 4, "Usage: %s NUM_TILES TILE_SIZE NUM_TIMESTEPS\n", argv[0]);

    // Create a new graph context
    Stencil1DCtx *context = Stencil1D_create();

    // initialize graph context parameters
    context->numTiles = atoi(argv[1]);
    context->tileSize = atoi(argv[2]);
    context->lastTimestep = atoi(argv[3]);

    // Launch the graph for execution
    Stencil1D_launch(NULL, context);

    // Exit when the graph execution completes
    CNC_SHUTDOWN_ON_FINISH(context);

    return 0;
}
