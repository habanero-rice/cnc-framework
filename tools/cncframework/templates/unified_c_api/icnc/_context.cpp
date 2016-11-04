{% import "common_macros.inc.c" as util with context -%}
{{ util.auto_file_banner() }}

#include "icnc_internal.h"
// Include the C interface
extern "C" {
#include "{{g.name}}_internal.h"
}

{% if g.hasCustomDist() -%}
static inline int numProcs() { return CnC::tuner_base::numProcs(); }
{% for name, i in g.itemDeclarations.items() -%}
static inline cncLocation_t _cncItemDistFn_{{name}}({{ util.print_tag(i.key, typed=True) ~ util.g_ctx_param()}}) { return {{g.itemDistFn(name, "numProcs()")}}; }
{% endfor -%}
{% endif %}
CNC_BITWISE_SERIALIZABLE({{util.g_ctx_t()}});

namespace {{g.name}} {

    {# /* TODO - Should move these XXX declarations to a common iCnC file */ #}
    // XXX - Unified CnC doesn't have separate tag collections, so we need a hybrid
    template< typename UserStep, typename StepTuner = CnC::step_tuner<> >
    struct CnCTagStepColl: public CnC::tag_collection<cncAggregateTag_t>, public CnC::step_collection<UserStep, StepTuner> {
        template <typename Ctx> CnCTagStepColl(Ctx &ctx, std::string &name):
            CnC::tag_collection<cncAggregateTag_t>(ctx, name), CnC::step_collection<UserStep, StepTuner>(ctx, name)
        {
            this->prescribes(*this, ctx);
        }
    };

    {% set gCppCtx = g.name ~ "CppCtx" -%}
    struct {{gCppCtx}};

    {% if g.hasCustomDist() -%}
    {% for name, i in g.itemDeclarations.items() -%}
    struct cncItemTuner_{{name}}: public CnC::hashmap_tuner {
        {{gCppCtx}} &_cppCtx;
        cncItemTuner_{{name}}({{gCppCtx}} &ctx): _cppCtx(ctx) { }
        int consumed_on(const cncAggregateTag_t &_tag);
        int produced_on(const cncAggregateTag_t &_tag);
    };
    {% endfor %}
    {% endif -%}
    {% for stepfun in g.finalAndSteps -%}
    // STEP {{stepfun.collName}}
    struct cncStepTuner_{{stepfun.collName}}: public CnC::step_tuner<> {
        {% if g.hasCustomDist() -%}
        int compute_on(const cncAggregateTag_t &_tag, {{gCppCtx}} &_cppCtx) const;
        {%- endif %}
        {% if g.hasTuning('priority') -%}
        int priority(const cncAggregateTag_t &_tag, {{gCppCtx}} &_cppCtx) const;
        {%- endif %}
    };
    struct cncStepImpl_{{stepfun.collName}} {
        int execute(const cncAggregateTag_t &_tag, {{gCppCtx}} &ctx) const;
    };
    std::string sname_{{stepfun.collName}} = std::string("({{stepfun.collName}})");

    {% endfor %}
    struct {{gCppCtx}}: public CnC::context<{{gCppCtx}}> {

        // Item collections
        // FIXME - singleton collections should use a vector backing
        // (probably need to update the serializer to set size to 1)
        // https://github.com/icnc/icnc/blob/v1.0.100/samples/blackscholes/blackscholes/blackscholes.h#L105-L113
        {%- for name, i in g.itemDeclarations.items() %}
        {% if g.hasCustomDist() -%}
        cncItemTuner_{{name}} ituner_{{name}};
        {% endif -%}
        CnC::item_collection<cncAggregateTag_t, cncBoxedItem_t*> i_{{name}};
        {% endfor %}

        // Step collections
        {% for stepfun in g.finalAndSteps -%}
        CnCTagStepColl< cncStepImpl_{{stepfun.collName}}, cncStepTuner_{{stepfun.collName}} > s_{{stepfun.collName}};
        {% endfor %}

        // Configuration
        {{util.g_ctx_t()}} cctx;

