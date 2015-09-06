{#/****** Warning banner for auto-generated files ******/#}
{% macro auto_file_banner() -%}
/**********************************************\
 *  WARNING: AUTO-GENERATED FILE!             *
 *  This file WILL BE OVERWRITTEN on each     *
 *  invocation of the graph translator tool.  *
\**********************************************/
{%- endmacro %}


{#/****** Indent calling block to the specified level ******/#}
{% macro log_msg(msgType, collName, tag, indent=0) -%}
{% call render_indented(1) -%}
#ifdef CNC_DEBUG_LOG
{%- endcall %}
{%- call render_indented(indent+1) %}
    fprintf(cncDebugLog, "{{msgType}} {{collName}} @ {{
            (['%ld'] * tag|count)|join(', ') if tag else 0 }}\n"{{
            ([""] + tag|list)|join(', ') }});
    fflush(cncDebugLog);
#elif CNC_DEBUG_TRACE
    printf("<<CnC Trace>>: {{msgType}} {{collName}} @ {{
           (['%ld'] * tag|count)|join(', ') if tag else 0 }}\n"{{
           ([""] + tag|list)|join(', ') }});
#endif
{%- endcall %}
{%- endmacro %}

{#/****** Context struct type ******/#}
{% macro g_ctx_t() -%}
{{g.name}}Ctx
{%- endmacro %}

{#/****** Context variable name ******/#}
{% macro g_ctx_var() -%}
ctx
{%- endmacro %}

{#/****** Context variable declaration ******/#}
{% macro g_ctx_param() -%}
{{g_ctx_t()}} *{{g_ctx_var()}}
{%- endmacro %}

{#/****** Init-args struct type ******/#}
{% macro g_args_t() -%}
{{g.name}}Args
{%- endmacro %}

{#/****** Init-args variable name ******/#}
{% macro g_args_var() -%}
args
{%- endmacro %}

{#/****** Init-args variable declaration ******/#}
{% macro g_args_param() -%}
{{g_args_t()}} *{{g_args_var()}}
{%- endmacro %}

{#/****** Step name qualified with graph name ******/#}
{% macro qualified_step_name(s) -%}
{{g.name}}_{{s.collName}}
{%- endmacro %}

{#/****** CnC Item Create call + variable declaration ******/#}
{% macro item_create_statement(itemcoll, varname) -%}
{% set isVec = itemcoll.type.isVecType -%}
{% if isVec -%}
{% set N = itemcoll.type.vecSize or "/*TODO: N=*/1" -%}
{% set safeN = N if N.isalnum() else "("~N~")" -%}
{% set multiple = " * "~safeN -%}
{% else -%}
{% set multiple = "" -%}
{% endif -%}
{{itemcoll.type.ptrType ~ varname}} = cncItemAlloc(sizeof(*{{varname}}){{multiple}});
{%- endmacro %}

{#/****** Indent calling block to the specified level ******/#}
{% macro render_indented(level) -%}
{{ caller()|indent(width=4*level) }}
{%- endmacro %}

{#/****** Print all the components of a key or tag ******/#}
{% macro print_tag(tag, typed=False, prefix="") -%}
{% for x in tag %}{% if typed %}cncTag_t {% endif %}{{prefix ~ x}}, {% endfor -%}
{%- endmacro %}

{#/****** Print ranged type for item collection ******/#}
{% macro ranged_type(item) -%}
{{ g.lookupType(item)
 }}{{ ("*" * item.keyRanges|count) }}
{%- endmacro %}

{#/****** Print bindings for a list of items ******/#}
{% macro print_bindings(items, typed=False, prefix="") -%}
{% for i in items %}
{%- if typed %}{{ ranged_type(i) }}{% endif -%}
{{ prefix ~ i.binding }}, {% endfor -%}
{%- endmacro %}

{#/****** Print indices for an array access ******/#}
{% macro print_indices(xs) -%}
{% for x in xs %}[{{x}}]{% endfor -%}
{%- endmacro %}

{#/****** Print indices for an array access ******/#}
{% macro range_cmp_op(r) -%}
{{ "<=" if r.inclusive else "<" }}
{%- endmacro %}

{#/* TODO: There should be a way to combine the following two macros
     (especially now since I lifted out the memory allocation stuff) */#}
{#/****** For-loop nest for iterating over a multi-dimentional
          item array based on a ranged tag function ******/#}
{% macro render_tag_nest(comment, item, useTag=False, prefix="") %}
{% set ranges = [] -%}
{% macro iVarIndexed() %}{{prefix ~ item.binding ~ ranges|join}}{% endmacro -%}
{ // {{comment}}
{%- for k in item.key -%}
{% call render_indented(1 + ranges|count) -%}
{% set idx = "_i" ~ loop.index0 -%}
{% if k.isRanged %}{#/* Range */#}
s64 {{idx}};
for ({{idx}} = {{k.start}}; {{idx}} {{range_cmp_op(k)}} {{k.end}}; {{idx}}++) {
{%- do ranges.append("["~idx~"]") -%}
{%- elif useTag %}{#/* Scalar (if used) */#}
s64 {{idx}} = {{k.expr}};
{%- endif -%}
{%- endcall -%}
{%- endfor -%}
{% set content = caller(iVarIndexed()) -%}
{%- call render_indented(1 + ranges|count) %}
{{ content }}
{%- endcall -%}
{%- for r in item.keyRanges -%}
{% call render_indented(1 + loop.revindex0) %}
}
{%- endcall -%}
{%- endfor %}
}
{%- endmacro %}

{#/****** For-loop nest for iterating over a multi-dimentional
          item array based on a ranged tag function ******/#}
{% macro render_io_nest(comment, tag, bindings, zeroBased=False) %}
{% set ranges = [] -%}
{% set args = [] -%}
{%- for x in tag -%}
{% if x.isRanged %}{#/* Range */#}
{%- set idx = "_" ~ bindings[loop.index0] -%}
{%- do ranges.append([idx, x]) -%}
{%- do args.append(idx) -%}
{%- else %}{#/* Scalar */#}
{%- do args.append(x.expr) -%}
{%- endif -%}
{%- endfor -%}
{% if ranges -%}
{ // {{comment}}
    s64 {{ranges|join(", ", attribute=0)}};
{%- for idx, x in ranges -%}
{% call render_indented(loop.index) %}
{% set startVal = 0 if zeroBased else x.start -%}
{% set endVal = x.upperLoopBound if zeroBased else x.end -%}
for ({{idx}} = {{startVal}}; {{idx}} {{range_cmp_op(x)}} {{endVal}}; {{idx}}++) {
{%- endcall -%}
{%- endfor -%}
{% set content = caller(args, ranges|map('first')|list) -%}
{%- call render_indented(1 + ranges|count) %}
{{ content }}
{%- endcall -%}
{%- for x in ranges -%}
{% call render_indented(1 + loop.revindex0) %}
}
{%- endcall -%}
{%- endfor %}
}
{% else -%}
// {{comment}}
{{ caller(args, ranges) }}
{% endif -%}
{%- endmacro %}

{#/* Scaffolding code generation for step inputs */#}
{% macro step_input_scaffolding(stepfun, indent=1) %}
{% set rangedInputs = stepfun.inputs|selectattr('keyRanges')|list -%}
{% if rangedInputs %}
    //
    // INPUTS
    //
{% for input in rangedInputs -%}
{%- set comment = "Access \"" ~ input.binding ~ "\" inputs" -%}
{%- set decl = g.itemDeclarations[input.collName] -%}
{%- call render_indented(1) -%}
{%- call(args, ranges) render_io_nest(comment, input.key, decl.key, zeroBased=True) -%}
{%- set var = input.binding ~ print_indices(ranges) -%}
/* TODO: Do something with {{var}} */
{%- endcall -%}
{%- endcall %}
{% endfor %}
{% endif %}
{%- endmacro %}

{% macro step_enter() -%}
#ifdef CNC_DEBUG_LOG
        pthread_mutex_lock(&_cncDebugMutex);
    #endif
{%- endmacro %}
{% macro step_exit() -%}
#ifdef CNC_DEBUG_LOG
        pthread_mutex_unlock(&_cncDebugMutex);
    #endif
{%- endmacro %}

{% macro render_step_outputs(outputs) -%}
{% for output in outputs recursive -%}
{% if output.kind == 'ITEM' -%}
{%- set comment = "Put \"" ~ output.binding ~ "\" items" -%}
{%- set decl = g.itemDeclarations[output.collName] -%}
{%- call(args, ranges) render_io_nest(comment, output.key, decl.key) -%}
{%- set var = output.binding ~ print_indices(ranges) -%}
{{ item_create_statement(decl, output.binding) }}
/* TODO: Initialize {{output.binding}} */
cncPut_{{output.collName}}({{output.binding}}, {{ print_tag(args) }}{{g_ctx_var()}});
{%- endcall -%}
{% elif output.kind == 'STEP' -%}
{%- set comment = "Prescribe \"" ~ output.collName ~ "\" steps" -%}
{%- set decl = g.stepFunctions[output.collName] -%}
{%- call(args, ranges) render_io_nest(comment, output.tag, decl.tag) -%}
cncPrescribe_{{output.collName}}({{ print_tag(args) }}{{g_ctx_var()}});
{%- endcall -%}
{% elif output.kind == 'IF' %}
if ({{ output.cond }}) {
{%- call render_indented(1) -%}
{{ loop(output.refs) }}
{%- endcall %}
}
{% elif output.kind == 'ELSE' -%}
else {
{%- call render_indented(loop.depth) -%}
{{ loop(output.refs) }}
{%- endcall -%}
}
{% else %}
{% do exit("Unknown output type:" + (output|string)) %}
{% endif -%}
{% endfor -%}
{% endmacro %}


{% macro render_step_inputs(rangedIns) -%}
{% for input in rangedIns %}
{%- set comment = "Access \"" ~ input.binding ~ "\" inputs" -%}
{%- set decl = g.itemDeclarations[input.collName] -%}
{%- call(args, ranges) render_io_nest(comment, input.key, decl.key, zeroBased=True) -%}
{%- set var = input.binding ~ print_indices(ranges) -%}
/* TODO: Do something with {{var}} */
{%- endcall %}
{% endfor -%}
{% endmacro %}
