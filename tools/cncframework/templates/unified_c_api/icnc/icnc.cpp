{% import "common_macros.inc.c" as util with context -%}
{{ util.auto_file_banner() }}

#include "icnc_internal.h"

cncAggregateTag_t _cncSingletonTag((cncTag_t)0, 1);

extern "C" {
    void *cncItemAlloc(size_t bytes) {
        cncBoxedItem_t *item;
        #ifdef DIST_CNC
        item = (cncBoxedItem_t*) cncLocalAlloc(sizeof(*item) + bytes);
        item->size = bytes;
        #else
        item = cncLocalAlloc(bytes);
        #endif /* DIST_CNC */
        return _cncItemUnbox(item);
    }

    void cncItemFree(void *item) {
        cncLocalFree(_cncItemRebox(item));
    }
}

#ifdef DIST_CNC
void CnC::serialize(CnC::serializer &ser, cncBoxedItem_t *&ptr) {
    const bool unpacking = ser.is_unpacking();
    uint64_t sz = 0;
    char *data = NULL;
    if (!unpacking) {
        sz = ptr->size;
        data = (char*)_cncItemUnbox(ptr);
    }
    ser & sz;
    if (unpacking) {
        data = (char*)cncItemAlloc(sz);
        ptr = _cncItemRebox((void*)data);
    }
    ser & CnC::chunk< char, CnC::no_alloc >( data, sz );
}
#endif /* DIST_CNC */

