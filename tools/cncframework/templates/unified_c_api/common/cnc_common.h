{% import "common_macros.inc.c" as util with context -%}
{{ util.auto_file_banner() }}

{% set defname = "_CNC_COMMON_H_" -%}
#ifndef {{defname}}
#define {{defname}}

#include "{{cncRuntimeName}}.h"

void *_cncRangedInputAlloc(u32 n, u32 dims[], size_t itemSize, void **dataStartPtr);

#endif /*{{defname}}*/
