{% import "common_macros.inc.c" as util with context -%}
{{ util.auto_file_banner() }}

{% set defname = "_CNCOCR_" ~ g.name.upper() ~ "_INTERNAL_H_" -%}
#ifndef {{defname}}
#define {{defname}}

#include "{{g.name}}.h"
#include "icnc.h"


/******************************\
 ******** ITEM GETTERS ********
\******************************/

{% for name, i in g.itemDeclarations.items() %}
{{i.type.ptrType}}cncGet_{{name}}({{ util.print_tag(i.key, typed=True) ~ util.g_ctx_param()}});
{% endfor %}

/********************************\
 ******** STEP FUNCTIONS ********
\********************************/

void {{util.qualified_step_name(g.initFunction)}}({{util.g_args_param()}}, {{util.g_ctx_param()}});
{% for stepfun in g.finalAndSteps %}
void {{util.qualified_step_name(stepfun)}}({{
        util.print_tag(stepfun.tag, typed=True)}}{{
        util.print_bindings(stepfun.inputItems, typed=True)
        }}{{util.g_ctx_param()}});
void _{{g.name}}_cncStep_{{stepfun.collName}}({{ util.print_tag(stepfun.tag, typed=True) ~ util.g_ctx_param()}});
{% endfor %}
#endif /*{{defname}}*/
