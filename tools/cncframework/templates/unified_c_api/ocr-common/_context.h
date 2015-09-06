{% import "common_macros.inc.c" as util with context -%}
{{ util.auto_file_banner() }}

{% set defname = "_CNC_" ~ g.name.upper() ~ "_CONTEXT_H_" -%}
#ifndef {{defname}}
#define {{defname}}

#include "cncocr.h"
#include "{{g.name}}_defs.h"

typedef struct {{g.name}}Context {
{%- for line in g.ctxParams %}
    {{ line }}
{%- endfor %}
    struct {
        ocrGuid_t self;
        ocrGuid_t finalizedEvent;
        ocrGuid_t quiescedEvent;
        ocrGuid_t doneEvent;
        ocrGuid_t awaitTag;
        ocrGuid_t contextReady;
    } _guids;
    struct {
        {%- for i in g.concreteItems %}
        cncItemCollection_t {{i.collName}};
        {%- endfor %}
    } _items;
    struct {
        {%- for s in g.finalAndSteps %}
        ocrGuid_t {{s.collName}};
        {%- endfor %}
    } _steps;
#ifdef CNC_AFFINITIES
    cncLocation_t _rank;
    u64 _affinityCount;
    ocrGuid_t _affinities[];
#endif /* CNC_AFFINITIES */
} {{util.g_ctx_t()}};

#endif /*{{defname}}*/
