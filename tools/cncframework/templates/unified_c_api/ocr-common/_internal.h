{% import "common_macros.inc.c" as util with context -%}
{{ util.auto_file_banner() }}

{% set defname = "_CNCOCR_" ~ g.name.upper() ~ "_INTERNAL_H_" -%}
#ifndef {{defname}}
#define {{defname}}

#include "{{g.name}}.h"
#include "cncocr_internal.h"

/********************************\
 ******** ITEM FUNCTIONS ********
\********************************/

{% for name, i in g.itemDeclarations.items() %}
void cncGet_{{name}}({{ util.print_tag(i.key, typed=True) }}ocrGuid_t destination, u32 slot, ocrDbAccessMode_t mode, {{util.g_ctx_param()}});
{% endfor %}

#ifdef CNC_AFFINITIES
{% for name, i in g.itemDeclarations.items() -%}
static inline cncLocation_t _cncItemDistFn_{{name}}({{ util.print_tag(i.key, typed=True) ~ util.g_ctx_param()}}) { return {{g.itemDistFn(name, util.g_ctx_var()~"->_affinityCount")}}; }
{% endfor -%}
#endif /* CNC_AFFINITIES */

/********************************\
 ******** STEP FUNCTIONS ********
\********************************/

void {{util.qualified_step_name(g.initFunction)}}({{g.name}}Args *args, {{util.g_ctx_param()}});
{% for stepfun in g.finalAndSteps %}
void {{util.qualified_step_name(stepfun)}}({{
        util.print_tag(stepfun.tag, typed=True)}}{{
        util.print_bindings(stepfun.inputItems, typed=True)
        }}{{util.g_ctx_param()}});
ocrGuid_t _{{g.name}}_cncStep_{{stepfun.collName}}(u32 paramc, u64 paramv[], u32 depc, ocrEdtDep_t depv[]);
{% endfor %}
#endif /*{{defname}}*/
