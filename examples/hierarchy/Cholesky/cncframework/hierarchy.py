import sympy

_missing = RuntimeError("Missing Property")

class lazy_property(property):

    """A decorator that converts a function into a lazy property.  The
    function wrapped is called the first time to retrieve the result
    and then that calculated result is used the next time you access
    the value::
        class Foo(object):
            @lazy_property
            def foo(self):
                # calculate something important here
                return 42
    The class has to have a `__dict__` in order for this property to
    work.

    From https://github.com/pallets/werkzeug/blob/master/werkzeug/utils.py
    """

    # implementation detail: A subclass of python's builtin property
    # decorator, we override __get__ to check for a cached value. If one
    # choses to invoke __get__ by hand the property will still work as
    # expected because the lookup logic is replicated in __get__ for
    # manual invocation.

    def __init__(self, func, name=None, doc=None):
        self.__name__ = name or func.__name__
        self.__module__ = func.__module__
        self.__doc__ = doc or func.__doc__
        self.func = func

    def __set__(self, obj, value):
        obj.__dict__[self.__name__] = value

    def __get__(self, obj, type=None):
        if obj is None:
            return self
        value = obj.__dict__.get(self.__name__, _missing)
        if value is _missing:
            value = self.func(obj)
            obj.__dict__[self.__name__] = value
        return value

def _instantiate_var(v):
    """
    Create a fresh SymPy Symbol for the provided variable name.
    An internal global counter is used to ensure uniqueness.
    """
    i = _instantiate_var.counter
    _instantiate_var.counter += 1
    return sympy.Symbol("{}__{:x}".format(v, i))
_instantiate_var.counter = 0

def _parse_expr(expr, local_vars):
    """
    Parse a tag function expression.
    """
    return sympy.sympify(expr, local_vars)

class GraphException(Exception):
    pass

def Constant(n, deps=None):
    return _parse_expr(n, deps if deps else {})

class _Base(object):
    """
    Base class that ignores super arguments.
    This is meant to serve as the root of multiple-inheritence hierarchies,
    avoiding problems with __init__ argument mismatches.
    """
    def __init__(self, *args, **kwargs):
        pass

class CnCObject(_Base):
    def __init__(self, **kwargs):
        super(CnCObject, self).__init__(**kwargs)
        self.init_args = kwargs
        self.name = kwargs['name']
        self.tag = tuple(sympy.Symbol(str(t)) for t in kwargs['tag'])
        self.graph = kwargs.get('graph')
    def str_tag(self):
        return map(str, self.tag)
    def __str__(self):
        tag = ", ".join(self.str_tag())
        lhs = self.brackets[0]
        rhs = self.brackets[1]
        return "{}{}: {}{}".format(lhs, self.name, tag, rhs)
    def __repr__(self):
        return "<{}>{}".format(type(self).__name__, str(self))

class StepBrackets(_Base):
    def __init__(self, **kwargs):
        super(StepBrackets, self).__init__(**kwargs)
        self.brackets = "()"

class ItemBrackets(_Base):
    def __init__(self, **kwargs):
        super(ItemBrackets, self).__init__(**kwargs)
        self.brackets = "[]"

#
# Reified collections
#

class ReifiedColl(CnCObject):
    def __init__(self, coll):
        super(ReifiedColl, self).__init__(**coll.init_args)
        self.coll = coll
        self.reified_tag = tuple(_instantiate_var(v) for v in self.tag)
    def str_tag(self):
        return map(str, self.reified_tag)
    def _instantiate_relations(self, rels):
        updated_rels = []
        substitutions = zip(self.tag, self.reified_tag)
        context = { str(t): t for t in self.reified_tag }
        for r in rels:
            tag = []
            for t in r.instance_tag:
                for k,v in substitutions:
                    t = t.subs(k,v)
                tag.append(t)
            rel_instance = r.coll.instantiate(tag, context)
            updated_rels.append(rel_instance)
        return updated_rels

class ReifiedStepColl(ReifiedColl, StepBrackets):
    def __init__(self, coll):
        super(ReifiedStepColl, self).__init__(coll)

class ReifiedItemColl(ReifiedColl, ItemBrackets):
    def __init__(self, coll):
        super(ReifiedItemColl, self).__init__(coll)
        self.producers = self._instantiate_relations(coll.produced_by)
        self.consumers = self._instantiate_relations(coll.consumed_by)
    def __str__(self):
        tag = map(str, self.reified_tag)
        return "[{}: {}]".format(self.name, ", ".join(self.str_tag()))