        // Context constructor
        {{gCppCtx}}()
            : CnC::context<{{gCppCtx}}>(),
              // init item colls
              {%- for name, i in g.itemDeclarations.items() %}
              {% if g.hasCustomDist() -%}
              ituner_{{name}}(*this),
              {% endif -%}
              i_{{name}}(*this, "[{{name}}]"
                      {%- if g.hasCustomDist() -%}
                      , ituner_{{name}}
                      {%- endif -%}
                      ),
              {% endfor %}
              // init step colls
              {% for stepfun in g.finalAndSteps -%}
              s_{{stepfun.collName}}(*this, sname_{{
                      stepfun.collName}}){{ "" if loop.last else "," }}
              {% endfor %}
        {
            // Set context pointer
            cctx._externCtxOffset = (size_t)(((char*)&cctx)-((char*)this));

            // producer/consumer relations
            {% for stepfun in g.finalAndSteps -%}
            {% for x in stepfun.inputItemColls() -%}
            s_{{stepfun.collName}}.consumes(i_{{x}});
            {% endfor %}
            {% for x in stepfun.outputItemColls() -%}
            s_{{stepfun.collName}}.produces(i_{{x}});
            {% endfor %}
            {%- endfor %}

            // Debug options
            //CnC::debug::collect_scheduler_statistics(*this);
            //CnC::debug::trace_all( *this );
        }

