{% import "common_macros.inc.c" as util with context -%}

#include "{{g.name}}.h"

{#/* TODO - eliminate code reuse between here and StepFunc.c */#}
void {{util.qualified_step_name(g.initFunction)}}({{util.g_args_param()}}, {{util.g_ctx_param()}}) {
{% if g.initFunction.tag %}
    // TODO: Initialize these tag variables using args
    cncTag_t {{ g.initFunction.tag|join(", ") }};
{% endif %}
{% call util.render_indented(1) -%}
{{ util.render_step_outputs(g.initFunction.outputs) }}
{% endcall %}
    // Set finalizer function's tag
    {{g.name}}_await({{util.print_tag(g.finalizeFunction.tag)}}{{util.g_ctx_var()}});

}


{#/* XXX -  This code is copied from StepFunc.c */-#}
{% set stepfun = g.finalizeFunction -%}
void {{util.qualified_step_name(stepfun)}}({{ util.print_tag(stepfun.tag, typed=True)
        }}{{ util.print_bindings(stepfun.inputs, typed=True)
        }}{{util.g_ctx_param()}}) {
{% for input in stepfun.inputItems -%}
{% if input.keyRanges %}
{%- set comment = "Access \"" ~ input.binding ~ "\" inputs" -%}
{%- set decl = g.itemDeclarations[input.collName] -%}
{%- call util.render_indented(1) -%}
{%- call(args, ranges) util.render_io_nest(comment, input.key, decl.key, zeroBased=True) -%}
{%- set var = input.binding ~ util.print_indices(ranges) -%}
/* TODO: Do something with {{var}} */
{%- endcall -%}
{%- endcall -%}
{% else %}
    /* TODO: Do something with {{input.binding}} */
{% endif -%}
{% endfor %}
}
{% if g.externVms %}

{% for i in g.externVms -%}
/* Mapping {{i.collName}} onto {{i.mapTarget}} */
{{i.mapTarget}}ItemKey {{i.functionName}}({{
  util.print_tag(i.key, typed=True)
  }}{{util.g_ctx_param()}}) {
    {{i.mapTarget}}ItemKey _result;
    {% for x in g.itemDeclarations[i.mapTarget].key -%}
    _result.{{x}} = /*TODO*/0;
    {% endfor -%}
    return _result;
}
{% endfor %}
{% endif %}

