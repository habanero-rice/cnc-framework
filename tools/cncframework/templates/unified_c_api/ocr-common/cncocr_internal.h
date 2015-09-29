{% import "common_macros.inc.c" as util with context -%}
{{ util.auto_file_banner() }}

{% set defname = "_CNCOCR_INTERNAL_H_" -%}
#ifndef {{defname}}
#define {{defname}}

#include <string.h>
#include "cncocr.h"

#ifdef CNC_AFFINITIES
#define ENABLE_EXTENSION_AFFINITY
#include <extensions/ocr-affinity.h>
#endif /* CNC_AFFINITIES */

#if CNCOCR_x86
#    ifndef HAL_X86_64
#    define hal_memCopy(dest, src, len, bg) memcpy(dest, src, len)
#    define hal_fence() __sync_synchronize()
#    endif
#elif CNCOCR_TG
// some HAL definitions (apps/lib/tg/include)
#    include "misc.h"
#else
#    warning UNKNOWN PLATFORM (possibly unsupported)
#endif

#if CNCOCR_TG
#define CNC_TABLE_SIZE 16
#else
#define CNC_TABLE_SIZE 1024
#endif

#define _CNC_ITEM_MODE DB_MODE_RW
#define _CNC_DBCREATE(guid, ptr, sz) _CNC_DBCREATE_PLACED(guid, ptr, sz, NULL_GUID)
#define _CNC_DBCREATE_PLACED(guid, ptr, sz, loc) ocrDbCreate(guid, ptr, sz, DB_PROP_SINGLE_ASSIGNMENT, loc, NO_ALLOC)

{% block tag_util -%}
/* Compare byte-by-byte for tag equality */
static inline bool _cncTagEquals(cncTag_t *tag1, cncTag_t *tag2, u32 length) {
    return memcmp(tag1, tag2, length*sizeof(*tag1)) == 0;
}

/* Hash function implementation. Fast and pretty good */
static inline u64 _cncTagHash(cncTag_t *components, u32 length) {
    u64 hash = 5381;
    u64 c;
    // All tags should be composed of longs, but even if
    // they're not, part of it is just ignored for the hash.
    u32 n = length;
    while (n-- > 0) {
        c = *components++;
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}
{% endblock tag_util -%}

cncItemCollection_t _cncItemCollectionCreate(void);
void _cncItemCollectionDestroy(cncItemCollection_t coll);

cncItemCollection_t _cncItemCollectionSingletonCreate(void);
void _cncItemCollectionSingletonDestroy(cncItemCollection_t coll);

#define _CNC_GETTER_ROLE 'G'
#define _CNC_PUTTER_ROLE 'P'

void _cncItemCollUpdate(cncItemCollHandle_t handle, cncTag_t *tag, u32 tagLength, u8 role,
        ocrGuid_t input, u32 slot, ocrDbAccessMode_t mode);

/* Putting an item into the hashmap */
static inline void _cncPut(ocrGuid_t item, cncTag_t *tag, int tagLength, cncItemCollHandle_t handle) {
    _cncItemCollUpdate(handle, tag, tagLength, _CNC_PUTTER_ROLE, item, 0, _CNC_ITEM_MODE);
}

/* Get GUID from an item tag */
static inline void _cncGet(cncTag_t *tag, int tagLength, ocrGuid_t destination, u32 slot,
        ocrDbAccessMode_t mode, cncItemCollHandle_t handle) {
    _cncItemCollUpdate(handle, tag, tagLength, _CNC_GETTER_ROLE, destination, slot, mode);
}

{% block singleton_ops -%}
/* Put a singleton item */
static inline void  _cncPutSingleton(ocrGuid_t item, cncItemCollHandle_t handle) {
    cncTag_t tag = 0;
    tag = -_cncTagHash(&tag, 1);
    _cncPut(item, &tag, 1, handle);
}

/* Get GUID for singleton item */
static inline void _cncGetSingleton(ocrGuid_t destination, u32 slot, ocrDbAccessMode_t mode,
        cncItemCollHandle_t handle) {
    cncTag_t tag = 0;
    tag = -_cncTagHash(&tag, 1);
    _cncGet(&tag, 1, destination, slot, mode, handle);
}
{% endblock singleton_ops -%}

static inline ocrGuid_t _cncCurrentAffinity() {
    #ifdef CNC_AFFINITIES
    ocrGuid_t affinity;
    ocrAffinityGetCurrent(&affinity);
    return affinity;
    #else
    return NULL_GUID;
    #endif
}

static inline ocrGuid_t _cncAffinityFromCtx(ocrGuid_t ctxGuid) {
    #ifdef CNC_AFFINITIES
    ocrGuid_t affinity;
    u64 count = 1;
    ocrAffinityQuery(ctxGuid, &count, &affinity);
    return affinity;
    #else
    return NULL_GUID;
    #endif /* CNC_AFFINITIES */
}

static inline ocrGuid_t _cncAffinityFromRank(cncLocation_t rank, ocrGuid_t affinities[]) {
    #ifdef CNC_AFFINITIES
    ASSERT(rank >= 0);
    return _cncAffinityFromCtx(affinities[rank]);
    #else
    return NULL_GUID;
    #endif /* CNC_AFFINITIES */
}

#ifdef CNC_AFFINITIES
#define _CNC_CTX_OFFSET(ctx, field) ((u8*)(&(ctx)->field) - (u8*)(ctx))
#define _CNC_ITEM_COLL_HANDLE(ctx, collName, loc) ((cncItemCollHandle_t){\
        (ctx)->_items.collName, (ctx)->_affinities[loc], _CNC_CTX_OFFSET(ctx, _items.collName)})
#else
#define _CNC_ITEM_COLL_HANDLE(ctx, collName, loc) ((cncItemCollHandle_t){(ctx)->_items.collName})
#endif /* CNC_AFFINITIES */

#endif /*{{defname}}*/
