{% import "common_macros.inc.c" as util with context -%}
{{ util.auto_file_banner() }}

{% set defname = "_CNC_" ~ g.name.upper() ~ "_H_" -%}
#ifndef {{defname}}
#define {{defname}}

{#- /* TODO - Should refactor and include GraphName_external.h instead */ #}
#include "{{g.name}}_context.h"
#include "cnc_common.h"

/***************************\
 ******** CNC GRAPH ********
\***************************/

{{util.g_ctx_t()}} *{{g.name}}_create(void);
void {{g.name}}_destroy({{util.g_ctx_param()}});

void {{g.name}}_launch({{g.name}}Args *args, {{util.g_ctx_param()}});
void {{g.name}}_await({{
        util.print_tag(g.finalizeFunction.tag, typed=True)
        }}{{util.g_ctx_param()}});

/**********************************\
 ******** ITEM KEY STRUCTS ********
\**********************************/

{% for name, i in g.itemDeclarations.items() if not i.isSingleton -%}
typedef struct { cncTag_t {{ i.key|join(", ") }}; } {{name}}ItemKey;
{% endfor %}
{% if g.externVms -%}
/****************************************\
 ******** ITEM MAPPING FUNCTIONS ********
\****************************************/

{% for i in g.externVms -%}
{{i.mapTarget}}ItemKey {{i.functionName}}({{
  util.print_tag(i.key, typed=True)
  }}{{util.g_ctx_param()}});
{% endfor %}
{% endif -%}
/**************************\
 ******** ITEM PUT ********
\**************************/
{% for name, i in g.itemDeclarations.items() %}
{# /* TODO - ADD NAMESPACE PREFIX DEFINE THING */ -#}
// {{i.collName}}

void cncPut_{{name}}({{i.type.ptrType}}_item, {{
        util.print_tag(i.key, typed=True)
        }}{{util.g_ctx_param()}});
{% endfor %}
/************************************\
 ******** STEP PRESCRIPTIONS ********
\************************************/

{% for stepfun in g.finalAndSteps -%}
void cncPrescribe_{{stepfun.collName}}({{
        util.print_tag(stepfun.tag, typed=True) }}{{util.g_ctx_param()}});
{% endfor %}

#endif /*{{defname}}*/