        #ifdef DIST_CNC
        void serialize(CnC::serializer &ser) {
            ser & cctx;
        }
        #endif /* DIST_CNC */

    };
}

extern "C" {
    static inline {{g.name~"::"~gCppCtx}} *_cncCppCtx({{util.g_ctx_param()}}) {
        return ({{g.name~"::"~gCppCtx}}*)(((char*){{util.g_ctx_var()}}) - ctx->_externCtxOffset);
    }

    {% for stepfun in g.finalAndSteps -%}
    /* {{stepfun.collName}} task creation */
    void cncPrescribe_{{stepfun.collName}}({{
            util.print_tag(stepfun.tag, typed=True)
            }}{{util.g_ctx_param()}}) {
        {% if stepfun.isSingleton -%}
        cncAggregateTag_t &_tag = _cncSingletonTag;
        {% else %}
        cncTag_t _init[] = { {{ stepfun.tag|join(", ") }} };
        cncAggregateTag_t _tag(_init, {{stepfun.tag|count}});
        {% endif %}
        _cncCppCtx({{util.g_ctx_var()}})->s_{{stepfun.collName}}.put(_tag);
        {{ util.log_msg("PRESCRIBED", stepfun.collName, stepfun.tag, indent=2) }}
    }
    {% endfor %}

    {% for i in g.itemDeclarations.values() %}
    void cncPut_{{i.collName}}({{i.type.ptrType}}_item, {{
            util.print_tag(i.key, typed=True) ~ util.g_ctx_param() }}) {
        {% if not i.isVirtual -%}
        {#/*****NON-VIRTUAL*****/-#}
        {{ util.log_msg("PUT", i.collName, i.key, indent=2) }}
        {% if i.key -%}
        cncTag_t _init[] = { {{i.key|join(", ")}} };
        cncAggregateTag_t _tag(_init, {{i.key|count}});
        {%- else -%}
        cncAggregateTag_t &_tag = _cncSingletonTag;
        {%- endif %}
        _cncCppCtx({{util.g_ctx_var()}})->i_{{i.collName}}.put(_tag, _cncItemRebox((void*)_item));
        {%- else -%}
        {% set targetColl = g.itemDeclarations[i.mapTarget] -%}
        {% if i.isInline -%}
        {#/*****INLINE VIRTUAL*****/-#}
        cncPut_{{i.mapTarget}}(_item, {{
                util.print_tag(i.keyFunction) ~ util.g_ctx_var()}});
        {%- else -%}
        {#/*****EXTERN VIRTUAL******/-#}
        {{i.mapTarget}}ItemKey _key = {{i.functionName}}({{
                util.print_tag(i.key) }}{{util.g_ctx_var()}});
        cncPut_{{i.mapTarget}}(_item, {{
                util.print_tag(targetColl.key, prefix="_key.") ~ util.g_ctx_var()}});
        {%- endif %}
        {%- endif %}
    }

    {{i.type.ptrType}}cncGet_{{i.collName}}({{ util.print_tag(i.key, typed=True) ~ util.g_ctx_param()}}) {
        {% if not i.isVirtual -%}
        {#/*****NON-VIRTUAL*****/-#}
        {{ util.log_msg("GET-DEP", i.collName, i.key, indent=2) }}
        {% if i.key -%}
        cncTag_t _init[] = { {{i.key|join(", ")}} };
        cncAggregateTag_t _tag(_init, {{i.key|count}});
        {%- else -%}
        cncAggregateTag_t &_tag = _cncSingletonTag;
        {%- endif %}
        cncBoxedItem_t *_box;
        _cncCppCtx({{util.g_ctx_var()}})->i_{{i.collName}}.get(_tag, _box);
        {{ i.type.ptrType }}_item = ({{ i.type.ptrType }})_cncItemUnbox(_box);
        return _item;
        {%- else -%}
        {% set targetColl = g.itemDeclarations[i.mapTarget] -%}
        {% if i.isInline -%}
        {#/*****INLINE VIRTUAL*****/-#}
        return cncGet_{{i.mapTarget}}({{util.print_tag(i.keyFunction) ~ util.g_ctx_var()}});
        {%- else -%}
        {#/*****EXTERN VIRTUAL******/-#}
        {{i.mapTarget}}ItemKey _key = {{i.functionName}}({{
                util.print_tag(i.key) }}{{util.g_ctx_var()}});
        return cncGet_{{i.mapTarget}}({{util.print_tag(targetColl.key, prefix="_key.") ~ util.g_ctx_var()}});
        {%- endif %}
        {%- endif %}
    }
    {% endfor %}

    {{util.g_ctx_t()}} *{{g.name}}_create(void) {
        {{g.name~"::"~gCppCtx}} *cppCtx = new {{g.name~"::"~gCppCtx}}();
        return &cppCtx->cctx;
    }

    void {{g.name}}_destroy({{util.g_ctx_param()}}) {
        delete _cncCppCtx({{util.g_ctx_var()}});
    }

    void {{g.name}}_launch({{util.g_args_param()}}, {{util.g_ctx_param()}}) {
        {{util.qualified_step_name(g.initFunction)}}({{util.g_args_var()}}, {{util.g_ctx_var()}});
        _cncCppCtx({{util.g_ctx_var()}})->wait();
    }

    void {{g.name}}_await({{ util.print_tag(g.finalizeFunction.tag, typed=True) ~ util.g_ctx_param()}}) {
        cncPrescribe_{{g.finalizeFunction.collName}}({{
                util.print_tag(g.finalizeFunction.tag) ~ util.g_ctx_var()}});
    }
}


namespace {{g.name}} {
    {% if g.hasCustomDist() -%}
    {% for name, i in g.itemDeclarations.items() -%}
    {% for f in ["produced_on", "consumed_on"] -%}
    int cncItemTuner_{{name}}::{{f}}(const cncAggregateTag_t &_tag) {
        const {{util.g_ctx_param()}} = &_cppCtx.cctx;
        {% for x in i.key -%}
        const cncTag_t {{x}} = _tag[{{loop.index0}}];
        {% endfor -%}
        return {{g.itemDistFn(name, "numProcs()")}};
    }
    {% endfor -%}
    {% endfor %}
    {% endif -%}
    {% for stepfun in g.finalAndSteps -%}
    // STEP {{stepfun.collName}}
    {% if g.hasCustomDist() -%}
    int cncStepTuner_{{stepfun.collName}}::compute_on(const cncAggregateTag_t &_tag, {{gCppCtx}} &_cppCtx) const {
        {{util.g_ctx_param()}} = &_cppCtx.cctx; MAYBE_UNUSED({{util.g_ctx_var()}});
        {% for x in stepfun.tag -%}
        const cncTag_t {{x}} = _tag[{{loop.index0}}];
        {% endfor -%}
        return {{g.stepDistFn(stepfun.collName, "numProcs()")}};
    }
    {% endif -%}
    {% if g.hasTuning('priority') -%}
    int cncStepTuner_{{stepfun.collName}}::priority(const cncAggregateTag_t &_tag, {{gCppCtx}} &_cppCtx) const {
        {{util.g_ctx_param()}} = &_cppCtx.cctx; MAYBE_UNUSED({{util.g_ctx_var()}});
        {% for x in stepfun.tag -%}
        const cncTag_t {{x}} = _tag[{{loop.index0}}];
        {% endfor -%}
        return {{g.priorityFn(stepfun.collName, "numProcs()")}};
    }
    {%- endif %}
    int cncStepImpl_{{stepfun.collName}}::execute(const cncAggregateTag_t &_tag, {{gCppCtx}} &ctx) const {
        _{{g.name}}_cncStep_{{stepfun.collName}}({% for x in stepfun.tag -%}_tag[{{loop.index0}}], {% endfor -%}&ctx.cctx);
        return CnC::CNC_Success;
    }
    {% endfor %}
}

// FIXME - should be declared in icnc.c, but can't do that
// because dist_cnc_init needs the graph context struct
extern "C" int cncMain(int, char **);
int main(int argc, char **argv) {
    #ifdef DIST_CNC
    CnC::dist_cnc_init< {{g.name~"::"~gCppCtx}} > dist;
    #endif /* DIST_CNC */
    return cncMain(argc, argv);
}
