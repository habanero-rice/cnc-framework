#ifndef _CNCOCR_SMITHWATERMAN_TYPES_H_
#define _CNCOCR_SMITHWATERMAN_TYPES_H_

#include <sys/time.h>
#include <math.h>

#define GAP_PENALTY -1
#define TRANSITION_PENALTY -2
#define TRANSVERSION_PENALTY -4
#define MATCH 2

enum Nucleotide {GAP=0, ADENINE, CYTOSINE, GUANINE, THYMINE};

#define SEQ1(seqDataPtr) ((seqDataPtr)->strings)
#define SEQ2(seqDataPtr) ((seqDataPtr)->strings+(seqDataPtr)->seq2offset)

#define TSEQ1(seqDataPtr) ((char(*)[(seqDataPtr)->tw])SEQ1(seqDataPtr))
#define TSEQ2(seqDataPtr) ((char(*)[(seqDataPtr)->th])SEQ2(seqDataPtr))

typedef struct {
    s32 tw, th, seq2offset;
    signed char score_matrix[5][5];
    signed char strings[];
} SeqData;

typedef SeqData SmithWatermanArgs;

#ifdef CNC_DISTRIBUTED
static inline cncLocation_t swDist(int i, int j, int rows, int cols, int ranks) {
    const double width = (cols+rows) / ranks;
    const int offset = width;
    const int rawRank = ((int)floor((i-j+offset) / width)) % ranks;
    return (rawRank + ranks) % ranks;
}
#endif /* CNC_DISTRIBUTED */

#endif /*_CNCOCR_SMITHWATERMAN_TYPES_H_*/
