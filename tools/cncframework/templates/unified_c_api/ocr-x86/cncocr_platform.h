{% import "common_macros.inc.c" as util with context -%}
{{ util.auto_file_banner() }}

#include "ocr.h"

#ifdef CNC_DISTRIBUTED
// enable affinities API on x86-mpi
#define CNC_AFFINITIES 1
#endif /* CNC_DISTRIBUTED */

typedef struct _cncItemCollEntry **cncItemCollection_t; // item collections

typedef struct {
    cncItemCollection_t coll;
#ifdef CNC_AFFINITIES
    ocrGuid_t remoteCtxGuid;
    ptrdiff_t collOffset;
#endif /* CNC_AFFINITIES */
} cncItemCollHandle_t;

static inline void *cncLocalAlloc(size_t bytes) { return malloc(bytes); }
static inline void cncLocalFree(void *data) { free(data); }

