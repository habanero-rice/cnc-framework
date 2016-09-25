#ifndef _CNCOCR_CHOLESKY_TYPES_H_
#define _CNCOCR_CHOLESKY_TYPES_H_

#include "Cholesky_common.h"

typedef struct CholeskyArguments { } CholeskyArgs;

// Custom tuning functions for slices

#ifndef CHUNK_FOR_C
#define CHUNK_FOR_C 1
#endif
#ifndef CHUNK_FOR_T
#define CHUNK_FOR_T 1
#endif
#ifndef CHUNK_FOR_U
#define CHUNK_FOR_U 1
#endif

static inline unsigned distC_0(cncTag_t i) { return 0; }
static inline unsigned distC_i(cncTag_t i) { return i / CHUNK_FOR_C; }

static inline unsigned distT_0(cncTag_t i, cncTag_t r) { return 0; }
static inline unsigned distT_i(cncTag_t i, cncTag_t r) { return i / CHUNK_FOR_T; }
static inline unsigned distT_r(cncTag_t i, cncTag_t r) { return r / CHUNK_FOR_T; }

static inline unsigned distU_0(cncTag_t i, cncTag_t r, cncTag_t c) { return 0; }
static inline unsigned distU_i(cncTag_t i, cncTag_t r, cncTag_t c) { return i / CHUNK_FOR_U; }
static inline unsigned distU_r(cncTag_t i, cncTag_t r, cncTag_t c) { return r / CHUNK_FOR_U; }
static inline unsigned distU_c(cncTag_t i, cncTag_t r, cncTag_t c) { return c / CHUNK_FOR_U; }

#define MACRO_CONCAT_( symbol1, symbol2 ) DO_MACRO_CONCAT_( symbol1, symbol2 )
#define DO_MACRO_CONCAT_( symbol1, symbol2 ) symbol1##symbol2

#define distC MACRO_CONCAT_(distC_, TAG_FOR_C)
#define distT MACRO_CONCAT_(distT_, TAG_FOR_T)
#define distU MACRO_CONCAT_(distU_, TAG_FOR_U)

#ifndef TAG_FOR_C
#define TAG_FOR_C i
#endif
#ifndef TAG_FOR_T
#define TAG_FOR_T r
#endif
#ifndef TAG_FOR_U
#define TAG_FOR_U c
#endif

#endif /*_CNCOCR_CHOLESKY_TYPES_H_*/
