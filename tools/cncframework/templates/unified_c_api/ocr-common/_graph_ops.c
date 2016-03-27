{% import "common_macros.inc.c" as util with context -%}
{{ util.auto_file_banner() }}

#include "{{g.name}}_internal.h"
#include <string.h>

#ifdef CNC_DEBUG_LOG
#ifndef CNCOCR_x86
#error "Debug logging mode only supported on x86 targets"
#endif
#include <pthread.h>
pthread_mutex_t _cncDebugMutex = PTHREAD_MUTEX_INITIALIZER;
#endif /* CNC_DEBUG_LOG */

{{util.g_ctx_t()}} *{{g.name}}_create() {
#ifdef CNC_DEBUG_LOG
    // init debug logger (only once)
    if (!cncDebugLog) {
        cncDebugLog = fopen(CNC_DEBUG_LOG, "w");
    }
#endif /* CNC_DEBUG_LOG */
    // allocate the context datablock
    ocrGuid_t contextGuid;
    {{util.g_ctx_param()}};
    u64 ctxBytes = sizeof(*{{util.g_ctx_var()}});
    #ifdef CNC_AFFINITIES
    u64 affinityCount;
    ocrAffinityCount(AFFINITY_PD, &affinityCount);
    ctxBytes += sizeof(ocrGuid_t) * affinityCount;
    #endif /* CNC_AFFINITIES */
    // IMPORTANT! We do NOT use _CNC_DBCREATE here because we do NOT want DB_PROP_SINGLE_ASSIGNMENT
    ocrDbCreate(&contextGuid, (void**)&{{util.g_ctx_var()}}, ctxBytes, DB_PROP_NONE, NULL_HINT, NO_ALLOC);
    // store a copy of its guid inside
    {{util.g_ctx_var()}}->_guids.self = contextGuid;
    // initialize graph events
    // TODO - these events probably shouldn't be marked as carrying data
    ocrEventCreate(&{{util.g_ctx_var()}}->_guids.finalizedEvent, OCR_EVENT_STICKY_T, EVT_PROP_TAKES_ARG);
    ocrEventCreate(&{{util.g_ctx_var()}}->_guids.quiescedEvent, OCR_EVENT_STICKY_T, EVT_PROP_NONE);
    ocrEventCreate(&{{util.g_ctx_var()}}->_guids.doneEvent, OCR_EVENT_STICKY_T, EVT_PROP_TAKES_ARG);
    ocrEventCreate(&{{util.g_ctx_var()}}->_guids.awaitTag, OCR_EVENT_ONCE_T, EVT_PROP_TAKES_ARG);
    ocrEventCreate(&{{util.g_ctx_var()}}->_guids.contextReady, OCR_EVENT_LATCH_T, EVT_PROP_NONE);
    #ifdef CNC_AFFINITIES
    {{util.g_ctx_var()}}->_affinityCount = affinityCount;
    // XXX - should use this for item collection affinities
    // Currently, I have to query DB guids to get affinities safely
    // across PDs (see issue #640). However, this works well since I'm
    // making a local copy of the context on each node anyway.
    { // initialize affinity data
        const ocrGuid_t currentLoc = _cncCurrentAffinity();
        ocrAffinityGet(AFFINITY_PD, &affinityCount, {{util.g_ctx_var()}}->_affinities);
        assert(affinityCount == {{util.g_ctx_var()}}->_affinityCount);
        u32 i;
        void *data;
        for (i=0; i<affinityCount; i++) {
            ocrGuid_t a = {{util.g_ctx_var()}}->_affinities[i];
            if (ocrGuidIsEq(a, currentLoc)) {
                {{util.g_ctx_var()}}->_affinities[i] = contextGuid;
            }
            else {
                ocrHint_t hint;
                // IMPORTANT! We do NOT use _CNC_DBCREATE here because we do NOT want DB_PROP_SINGLE_ASSIGNMENT
                ocrDbCreate(&{{util.g_ctx_var()}}->_affinities[i], &data, ctxBytes, DB_PROP_NO_ACQUIRE,
                        _cncDbAffinityHint(&hint, a), NO_ALLOC);
            }
        }
    }
    #endif /* CNC_AFFINITIES */
    return {{util.g_ctx_var()}};
}