#
# Collection instances
#

class Instance(CnCObject):
    def __init__(self, coll, tag, context=None):
        super(Instance, self).__init__(name=coll.name, tag=coll.tag)
        self.coll = coll
        self.context = context
        local_vars = dict(context) if context else {}
        local_vars.update(coll.graph.params)
        self.instance_tag = tuple(_parse_expr(t, local_vars) for t in tag)
        local_var_set = set(local_vars.values())
        for t in self.instance_tag:
            for v in t.free_symbols:
                if not v in local_var_set:
                    #raise GraphException("ERROR: free variable {} in {}".format(v, self))
                    print "ERROR: free variable {} in {}".format(v, self)
    def str_tag(self):
        return map(str, self.instance_tag)
    def _instantiate_relations(self, rels):
        updated_rels = []
        substitutions = zip(self.tag, self.instance_tag)
        for r in rels:
            tag = []
            for t in r.instance_tag:
                for k,v in substitutions:
                    t = t.subs(k,v)
                tag.append(t)
            # NOTE: I don't think we need a context here
            # (the graph parameters should be sufficient at this point)
            factory = r.coll
            updated_rels.append(factory(*tag))
        return updated_rels

class ItemInstance(Instance, ItemBrackets):
    def __init__(self, coll, tag, context):
        super(ItemInstance, self).__init__(coll, tag, context)
    @lazy_property
    def producers(self):
        return self._instantiate_relations(self.coll.produced_by)
    @lazy_property
    def consumers(self):
        return self._instantiate_relations(self.coll.consumed_by)

class StepInstance(Instance, StepBrackets):
    def __init__(self, coll, tag, context):
        super(StepInstance, self).__init__(coll, tag, context)
    @lazy_property
    def inputs(self):
        return self._instantiate_relations(self.coll.inputs)
    @lazy_property
    def outputs(self):
        return self._instantiate_relations(self.coll.outputs)

#
# Collections (abstract)
#

class Coll(CnCObject):
    def __init__(self, **kwargs):
        super(Coll, self).__init__(**kwargs)
    def __call__(self, *instance_tag):
        return self.instantiate(instance_tag)
    def instantiate(self, instance_tag, context=None):
        t = tuple(instance_tag)
        assert len(t) == len(self.tag), "Tag length mismatch: {} vs {}".format(t, self)
        factory = self.instance_factory()
        return factory(self, t, context)
    def _parse_relations(self, rels, mapping):
        parsed_rels = []
        context = { str(t): t for t in self.tag }
        for r in rels:
            # TODO: handle conditions
            # TODO: handle free variables that are defined by conditions
            x = mapping[r['name']]
            parsed_rels.append(x.instantiate(r['tag'], context))
        return parsed_rels

class StepColl(Coll, StepBrackets):
    def __init__(self, **kwargs):
        super(StepColl, self).__init__(**kwargs)
    def instance_factory(self):
        return StepInstance
    def reify(self):
        return ReifiedStepColl(self)
    @lazy_property
    def inputs(self):
        return self._parse_relations(self.init_args['inputs'], self.graph.item_colls)
    @lazy_property
    def outputs(self):
        return self._parse_relations(self.init_args['outputs'], self.graph.item_colls)

class ItemColl(Coll, ItemBrackets):
    def __init__(self, **kwargs):
        super(ItemColl, self).__init__(**kwargs)
    def instance_factory(self):
        return ItemInstance
    def reify(self):
        return ReifiedItemColl(self)
    @lazy_property
    def produced_by(self):
        return self._parse_relations(self.init_args['produced_by'], self.graph.step_colls)
    @lazy_property
    def consumed_by(self):
        return self._parse_relations(self.init_args['consumed_by'], self.graph.step_colls)

#
# CnC Graph
#

class Graph(object):
    def __init__(self):
        self.params = {}
        self.step_colls = {}
        self.item_colls = {}
    def add_param(self, v):
        p = sympy.Symbol(v)
        self.params[v] = p
        return p
    def add_item_coll(self, **kwargs):
        i = ItemColl(graph=self, **kwargs)
        self.item_colls[i.name] = i
        return i
    def add_step_coll(self, **kwargs):
        s = StepColl(graph=self, **kwargs)
        self.step_colls[s.name] = s
        return s
