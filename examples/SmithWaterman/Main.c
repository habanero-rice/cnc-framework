#include "SmithWaterman.h"
#include <string.h>

static char ALIGNMENT_SCORES[5][5] = {
    {GAP_PENALTY,GAP_PENALTY,GAP_PENALTY,GAP_PENALTY,GAP_PENALTY},
    {GAP_PENALTY,MATCH,TRANSVERSION_PENALTY,TRANSITION_PENALTY,TRANSVERSION_PENALTY},
    {GAP_PENALTY,TRANSVERSION_PENALTY, MATCH,TRANSVERSION_PENALTY,TRANSITION_PENALTY},
    {GAP_PENALTY,TRANSITION_PENALTY,TRANSVERSION_PENALTY, MATCH,TRANSVERSION_PENALTY},
    {GAP_PENALTY,TRANSVERSION_PENALTY,TRANSITION_PENALTY,TRANSVERSION_PENALTY, MATCH}
};

static inline signed char char_mapping(char c) {
    switch(c) {
        case '_': return GAP;
        case 'A': return ADENINE;
        case 'C': return CYTOSINE;
        case 'G': return GUANINE;
        case 'T': return THYMINE;
    }
    return -1;
}

static FILE *open_file(const char *fileName) {
    FILE *f = fopen(fileName, "r");
    CNC_REQUIRE(f, "Could not open file: %s\n", fileName);
    return f;
}

static size_t file_length(FILE *file) {
    fseek(file, 0L, SEEK_END);
    size_t len = (size_t)ftell(file);
    fseek(file, 0L, SEEK_SET);
    return len;
}

static size_t read_sequence(FILE *file, int fnum, signed char *dest, size_t fsize) {
    fread(dest, sizeof(char), fsize, file);
    size_t seqlen = 0, traverse_index = 0;
    while ( traverse_index < fsize ) {
        char curr_char = dest[traverse_index];
        switch ( curr_char ) {
        case 'A': case 'C': case 'G': case 'T':
            dest[seqlen++] = char_mapping(curr_char);
            break;
        }
        ++traverse_index;
    }
    dest[seqlen] = '\0';
    printf("Size of input sequence %d has length %lu\n", fnum, seqlen);
    return seqlen;
}

int cncMain(int argc, char *argv[]) {
    CNC_REQUIRE(argc == 5, "Usage: %s tileWidth tileHeight fileName1 fileName2\n", argv[0]);

    // Tile width and height
    int tw = atoi(argv[1]);
    int th = atoi(argv[2]);
    printf("Tile width:  %d\n", tw);
    printf("Tile height: %d\n", th);

    // Open sequence input files
    FILE *file1 = open_file(argv[3]);
    FILE *file2 = open_file(argv[4]);
    size_t filesize1 = file_length(file1);
    size_t filesize2 = file_length(file2);

    // Allocate tile data item and read sequence data
    size_t dataSize = sizeof(SeqData) + filesize1 + filesize2 + 2;
    SeqData *data = cncItemAlloc(dataSize);
    data->seq2offset = filesize1 + 1;
    size_t length1 = read_sequence(file1, 1, SEQ1(data), filesize1);
    size_t length2 = read_sequence(file2, 2, SEQ2(data), filesize2);
    CNC_REQUIRE(tw <= length1 && th <= length2, "Tile size too large for given input.\n");

    // Initialize tile scores and dimensions
    data->tw = tw;
    data->th = th;
    memcpy(data->score_matrix, ALIGNMENT_SCORES, sizeof(ALIGNMENT_SCORES));

    // Create a new graph context
    SmithWatermanCtx *context = SmithWaterman_create();
    context->ntw = length1 / tw;
    context->nth = length2 / th;
    context->tw = tw;
    context->th = th;

    // Done initializing data
    fclose(file1);
    fclose(file2);
    printf("Imported %d x %d tiles.\n", context->ntw, context->nth);
    // Exit when the graph execution completes
    CNC_SHUTDOWN_ON_FINISH(context);

    // Launch the graph for execution
    SmithWaterman_launch(data, context);

    return 0;
}
