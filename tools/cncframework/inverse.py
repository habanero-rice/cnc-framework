from itertools import chain

from sympy import Symbol, solve, Piecewise
from sympy.core import sympify

import cncframework.events.actions as actions

def tag_expr(tag, out_var):
    """Return out_var = tag as a SymPy expression."""
    # since sympify will automatically equate to zero, we convert it to:
    # tag_expr - o_n, and solve for s, some variable in the tagspace
    return sympify(str.format("{} - {}", tag.expr, out_var))


def piecewise_tag_expr(tag, out_var, condition):
    """Return out_var = tag as a piecewise function defined where condition
    evaluates to True."""
    expr = tag_expr(tag, out_var)
    # replace '#' and '@' from condition
    condition = condition.replace('@', 'arg').replace('#', 'ctx')
    cond = sympify(condition)
    return Piecewise((expr, cond))


def find_collNames(output_list):
    """
    Return list of collection names collected from refs in output_list.
    """
    colls = []
    for out in output_list:
        if out.kind in {"STEP", "ITEM"}:
            colls.append(out.collName)
        elif out.kind == "IF":
            colls.append(out.refs[0].collName)
    return colls


def find_step_inverses(stepFunction):
    """
    Given a StepFunction, read the expressions for each output and return a map
    {c: [f: tagspace -> t for each output tag t] for each output collection or
    step c} where the tagspace is enumerated (t1,t2,...,tn).
    """
    tag_space = [Symbol(t) for t in stepFunction.tag]
    outputs = {coll: [] for coll in find_collNames(stepFunction.outputs)}

    def solve_for(tag, tag_space, out_var, cond=None):
        expr = (piecewise_tag_expr(tag, out_var, cond) if cond else
                tag_expr(tag, out_var))
        solution = solve(expr, tag_space, dict=True)
        return solution[0] if solution else {}

    for output in stepFunction.outputs:
        if output.kind in {"STEP", "ITEM"}:
            tag_list = output.key if output.kind == "ITEM" else output.tag
            outputs[output.collName].extend(
                solve_for(t, tag_space, "t{}".format(i + 1))
                for (i, t) in enumerate(t for t in tag_list if not t.isRanged))
        elif output.kind == "IF":
            out_ref = output.refs[0]
            tag_list = out_ref.key if out_ref.kind == "ITEM" else out_ref.tag
            outputs[out_ref.collName].extend(
                solve_for(t, tag_space, "t{}".format(i + 1), output.rawCond)
                for (i, t) in enumerate(t for t in tag_list if not t.isRanged))
    return outputs


def find_blame_candidates(arg_blame, graph_data):
    """
    Given arg_blame in format coll@tag and graph_data from specfile, find the
    possible steps@tag that could be responsible for putting or prescribing
    arg_blame.
    """
    coll_name, coll_tag = arg_blame.split("@")
    # turn coll_tag into a tuple representing a point in tagspace
    coll_tag = tuple(coll_tag.split(","))
    # turn coll_tag into dict of substitutions tk: coll_tag[k]
    coll_tag_system = {
        Symbol("t{}".format(i + 1)): v for i, v in enumerate(coll_tag)}
    # {s: {in_tag: value for each input tag of s} for each step s}
    candidates = {}
    # steps that contain the collection in output but have no valid solution
    rejected_steps = set()
    for (step, func) in graph_data.stepFunctions.iteritems():
        func_inverses = find_step_inverses(func)
        if coll_name in func_inverses:
            candidates[step] = {}
            for out_tag in func_inverses[coll_name]:
                for (in_tag, expr) in out_tag.iteritems():
                    in_tag = str(in_tag)
                    # evaluate inv_p(t)
                    inv = expr.subs(coll_tag_system)
                    if in_tag in candidates[step]:
                        if inv != candidates[step][in_tag]:
                            # then the solution is inconsistent, reject
                            rejected_steps.add(step)
                    else:
                        candidates[step][in_tag] = inv
    for s in rejected_steps:
        del candidates[s]
    return candidates


def _node_to_name(node, event_graph):
    """Create a name string for a given node in the event graph.
    """

    return "{}@{}".format(event_graph.property(node, "name", ""),
            event_graph.property(node, "tag", ""))


def blame_deadlocks(graph_ast, event_graph):
    """Blame candidates for deadlock given the execution graph of a program by
    attempting to remove all steps that depend on blocked steps.
    """

    step_functions = graph_ast.stepFunctions
    potentially_deadlocked = event_graph.gotten_without_put()
    # Map step/item@tag to node in graph
    tags_to_nodes = {_node_to_name(node, event_graph):node for node in event_graph}
    blame_candidateses = [find_blame_candidates(_node_to_name(blame_node,
        event_graph), graph_ast) for blame_node in potentially_deadlocked]
    # Fill in all blamed nodes as having "run" (by adding the node if not
    # present and add put/prescribe edges).
    for step_name, tags in chain.from_iterable(map(dict.iteritems, blame_candidateses)):
        # Put the tag tuple in canonical order (same as in spec file)
        tag_tuple = tuple([int(tags[tag]) for tag in step_functions[step_name].tag])
        tag_tuple_string = ", ".join(map(str, tag_tuple))
        step_tag_label = "{}@{}".format(step_name, tag_tuple_string)
        if step_tag_label in tags_to_nodes:
            step_tag_id = tags_to_nodes[step_tag_label]
        else:
            step_tag_id = event_graph.create_node_id(actions.RUNNING, step_name, tag_tuple_string)
            tags_to_nodes[step_tag_label] = step_tag_id
            event_graph.add_node(step_tag_id)
            event_graph.style_step(step_tag_id, step_name, tag_tuple_string)
        for output in step_functions[step_name].outputs:
            if output.kind in {"STEP", "ITEM"}:
                tag_list = output.key if output.kind == "ITEM" else output.tag
                coll = output.collName
            elif output.kind == "IF":
                out_ref = output.refs[0]
                tag_list = out_ref.key if out_ref.kind == "ITEM" else out_ref.tag
                coll = out_ref.collName

            # Substitute, and add edge in graph
            output_node = "{}@{}".format(coll, ", ".join(
                [str(sympify(t.expr).subs(tags)) for t in tag_list]))
            if output_node in tags_to_nodes:
                event_graph.add_node_with_children(step_tag_id, [tags_to_nodes[output_node]])
    # Now we re-traverse the graph and only blame step nodes with indegree = 0.
    filtered = {}
    for blame_node, blame_candidate in zip(potentially_deadlocked, blame_candidateses):
        for step_name, tags in blame_candidate.iteritems():
            tag_tuple = tuple([int(tags[tag]) for tag in step_functions[step_name].tag])
            tag_tuple_string = ", ".join(map(str, tag_tuple))
            step_tag_label = "{}@{}".format(step_name, tag_tuple_string)
            if event_graph.in_degree(tags_to_nodes[step_tag_label]) == 0:
                filtered.setdefault(_node_to_name(blame_node, event_graph), {})[step_name] = tags
    return filtered

