{% import "common_macros.inc.c" as util with context -%}
{{ util.auto_file_banner() }}

#include "{{g.name}}_internal.h"

{#/****** Item instance data cast ******/-#}
{%- macro unpack_item(i) -%}
{%- with itemType = g.lookupType(i) -%}
{%- if not itemType.isPtrType %}*{% endif -%}
{{ "(" ~ itemType.ptrType ~ ")" }}
{%- endwith -%}
{%- endmacro -%}

#ifdef CNC_DEBUG_LOG
#if !defined(CNCOCR_x86)
#error "Debug logging mode only supported on x86 targets"
#endif
#include <pthread.h>
extern pthread_mutex_t _cncDebugMutex;
#endif /* CNC_DEBUG_LOG */

{% for stepfun in g.finalAndSteps %}
{% set isFinalizer = loop.first -%}
{% set paramTag = (stepfun.tag|count) <= 8 -%}
/* {{stepfun.collName}} setup/teardown function */
ocrGuid_t _{{g.name}}_cncStep_{{stepfun.collName}}(u32 paramc, u64 paramv[], u32 depc, ocrEdtDep_t depv[]) {
    {{util.g_ctx_param()}} = depv[0].ptr;

    u64 *_tag = {{ "paramv" if paramTag else "depv[1].ptr" }}; MAYBE_UNUSED(_tag);
    {% for x in stepfun.tag -%}
    const cncTag_t {{x}} = (cncTag_t)_tag[{{loop.index0}}]; MAYBE_UNUSED({{x}});
    {% endfor -%}
    {% if not paramTag -%}
    ocrDbDestroy(depv[1].guid); // free tag component datablock
    {% endif %}
    s32 _edtSlot = {{ 1 if paramTag else 2 }}; MAYBE_UNUSED(_edtSlot);
    {#-/****** Set up input items *****/#}
    {% for input in stepfun.inputItems %}
    {% if input.keyRanges -%}
    {#/*RANGED*/-#}
    {{ util.ranged_type(input) ~ input.binding }};
    { // Init ranges for "{{input.binding}}"
        u32 _i;
        u32 _itemCount = {{input.keyRanges|join("*", attribute='sizeExpr')}};
        u32 _dims[] = { {{input.keyRanges|join(", ", attribute='sizeExpr')}} };
        {{ g.lookupType(input) }}*_item;
        {{input.binding}} = _cncRangedInputAlloc({{ input.keyRanges|count
                }}, _dims, sizeof({{ g.lookupType(input) }}), (void**)&_item);
        for (_i=0; _i<_itemCount; _i++) {
            _item[_i] = {{unpack_item(input)}}_cncItemDataPtr(depv[_edtSlot++].ptr);
        }
    }
    {% else -%}
    {#/*SCALAR*/-#}
    {{ g.lookupType(input) ~ input.binding }};
    {{input.binding}} = {{unpack_item(input)}}_cncItemDataPtr(depv[_edtSlot++].ptr);
    {% endif -%}
    {% endfor %}
    {{ util.step_enter() }}
    // Call user-defined step function
    {{ util.log_msg("RUNNING", stepfun.collName, stepfun.tag) }}
    {{util.qualified_step_name(stepfun)}}({{ util.print_tag(stepfun.tag) ~ util.print_bindings(stepfun.inputItems) }}{{util.g_ctx_var()}});
    // Clean up
    {% for input in stepfun.rangedInputItems -%}
    cncLocalFree({{input.binding}});
    {% endfor -%}
    {{ util.log_msg("DONE", stepfun.collName, stepfun.tag) }}
    {{ util.step_exit() }}
    return NULL_GUID;
}

/* {{stepfun.collName}} task creation */
static void cncPrescribeInternal_{{stepfun.collName}}({{
        ("" if paramTag else "ocrGuid_t _tagGuid, ")
        }}u64 *_tag, {{ util.g_ctx_param()}}) {
    {% for x in stepfun.tag -%}
    const cncTag_t {{x}} = (cncTag_t)_tag[{{loop.index0}}]; MAYBE_UNUSED({{x}});
    {% endfor -%}

    ocrGuid_t _stepGuid;
    {# /* TODO - figure out if there's a way to compute the size of non-rectangular
          ranges (e.g. _i3={0.._i1}). Writing out a whole loop nest, and then
          letting the compiler optimize it away to a constant in the trivial
          (rectangular) case might be possible. */ -#}
    u64 _depc = {{stepfun.inputCountExpr}} + {{ 1 if paramTag else 2 }};
    ocrHint_t _hint;
    {% if g.hasTuning('priority') -%}
    ocrHint_t *const _hintPtr = &_hint;
    if (!_cncCurrentEdtAffinityHint(_hintPtr)) {
        ocrHintInit(_hintPtr, OCR_HINT_EDT_T);
    }
    s64 _priorityWeight = {{g.priorityFn(stepfun.collName, util.g_ctx_var()~"->_affinityCount")}};
    ocrSetHintValue(_hintPtr, OCR_HINT_EDT_PRIORITY, _priorityWeight);
    {%- else -%}
    ocrHint_t *const _hintPtr = _cncCurrentEdtAffinityHint(&_hint);
    {%- endif %}
    ocrEdtCreate(&_stepGuid, {{util.g_ctx_var()}}->_steps.{{stepfun.collName}},
        {% if paramTag -%}
        /*paramc=*/{{(stepfun.tag|count)}}, /*paramv=*/_tag,
        {% else -%}
        /*paramc=*/0, /*paramv=*/NULL,
        {% endif -%}
        /*depc=*/_depc, /*depv=*/NULL,
        /*properties=*/EDT_PROP_NONE,
        /*hint=*/_hintPtr, /*outEvent=*/NULL);

    s32 _edtSlot = 0; MAYBE_UNUSED(_edtSlot);
    ocrAddDependence({{util.g_ctx_var()}}->_guids.self, _stepGuid, _edtSlot++, _CNC_AUX_DATA_MODE);
    {% if not paramTag -%}
    ocrAddDependence(_tagGuid, _stepGuid, _edtSlot++, _CNC_AUX_DATA_MODE);
    {% endif -%}

    {#-/****** Set up input items *****/#}
    {% set inputIsEnabled = [ true ] -%}
    {%- call util.render_indented(1) -%}
{% for input in stepfun.inputs recursive -%}
{% if input.kind in ['IF', 'ELSE'] -%}
if ({{ input.cond }}) {
{%- call util.render_indented(1) -%}
{{ loop(input.refs) }}
{%- endcall %}
}
{% do inputIsEnabled.append(false) -%}
else {
{%- call util.render_indented(1) -%}
{{ loop(input.refs) }}
{%- endcall %}
}
{% do inputIsEnabled.pop() -%}
{% else -%}
{%- set comment = "Set up \"" ~ input.binding ~ "\" input dependencies" -%}
{%- call(var) util.render_tag_nest(comment, input, useTag=inputIsEnabled[-1]) -%}
{#/* FIXME: shouldn't even do gets if the input is disabled,
     but that will require a more complicated calculation on the
     dependence count. I could do something where the complicated count
     is only used for steps with conditional ranged inputs... */-#}
{%- if inputIsEnabled[-1] -%}
cncGet_{{input.collName}}(
        {%- for k in input.key %}_i{{loop.index0}}, {% endfor -%}
         _stepGuid, _edtSlot++, _CNC_ITEM_GET_MODE, {{util.g_ctx_var()}});
{%- else -%}
ocrAddDependence(NULL_GUID, _stepGuid, _edtSlot++, DB_MODE_NULL);
{%- endif -%}
{%- endcall -%}
{% endif %}
{% endfor %}
{% endcall %}
    ASSERT(_depc == _edtSlot);
    {{ util.log_msg("PRESCRIBED", stepfun.collName, stepfun.tag) }}
}

#ifdef CNC_AFFINITIES
static ocrGuid_t _cncRemotePrescribe_{{stepfun.collName}}(u32 paramc, u64 paramv[], u32 depc, ocrEdtDep_t depv[]) {
    {{util.g_ctx_param()}} = depv[0].ptr;

    cncPrescribeInternal_{{stepfun.collName}}({{
            ("paramv, " if paramTag else "depv[1].guid, depv[1].ptr, ")
            ~ util.g_ctx_var() }});

    return NULL_GUID;
}
#endif /* CNC_AFFINITIES */

/* {{stepfun.collName}} task creation */
void cncPrescribe_{{stepfun.collName}}({{
        util.print_tag(stepfun.tag, typed=True)
        ~ util.g_ctx_param() }}) {
    {% if stepfun.tag -%}
    u64 _args[] = { (u64){{ stepfun.tag|join(", (u64)") }} };
    {% if not paramTag -%}
    ocrGuid_t _tagBlockGuid;
    u64 *_tagBlockPtr;
    _CNC_DBCREATE(&_tagBlockGuid, (void**)&_tagBlockPtr, sizeof(_args));
    hal_memCopy(_tagBlockPtr, _args, sizeof(_args), 0);
    {% endif -%}
    {% else -%}
    u64 *_args = NULL;
    {% endif -%}
    // affinity
    #ifdef CNC_AFFINITIES
    const cncLocation_t _loc = {{g.stepDistFn(stepfun.collName, util.g_ctx_var()~"->_affinityCount")}};
    if (_loc != {{util.g_ctx_var()}}->_rank) {
        const ocrGuid_t _remoteCtx = {{util.g_ctx_var()}}->_affinities[_loc];
        const ocrGuid_t _affinity = _cncAffinityFromCtx(_remoteCtx);
        {% if paramTag -%}
        const u32 _argCount = {{stepfun.tag|count}};
        const u32 _depCount = 1;
        ocrGuid_t _deps[] = { _remoteCtx };
        {% else -%}
        u64 *_args = NULL; // shadowing
        const u32 _argCount = 0;
        const u32 _depCount = 2;
        ocrGuid_t _deps[] = { _remoteCtx, _tagBlockGuid };
        ocrDbRelease(_tagBlockGuid);
        {% endif -%}
        // XXX - should just create this template along with the step function template
        ocrGuid_t edtGuid, templGuid;
        ocrEdtTemplateCreate(&templGuid, _cncRemotePrescribe_{{stepfun.collName}}, _argCount, _depCount);
        ocrHint_t _hint;
        ocrEdtCreate(&edtGuid, templGuid,
                /*paramc=*/_argCount, /*paramv=*/_args,
                /*depc=*/_depCount, /*depv=*/_deps,
                /*properties=*/EDT_PROP_NONE,
                /*hint=*/_cncEdtAffinityHint(&_hint, _affinity),
                /*outEvent=*/NULL);
        ocrEdtTemplateDestroy(templGuid);
        return;
    }
    #endif /* CNC_AFFINITIES */
    cncPrescribeInternal_{{stepfun.collName}}({{
            ("_args, " if paramTag else "_tagBlockGuid, _tagBlockPtr, ")
            ~ util.g_ctx_var() }});
}
{% endfor %}

