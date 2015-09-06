#include "SmithWaterman.h"


void SmithWaterman_cncInitialize(SeqData *data, SmithWatermanCtx *ctx) {

    // Put sequence data
    cncPut_data(data, ctx);

    // Record starting time
    struct timeval *startTime = cncItemAlloc(sizeof(*startTime));
    gettimeofday(startTime, 0);
    cncPut_startTime(startTime, ctx);

    { // Prescribe "swStep" steps
        s64 _j;
        for (_j = 0; _j < ctx->ntw; _j++) {
            cncPrescribe_swStep(0, _j, ctx);
        }
    }

    // Set finalizer function's tag
    SmithWaterman_await(ctx);
}


void SmithWaterman_cncFinalize(struct timeval *startTime, int *above, SmithWatermanCtx *ctx) {
    struct timeval endTime;
    gettimeofday(&endTime, 0);
    double secondsRun = endTime.tv_sec - startTime->tv_sec;
    secondsRun += (endTime.tv_usec - startTime->tv_usec) / 1000000.0;
    printf("The computation took %f seconds\n", secondsRun);
    // Print the result
    printf("score: %d\n", above[ctx->tw]);
}

