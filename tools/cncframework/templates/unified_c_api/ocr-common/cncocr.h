{% import "common_macros.inc.c" as util with context -%}
{{ util.auto_file_banner() }}

{% set defname = "_CNCOCR_H_" -%}
#ifndef {{defname}}
#define {{defname}}

#ifdef CNC_DEBUG
#define OCR_ASSERT
#endif

#include <ocr.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__i386__) || defined(__x86_64__)
#    define CNCOCR_x86 1
#elif defined(TG_ARCH)
#    ifdef CNC_DEBUG_LOG
#        error "CnC debug logging is not supported on FSim (use trace instead)."
#    endif /* CNC_DEBUG_LOG */
#    define CNCOCR_TG 1
#else
#    warning UNKNOWN PLATFORM (possibly unsupported)
#endif

// XXX - OCR assert bug workaround
#ifdef CNCOCR_x86
#undef ASSERT
#define ASSERT(x) assert(x)
#include <assert.h>
#endif

#ifdef CNCOCR_TG
// use TG printf and assert functions
#define printf PRINTF
#define assert ASSERT
#endif

/********************************\
******** CNC TYPE ALIASES ********
\********************************/

typedef s64 cncTag_t; // tag components
{% block arch_typedefs -%}
{% endblock arch_typedefs %}
/*************************************\
******** CNC HELPER FUNCTIONS ********
\*************************************/

// Users should not call this function directly,
// but instead use the CNC_SHUTDOWN_ON_FINISH macro.
void cncAutomaticShutdown(ocrGuid_t doneEvent);

/*********************************\
******** CNC HELPER MACROS ********
\*********************************/

/* warning for variadic macro support */
#if __GNUC__ < 3 && !defined(__clang__) && __STDC_VERSION__ < 199901L && !defined(NO_VARIADIC_MACROS)
#warning Your compiler might not support variadic macros, in which case the CNC_REQUIRE macro is not supported. You can disable this warning by setting NO_VARIADIC_MACROS to 0, or disable the macro definitions by setting it to 1.
#endif

// FIXME - Should be able to handle this better after Bug#545 is addressed
#if CNCOCR_x86
#define CNC_ABORT(err) do { ocrShutdown(); exit(err); } while (0)
#else
#define CNC_ABORT(err) ocrAbort(err)
#endif

#if !NO_VARIADIC_MACROS
#define CNC_REQUIRE(cond, ...) do { if (!(cond)) { printf(__VA_ARGS__); CNC_ABORT(1); } } while (0)
#endif

/* squelch "unused variable" warnings */
#define MAYBE_UNUSED(x) ((void)x)

/* helpers for accessing packed argc/argv in ocrMain */
#define OCR_MAIN_ARGC OCR_ARGC(depv[0])
#define OCR_ARGC(dep) getArgc(dep.ptr)
#define OCR_MAIN_ARGV(i) OCR_ARGV(depv[0], i)
#define OCR_ARGV(dep, i) getArgv(dep.ptr, i)

#define CNC_SHUTDOWN_ON_FINISH(ctx) cncAutomaticShutdown((ctx)->_guids.doneEvent)
#define CNC_SHUTDOWN_ON_FINALIZE(ctx) cncAutomaticShutdown((ctx)->_guids.finalizedEvent)

/************************************************\
********* CNC ITEM MANAGEMENT FUNCTIONS *********
\************************************************/
// Cookie value for sanity-checking CnC items
// (before trying to extract the GUID)
static const u32 _CNC_ITEM_COOKIE = 0xC17C0C12;

// How much space do we need to store meta-data for CnC items?
typedef struct {
    ocrGuid_t guid;
    u64 size;
    u32 releasedFlag;
    u32 cookie;
    u64 data[];
} CnCItemMeta;

static inline CnCItemMeta *_cncItemMeta(void *item) {
    return (CnCItemMeta*) &((u8*)item)[-sizeof(CnCItemMeta)];
}

static inline void _cncItemCheckCookie(void *item) {
    #ifdef CNC_DEBUG
    if (item) {
        ASSERT(_cncItemMeta(item)->cookie == _CNC_ITEM_COOKIE && "Not a valid CnC item");
    }
    #endif
}

static inline bool _cncItemGetReleasedFlag(void *item) {
    return !item || _cncItemMeta(item)->releasedFlag;
}

static inline void _cncItemToggleReleasedFlag(void *item) {
    if (item) {
        _cncItemMeta(item)->releasedFlag ^= 1;
    }
}

static inline ocrGuid_t _cncItemGuid(void *item) {
    if (!item) return NULL_GUID;
    _cncItemCheckCookie(item);
    return _cncItemMeta(item)->guid;
}

static inline void *_cncItemDataPtr(void *item) {
    if (!item) return NULL;
    CnCItemMeta *meta = item;
    return &meta->data;
}

void *cncItemAlloc(size_t bytes);
void cncItemFree(void *item);

typedef s32 cncLocation_t;
#define CNC_CURRENT_LOCATION (-1)

#ifdef CNC_DEBUG_LOG
/**********************************\
********* CNC DEBUG LOGGING ********
\**********************************/
#ifdef CNC_DISTRIBUTED
#error "CnC debug logging not supported for distributed (try CNC_DEBUG_TRACE instead)"
#endif
extern FILE *cncDebugLog;
#endif /* CNC_DEBUG_LOG */

#include "cncocr_platform.h"

#endif /*{{defname}}*/
