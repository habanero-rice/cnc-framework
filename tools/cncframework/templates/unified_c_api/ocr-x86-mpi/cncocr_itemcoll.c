{#/* Inherit shared-memory item collection insertion functionality */-#}
{% extends "ocr-x86/cncocr_itemcoll.c" %}

{#/* Override the public item collection interface for distribution */-#}
{% block cnc_itemcoll_update -%}

struct _cncItemCollUpdateParams {
    ocrGuid_t input;
    ptrdiff_t collOffset;
    u8 role;
    u32 tagLength;
    u32 slot;
    ocrDbAccessMode_t mode;
    cncTag_t tag[];
};

static ocrGuid_t _cncItemCollUpdateEdt(u32 paramc, u64 paramv[], u32 depc, ocrEdtDep_t depv[]) {
    struct _cncItemCollUpdateParams *p = depv[0].ptr;
    u8 *ctxBase = depv[1].ptr;
    cncItemCollection_t *coll = (void*)(ctxBase + p->collOffset);
    ocrGuid_t result = _cncItemCollUpdateLocal(*coll, p->tag, p->tagLength, p->role, p->input, p->slot, p->mode);
    ocrDbDestroy(depv[0].guid);
    return result;
}

static void _cncItemCollUpdateRemote(cncItemCollHandle_t handle, cncTag_t *tag, u32 tagLength, u8 role,
        ocrGuid_t input, u32 slot, ocrDbAccessMode_t mode) {
    ocrGuid_t pGuid;
    struct _cncItemCollUpdateParams *p;
    const ocrGuid_t remoteCtx = handle.remoteCtxGuid;
    const ocrGuid_t affinity = _cncAffinityFromCtx(remoteCtx);
    if (ocrGuidIsEq(affinity, _cncCurrentAffinity())) {
        return; // the local update took care of it
    }
    { // init params struct
        const size_t tagBytes = tagLength*sizeof(*tag);
        _CNC_DBCREATE(&pGuid, (void**)&p, sizeof(*p) + tagBytes);
        p->input = input;
        p->collOffset = handle.collOffset;
        p->role = role;
        p->tagLength = tagLength;
        p->slot = slot;
        p->mode = mode;
        memcpy(p->tag, tag, tagBytes);
    }
    { // do remote op
        ocrGuid_t tmpl, edt;
        // FIXME - should just set up this template somewhere once...
        ocrEdtTemplateCreate(&tmpl, _cncItemCollUpdateEdt, 0, 2);
        ocrHint_t hint;
        ocrEdtCreate(&edt, tmpl,
                /*paramc=*/0, /*paramv=*/NULL,
                /*depc=*/EDT_PARAM_DEF, /*depv=*/NULL,
                /*properties=*/EDT_PROP_NONE,
                /*hint=*/_cncEdtAffinityHint(&hint, affinity),
                /*outEvent=*/NULL);
        ocrAddDependence(pGuid, edt, 0, DB_MODE_RO);
        ocrAddDependence(remoteCtx, edt, 1, DB_MODE_RO);
        ocrEdtTemplateDestroy(tmpl);
    }
}

void _cncItemCollUpdate(cncItemCollHandle_t handle, cncTag_t *tag, u32 tagLength, u8 role, ocrGuid_t input,
        u32 slot, ocrDbAccessMode_t mode) {
    // do local item collection update first
    ocrGuid_t placeholder = _cncItemCollUpdateLocal(handle.coll, tag, tagLength, role, input, slot, mode);
    // Remote lookup for gets only if local get failed
    if (role == _CNC_GETTER_ROLE) {
        if (!ocrGuidIsNull(placeholder)) {
            _cncItemCollUpdateRemote(handle, tag, tagLength, role, placeholder, 0, mode);
        }
    }
    // Always remote update for puts
    else {
        // FIXME - possible memory leak if the placeholder already exists and we do this put,
        // since it's going to create a copy later...
        _cncItemCollUpdateRemote(handle, tag, tagLength, role, input, slot, mode);
    }
}

{%- endblock cnc_itemcoll_update %}