static void _initCtxColls({{util.g_ctx_param()}}) {
    // initialize item collections
    {% for i in g.concreteItems -%}
    {% if i.key -%}
    {{util.g_ctx_var()}}->_items.{{i.collName}} = _cncItemCollectionCreate();
    {% else -%}
    {{util.g_ctx_var()}}->_items.{{i.collName}} = _cncItemCollectionSingletonCreate();
    {% endif -%}
    {% endfor -%}
    // initialize step collections
    {% for s in g.finalAndSteps -%}
    ocrEdtTemplateCreate(&{{util.g_ctx_var()}}->_steps.{{s.collName}},
            _{{g.name}}_cncStep_{{s.collName}}, EDT_PARAM_UNK, EDT_PARAM_UNK);
    {% endfor -%}
}

#ifdef CNC_AFFINITIES
static ocrGuid_t _distInitEdt(u32 paramc, u64 paramv[], u32 depc, ocrEdtDep_t depv[]) {
    const u64 rank = paramv[0];
    {{util.g_ctx_t()}} *orig = depv[0].ptr;
    {{util.g_ctx_t()}} *local = depv[1].ptr;

    // copy user data and guids
    memcpy(local, orig, offsetof({{util.g_ctx_t()}}, _items));
    local->_guids.self = depv[1].guid;

    // set up collections
    _initCtxColls(local);

    // copy affinity info
    local->_rank = rank;
    local->_affinityCount = orig->_affinityCount;
    const u64 affinityBytes = sizeof(ocrGuid_t) * local->_affinityCount;
    memcpy(local->_affinities, orig->_affinities, affinityBytes);

    ocrDbRelease(local->_guids.self);

    ocrEventSatisfySlot(orig->_guids.contextReady, NULL_GUID, OCR_EVENT_LATCH_DECR_SLOT);

    return NULL_GUID;
}

static void _distSetup({{util.g_ctx_param()}}) {
    u64 r;
    const u64 ranks = {{util.g_ctx_var()}}->_affinityCount;
    const ocrGuid_t currentLoc = _cncCurrentAffinity();
    ocrGuid_t edtGuid, templGuid, loc;
    ocrEdtTemplateCreate(&templGuid, _distInitEdt, 1, 2);
    for (r=0; r<ranks; r++) {
        loc = _cncAffinityFromRank(r, {{util.g_ctx_var()}}->_affinities);
        if (ocrGuidIsEq(loc, currentLoc)) {
            {{util.g_ctx_var()}}->_rank = r;
            _initCtxColls({{util.g_ctx_var()}});
        }
        else {
            ocrEventSatisfySlot({{util.g_ctx_var()}}->_guids.contextReady, NULL_GUID, OCR_EVENT_LATCH_INCR_SLOT);
            const ocrGuid_t remoteCtx = {{util.g_ctx_var()}}->_affinities[r];
            ocrHint_t hint;
            ocrEdtCreate(&edtGuid, templGuid,
                    /*paramc=*/EDT_PARAM_DEF, /*paramv=*/&r,
                    /*depc=*/EDT_PARAM_DEF, /*depv=*/NULL,
                    /*properties=*/EDT_PROP_NONE,
                    /*hint=*/_cncEdtAffinityHint(&hint, loc), /*outEvent=*/NULL);
            ocrAddDependence({{util.g_ctx_var()}}->_guids.self, edtGuid, 0, DB_MODE_RO);
            ocrAddDependence(remoteCtx, edtGuid, 1, DB_DEFAULT_MODE);
        }
    }
    ocrEdtTemplateDestroy(templGuid);
}
#endif /* CNC_AFFINITIES */

void {{g.name}}_destroy({{util.g_ctx_param()}}) {
    ocrEventDestroy({{util.g_ctx_var()}}->_guids.finalizedEvent);
    ocrEventDestroy({{util.g_ctx_var()}}->_guids.quiescedEvent);
    ocrEventDestroy({{util.g_ctx_var()}}->_guids.doneEvent);
    // destroy item collections
    {% for i in g.concreteItems -%}
    {% if i.key -%}
    _cncItemCollectionDestroy({{util.g_ctx_var()}}->_items.{{i.collName}});
    {% else -%}
    _cncItemCollectionSingletonDestroy({{util.g_ctx_var()}}->_items.{{i.collName}});
    {% endif -%}
    {% endfor -%}
    // destroy step collections
    {% for s in g.finalAndSteps -%}
    ocrEdtTemplateDestroy({{util.g_ctx_var()}}->_steps.{{s.collName}});
    {% endfor -%}
    ocrDbDestroy({{util.g_ctx_var()}}->_guids.self);
}

// FIXME - shouldn't need this. that's why we have finalized vs done
static ocrGuid_t _emptyEdt(u32 paramc, u64 paramv[], u32 depc, ocrEdtDep_t depv[]) {
    return NULL_GUID;
}

