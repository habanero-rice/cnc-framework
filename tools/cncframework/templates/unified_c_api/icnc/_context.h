{% import "common_macros.inc.c" as util with context -%}
{{ util.auto_file_banner() }}

{% set defname = "_CNC_" ~ g.name.upper() ~ "_CONTEXT_H_" -%}
#ifndef {{defname}}
#define {{defname}}

#include "icnc.h"
#include "{{g.name}}_defs.h"

typedef struct {{g.name}}Context {
    size_t _externCtxOffset;
{%- for line in g.ctxParams %}
    {{ line }}
{%- endfor %}
} {{util.g_ctx_t()}};

#endif /*{{defname}}*/

