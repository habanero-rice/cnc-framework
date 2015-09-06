{% import "common_macros.inc.c" as util with context -%}
#include "{{g.name}}.h"

int cncMain(int argc, char *argv[]) {

    // Create a new graph context
    {{util.g_ctx_t()}} *context = {{g.name}}_create();

    // TODO: Set up arguments for new graph initialization
    // Note that you should define the members of
    // this struct by editing {{g.name}}_defs.h.
    {{util.g_args_t()}} *args = NULL;

    {% if g.ctxParams %}
    // TODO: initialize graph context parameters
    {% for line in g.ctxParams -%}
    // {{ line }}
    {% endfor -%}
    {% endif %}
    // Launch the graph for execution
    {{g.name}}_launch(args, context);

    // Exit when the graph execution completes
    CNC_SHUTDOWN_ON_FINISH(context);

    return 0;
}
