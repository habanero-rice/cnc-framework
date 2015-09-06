{% import "common_macros.inc.c" as util with context -%}
{{ util.auto_file_banner() }}
{#/****** Item instance data cast ******/-#}
{%- macro unpack_item(i) -%}
{%- with itemType = g.lookupType(i) -%}
{%- if not itemType.isPtrType %}*{% endif -%}
{{ "(" ~ itemType.ptrType ~ ")" }}
{%- endwith -%}
{%- endmacro -%}

#include "{{g.name}}_internal.h"

#ifdef CNC_DEBUG_LOG
#if !defined(CNCOCR_x86)
#error "Debug logging mode only supported on x86 targets"
#endif
#include <pthread.h>
extern pthread_mutex_t _cncDebugMutex;
#endif /* CNC_DEBUG_LOG */

{% for stepfun in g.finalAndSteps %}
{% set isFinalizer = loop.first -%}
/* {{stepfun.collName}} setup/teardown function */
void _{{g.name}}_cncStep_{{stepfun.collName}}({{ util.print_tag(stepfun.tag, typed=True) ~ util.g_ctx_param()}}) {
    {% for x in stepfun.tag -%}
    MAYBE_UNUSED({{x}});
    {% endfor -%}
    {#-/****** Set up input items *****/#}
    {% set inputIsEnabled = [ true ] -%}
    {%- call util.render_indented(1) -%}
{% for input in stepfun.inputItems -%}
{#/* ranged items */-#}
{% if input.keyRanges -%}
{{ util.ranged_type(input) ~ input.binding }};
{#/* scalar items */-#}
{% else -%}
{{ g.lookupType(input) ~ input.binding}};
{% endif -%}
{% endfor %}
{% for input in stepfun.inputs recursive -%}
{#/* CONDITIONALS (recursive case) */-#}
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
{#/* ITEMS (base case) */-#}
{% else %}
{#/* ranged items */-#}
{% if input.keyRanges -%}
{% set varPrefix = "_raw_" -%}
{{ g.lookupType(input) ~ "*" ~ varPrefix ~ input.binding}};
//{{ util.ranged_type(input) ~ input.binding }};
{
    u32 _dims[] = { {{input.keyRanges|join(", ", attribute='sizeExpr')}} };
    {{input.binding}} = _cncRangedInputAlloc({{ input.keyRanges|count
            }}, _dims, sizeof({{ g.lookupType(input) }}), (void**)&{{varPrefix~input.binding}});
}
{#/* scalar items */-#}
{% else -%}
{% set varPrefix = "" -%}
//{{ g.lookupType(input) ~ input.binding}};
{% endif %}
{%- set comment = "Set up \"" ~ input.binding ~ "\" input dependencies" -%}
{%- call(var) util.render_tag_nest(comment, input, useTag=inputIsEnabled[-1]) -%}
{%- if inputIsEnabled[-1] -%}
{% if input.keyRanges -%}
*({{varPrefix ~ input.binding}}++)
{%- else -%}
{{var}}
{%- endif %} = {{ unpack_item(input) }}cncGet_{{input.collName}}({%- for k in input.key %}_i{{loop.index0}}, {% endfor -%}{{util.g_ctx_var()}});
{%- else -%}
{{var}} = NULL;
{%- endif -%}
{%- endcall -%}
{% endif %}
{% endfor %}
{% endcall %}
    {{ util.step_enter() }}
    // Call user-defined step function
    {{ util.log_msg("RUNNING", stepfun.collName, stepfun.tag) }}
    {{util.qualified_step_name(stepfun)}}({{ util.print_tag(stepfun.tag)
            ~ util.print_bindings(stepfun.inputItems) }}{{util.g_ctx_var()}});
    // Clean up
    {% for input in stepfun.rangedInputItems -%}
    cncLocalFree({{input.binding}});
    {% endfor -%}
    {{ util.log_msg("DONE", stepfun.collName, stepfun.tag) }}
    {{ util.step_exit() }}
}

{% endfor %}

