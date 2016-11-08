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