/* EDT's output event fires when all compute steps are done AND graph is finalized (graph is DONE) */
static ocrGuid_t _graphFinishedEdt(u32 paramc, u64 paramv[], u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t finalizerResult = depv[1].guid;
    return finalizerResult;
}

/* EDT's output event fires when all compute steps are done (graph is quiesced) */
static ocrGuid_t _stepsFinishEdt(u32 paramc, u64 paramv[], u32 depc, ocrEdtDep_t depv[]) {
    {{util.g_args_param()}} = _cncItemDataPtr(depv[1].ptr);
    {{util.g_ctx_param()}} = depv[2].ptr;
    // XXX - finalizer should be a finish EDT?
    // The graph isn't done until the finalizer runs as well,
    // so we need to make a dummy EDT depending on the
    // finalizer's output event.
    ocrGuid_t emptyEdtGuid, templGuid;
    ocrEdtTemplateCreate(&templGuid, _emptyEdt, 0, 1);
    ocrHint_t hint;
    ocrEdtCreate(&emptyEdtGuid, templGuid,
        /*paramc=*/EDT_PARAM_DEF, /*paramv=*/NULL,
        /*depc=*/EDT_PARAM_DEF, /*depv=*/&{{util.g_ctx_var()}}->_guids.finalizedEvent,
        /*properties=*/EDT_PROP_NONE,
        /*hint=*/_cncCurrentEdtAffinityHint(&hint), /*outEvent=*/NULL);
    // XXX - destroying this template caused crash on FSim
    //ocrEdtTemplateDestroy(templGuid);
    // Start graph execution
    {{ util.step_enter() }}
    {{util.qualified_step_name(g.initFunction)}}({{util.g_args_var()}}, {{util.g_ctx_var()}});
    {{ util.step_exit() }}
    return NULL_GUID;
}

static ocrGuid_t _finalizerEdt(u32 paramc, u64 paramv[], u32 depc, ocrEdtDep_t depv[]) {
    {{util.g_ctx_param()}} = depv[0].ptr;
    cncTag_t *tag = depv[1].ptr; MAYBE_UNUSED(tag);
    cncPrescribe_{{g.finalizeFunction.collName}}(
        {%- for x in g.finalizeFunction.tag %}tag[{{loop.index0}}], {% endfor -%}
        {{util.g_ctx_var()}});
    // FIXME - I probably need to free this (the tag) sometime
    // XXX - for some reason this causes a segfault?
    //ocrDbDestroy(depv[1].guid);
    return NULL_GUID;
}

