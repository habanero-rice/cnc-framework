{% import "common_macros.inc.c" as util with context -%}
{{ util.auto_file_banner() }}

#include "cncocr_internal.h"

#define SIMPLE_DBCREATE(guid, ptr, sz) ocrDbCreate(guid, ptr, sz, DB_PROP_NONE, NULL_HINT, NO_ALLOC)

static ocrGuid_t _destroyEventEdt(u32 paramc, u64 paramv[], u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t eventGuid = *(ocrGuid_t*)paramv;
    ocrEventDestroy(eventGuid);
    ocrGuid_t dbGuid = depv[0].guid;
    if (!ocrGuidIsNull(dbGuid)) {
        ocrDbDestroy(dbGuid);
    }
    return NULL_GUID;
}

u8 ocrEventDestroyDeep(ocrGuid_t event) {
    ocrGuid_t template, edt;
    // Make sure it's already satisfied (idempotent)
    u8 res = ocrEventSatisfy(event, NULL_GUID);
    const u32 paramc = sizeof(ocrGuid_t) / sizeof(u64);
    res = res ||ocrEdtTemplateCreate(&template, _destroyEventEdt, paramc, 1);
    res = res || ocrEdtCreate(&edt, template,
            /*paramc=*/EDT_PARAM_DEF, /*paramv=*/(u64*)&event,
            /*depc=*/EDT_PARAM_DEF, /*depv=*/&event,
            /*properties=*/EDT_PROP_NONE,
            /*hint=*/NULL_HINT, /*outEvent=*/NULL);
    res = res || ocrEdtTemplateDestroy(template);
    return res;
}

{#/* Require sub-templates to override the internal item collection interface */-#}
{% block cnc_itemcoll_internal -%}
#error Missing cnc item collection implementation in {{ self }}
{% endblock cnc_itemcoll_internal %}

{#/* Allow sub-templates to override the internal item singleton collection interface */-#}
{% block cnc_itemcoll_singleton_internal -%}
cncItemSingleton_t _cncItemCollectionSingletonCreate(void) {
    cncItemSingleton_t coll;
    // FIXME - Create with new event affinity API in OCR v1.2.0
    ocrEventCreate(&coll.only, OCR_EVENT_IDEM_T, EVT_PROP_TAKES_ARG);
    return coll;
}

void _cncItemCollectionSingletonDestroy(cncItemSingleton_t coll) {
    ocrEventDestroyDeep(coll.only);
}
{% endblock cnc_itemcoll_singleton_internal %}

{#/* Allow sub-templates to override the internal dense-item collection interface */-#}
{% block cnc_itemcoll_dense_internal -%}
{% if g.hasTuning('dense_mapping') -%}
cncItemCollectionDense_t _cncItemCollectionDenseCreate(u64 count) {
    cncItemCollectionDense_t coll;
    printf("LABELED GUID COUNT = %" PRIu64"\n", count); // XXX DEBUG
    ocrGuidRangeCreate(&coll.base, count, GUID_USER_EVENT_IDEM);
    return coll;
}

void _cncItemCollectionDenseDestroy(cncItemCollectionDense_t coll) {
    // FIXME - should do a deep destroy of all contained datablocks
    // Update to OCR v1.2.0 GUID Range destroy function
    ocrGuidMapDestroy(coll.base);
}
{% endif -%}
{% endblock cnc_itemcoll_dense_internal %}
