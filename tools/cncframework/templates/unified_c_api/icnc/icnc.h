{% import "common_macros.inc.c" as util with context -%}
{{ util.auto_file_banner() }}

{% set defname = "_ICNC_H_" -%}
#ifndef {{defname}}
#define {{defname}}

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

// OCR-compatible int types
#include <stdint.h>
typedef uint8_t  u8;
typedef int8_t   s8;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;
typedef uint64_t u64;
typedef int64_t  s64;

typedef s64 cncTag_t;

// TBB allocator
extern void* scalable_malloc( size_t size );
extern void  scalable_free( void* ptr );

static inline void *cncLocalAlloc(size_t bytes) {
    return scalable_malloc(bytes);
}

static inline void cncLocalFree(void *data) {
    if (data) { scalable_free(data); }
}

void *cncItemAlloc(size_t bytes);
void cncItemFree(void *item);

/* warning for variadic macro support */
#if __GNUC__ < 3 && !defined(__clang__) && __STDC_VERSION__ < 199901L && !defined(NO_VARIADIC_MACROS)
#warning Your compiler might not support variadic macros, in which case the CNC_REQUIRE macro is not supported. You can disable this warning by setting NO_VARIADIC_MACROS to 0, or disable the macro definitions by setting it to 1.
#endif

#if !NO_VARIADIC_MACROS
#define CNC_REQUIRE(cond, ...) do { if (!(cond)) { printf(__VA_ARGS__); exit(1); } } while (0)
#endif

#define CNC_SHUTDOWN_ON_FINISH(x) /* no op */
#define MAYBE_UNUSED(x) (void)(x);

#ifdef DIST_CNC
typedef struct cncBoxedItemStruct {
    // size should be the last field in this struct
    // (otherwise the unbox function breaks)
    uint64_t size; // counting this header
    // data[]
} cncBoxedItem_t;

static inline void *_cncItemUnbox(cncBoxedItem_t *box) {
    return box ? (void*)(&box->size + 1) : NULL;
}

static inline cncBoxedItem_t *_cncItemRebox(void *item) {
    return item ? (cncBoxedItem_t*)(((char*)item)-sizeof(cncBoxedItem_t)) : NULL;
}
#else
typedef void cncBoxedItem_t;

static inline void *_cncItemUnbox(cncBoxedItem_t *box) {
    return box;
}

static inline cncBoxedItem_t *_cncItemRebox(void *item) {
    return item;
}
#endif /* DIST_CNC */

#endif /*{{defname}}*/