void {{g.name}}_launch({{util.g_args_param()}}, {{util.g_ctx_param()}}) {
    ocrGuid_t graphEdtGuid, finalEdtGuid, doneEdtGuid, edtTemplateGuid, outEventGuid;
    // args struct should already be in a datablock (allocated with cncItemAlloc)
    ocrGuid_t argsDbGuid = _cncItemGuid({{util.g_args_var()}});
    // XXX - should only do this if it wasn't already released? (should check release flag)
    // I probably need to refactor some code out from cncPut
    if ({{util.g_args_var()}}) ocrDbRelease(argsDbGuid);
    // affinity
    const ocrGuid_t _affinity = _cncCurrentAffinity();
    // create a finish EDT for the CnC graph
    {
        ocrEdtTemplateCreate(&edtTemplateGuid, _stepsFinishEdt, 0, 3);
        ocrHint_t _hint;
        ocrEdtCreate(&graphEdtGuid, edtTemplateGuid,
                /*paramc=*/EDT_PARAM_DEF, /*paramv=*/NULL,
                /*depc=*/EDT_PARAM_DEF, /*depv=*/NULL,
                /*properties=*/EDT_PROP_FINISH,
                /*hint=*/_cncEdtAffinityHint(&_hint, _affinity),
                /*outEvent=*/&outEventGuid);
        ocrEdtTemplateDestroy(edtTemplateGuid);
        // hook the graph's quiescedEvent into the graph's output event
        ocrAddDependence(outEventGuid, {{util.g_ctx_var()}}->_guids.quiescedEvent, 0, DB_DEFAULT_MODE);
        // don't start until the context is fully initialized
        ocrAddDependence({{util.g_ctx_var()}}->_guids.contextReady, graphEdtGuid, 0, DB_MODE_NULL);
    }
    // finish initializing the context
    {
        ocrEventSatisfySlot({{util.g_ctx_var()}}->_guids.contextReady, NULL_GUID, OCR_EVENT_LATCH_INCR_SLOT);
        #ifdef CNC_AFFINITIES
        _distSetup({{util.g_ctx_var()}});
        #else
        _initCtxColls({{util.g_ctx_var()}});
        #endif /* CNC_AFFINITIES */
        ocrEventSatisfySlot({{util.g_ctx_var()}}->_guids.contextReady, NULL_GUID, OCR_EVENT_LATCH_DECR_SLOT);
    }
    ocrDbRelease({{util.g_ctx_var()}}->_guids.self);
    // set up the finalizer
    {
        ocrEdtTemplateCreate(&edtTemplateGuid, _finalizerEdt, 0, 2);
        ocrGuid_t deps[] = { {{util.g_ctx_var()}}->_guids.self, {{util.g_ctx_var()}}->_guids.awaitTag };
        ocrHint_t _hint;
        ocrEdtCreate(&finalEdtGuid, edtTemplateGuid,
            /*paramc=*/EDT_PARAM_DEF, /*paramv=*/NULL,
            /*depc=*/EDT_PARAM_DEF, /*depv=*/deps,
            /*properties=*/EDT_PROP_FINISH,
            /*hint=*/_cncEdtAffinityHint(&_hint, _affinity),
            /*outEvent=*/&outEventGuid);
        ocrEdtTemplateDestroy(edtTemplateGuid);
        // hook the graph's finalizedEvent into the finalizer's output event
        ocrAddDependence(outEventGuid, {{util.g_ctx_var()}}->_guids.finalizedEvent, 0, DB_DEFAULT_MODE);
    }
    // set up the EDT that controls the graph's doneEvent
    {
        ocrEdtTemplateCreate(&edtTemplateGuid, _graphFinishedEdt, 0, 2);
        ocrGuid_t deps[] = { {{util.g_ctx_var()}}->_guids.quiescedEvent, {{util.g_ctx_var()}}->_guids.finalizedEvent };
        ocrHint_t _hint;
        ocrEdtCreate(&doneEdtGuid, edtTemplateGuid,
            /*paramc=*/EDT_PARAM_DEF, /*paramv=*/NULL,
            /*depc=*/EDT_PARAM_DEF, /*depv=*/deps,
            /*properties=*/EDT_PROP_NONE,
            /*hint=*/_cncEdtAffinityHint(&_hint, _affinity),
            /*outEvent=*/&outEventGuid);
        ocrEdtTemplateDestroy(edtTemplateGuid);
        ocrAddDependence(outEventGuid, {{util.g_ctx_var()}}->_guids.doneEvent, 0, DB_DEFAULT_MODE);
    }
    // start the graph execution
    ocrAddDependence(argsDbGuid, graphEdtGuid, 1, DB_DEFAULT_MODE);
    ocrAddDependence({{util.g_ctx_var()}}->_guids.self, graphEdtGuid, 2, DB_DEFAULT_MODE);
}

void {{g.name}}_await({{
        util.print_tag(g.finalizeFunction.tag, typed=True)
        }}{{util.g_ctx_param()}}) {
    // Can't launch the finalizer EDT from within the finish EDT,
    // so we copy the tag information into a DB and do it indirectly.
    {% if g.finalizeFunction.tag -%}
    cncTag_t *_tagPtr;
    ocrGuid_t _tagGuid;
    int _i = 0;
    _CNC_DBCREATE(&_tagGuid, (void**)&_tagPtr, sizeof(cncTag_t) * {{ g.finalizeFunction.tag|count}});
    {% for x in g.finalizeFunction.tag -%}
    _tagPtr[_i++] = {{x}};
    {% endfor -%}
    ocrDbRelease(_tagGuid);
    {% else -%}
    ocrGuid_t _tagGuid = NULL_GUID;
    {% endif -%}
    ocrEventSatisfy({{util.g_ctx_var()}}->_guids.awaitTag, _tagGuid);
}

/* define NO_CNC_MAIN if you want to use mainEdt as the entry point instead */
#ifndef NO_CNC_MAIN

extern int cncMain(int argc, char *argv[]);

ocrGuid_t mainEdt(u32 paramc, u64 paramv[], u32 depc, ocrEdtDep_t depv[]) {
    // Unpack argc and argv (passed thru from mainEdt)
    int i, argc = OCR_MAIN_ARGC;
    char **argv = cncLocalAlloc(sizeof(char*)*argc);
    for (i=0; i<argc; i++) argv[i] = OCR_MAIN_ARGV(i);
    // Run user's cncEnvIn function
    cncMain(argc, argv);
    cncLocalFree(argv);
    return NULL_GUID;
}

#endif /* NO_CNC_MAIN */

