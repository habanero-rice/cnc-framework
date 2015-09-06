{% import "common_macros.inc.c" as util with context -%}
{{ util.auto_file_banner() }}

#include "ocr.h"

typedef ocrGuid_t cncItemCollection_t;

typedef struct {
    cncItemCollection_t coll;
} cncItemCollHandle_t;

// XXX - update this when OCR feature #630 is implemented
static inline void *cncLocalAlloc(size_t bytes) { return cncItemAlloc(bytes); }
static inline void cncLocalFree(void *data) { cncItemFree(data); }

