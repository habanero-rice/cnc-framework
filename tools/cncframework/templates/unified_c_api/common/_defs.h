{% import "common_macros.inc.c" as util with context -%}

{% set defname = "_CNCOCR_" ~ g.name.upper() ~ "_TYPES_H_" -%}
#ifndef {{defname}}
#define {{defname}}

typedef struct {{g.name}}Arguments {
    /* TODO: Add struct members.
     * Use this struct to pass all arguments for
     * graph initialization. This should not contain any
     * pointers (unless you know you'll only be executing
     * in shared memory and thus passing pointers is safe).
     */
} {{g.name}}Args;

#endif /*{{defname}}*/
