#include "Cholesky.h"
#include <string.h>

int cncMain(int argc, char *argv[]) {

    CNC_REQUIRE(argc==3,
            "Usage: ./Cholesky matrixSize tileSize (found %d args)\n", argc-1);

    // Create a new graph context
    CholeskyCtx *context = Cholesky_create();

    // Parse matrix dim info
    int matrixCols = atoi(argv[1]);
    int tileSize = atoi(argv[2]);
    int numTiles = matrixCols / tileSize;
    CNC_REQUIRE(matrixCols % tileSize == 0,
            "Incompatible tile size %d for the matrix of size %d\n", tileSize, matrixCols);

    // Set graph parameters
    context->numTiles = numTiles;
    context->tileSize = tileSize;

    // Launch the graph for execution
    Cholesky_launch(NULL, context);

    // Exit when the graph execution completes
    CNC_SHUTDOWN_ON_FINISH(context);

    return 0;
}
