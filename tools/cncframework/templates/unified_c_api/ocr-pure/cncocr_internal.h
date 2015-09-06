{% extends "ocr-common/cncocr_internal.h" %}

{% block tag_util -%}
#ifdef CNCOCR_TG
static inline bool memcmp(const void *a, const void *b, size_t n) {
    const u8 *x = (const u8*)a;
    const u8 *y = (const u8*)b;
    size_t i;
    for (i=0; i<n; i++) {
        if (x[i] != y[i]) return 1;
    }
    return 0;
}
#endif

{{ super() }}
{% endblock tag_util -%}

{% block singleton_ops -%}
/* Put a singleton item */
static inline void  _cncPutSingleton(ocrGuid_t item, cncItemCollHandle_t handle) {
    ocrEventSatisfy(handle.coll, item);
}

/* Get GUID for singleton item */
static inline void _cncGetSingleton(ocrGuid_t destination, u32 slot, ocrDbAccessMode_t mode, cncItemCollHandle_t handle) {
    ocrAddDependence(handle.coll, destination, slot, mode);
}
{% endblock singleton_ops -%}
