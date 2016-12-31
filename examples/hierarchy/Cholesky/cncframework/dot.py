import cncframework.lattice

def _output_dotfile(fname, root, leaves, nodes, straight_edges, fork_edges):
    # helpers
    def to_str(xs):
        return "; ".join(map(lambda x: ", ".join(map(str, x)), xs))
    def node_str(n):
        return '{} [label="{}"]'.format(n.safe_str(), n.label_str());
    def _edge_str_help(upper, lower, fmt):
        return '{} -> {} [{}]'.format(lower, upper, fmt)
    def edge_str(upper, lower):
        fmt = 'style=bold,color=blue'
        return _edge_str_help(upper.safe_str(), lower.safe_str(), fmt)
    def fork_strs(upper, lower1, lower2):
        fmt='color=green4'
        s1 = lower1.safe_str()
        s2 = lower2.safe_str()
        join_id = s1 + "__" + s2
        results = []
        results.append('{} [shape=point]'.format(join_id))
        results.append(_edge_str_help(upper.safe_str(), join_id, fmt))
        results.append(_edge_str_help(join_id, s1, fmt))
        results.append(_edge_str_help(join_id, s2, fmt))
        return results
    # get info recursively
    with open(fname, 'w') as out:
        out.write('digraph cholesky {\n')
        out.write('    rankdir=LR; // left-to-right\n')
        out.write('    node [fontname="Linux Biolinum Bold"];\n')
        out.write('    edge [dir=none];\n\n')
        #
        seen = set(leaves)
        seen.add(root)
        # print leaves
        out.write('    subgraph Leaves {\n')
        out.write('        rank=source;\n')
        for n in leaves:
            out.write('        {};\n'.format(node_str(n)))
        out.write('    }\n\n')
        # print root
        out.write('    subgraph Top {\n')
        out.write('        rank=sink;\n')
        out.write('        {};\n'.format(node_str(root)))
        out.write('    }\n\n')
        # print other nodes
        for n in nodes:
            if not (n in seen):
                out.write("    {};\n".format(node_str(n)))
        out.write('\n')
        # print edges
        for a, b in straight_edges:
            out.write("    {};\n".format(edge_str(a, b)))
        out.write('\n')
        # print forks
        for a, b, c in fork_edges:
            for x in fork_strs(a, b, c):
                out.write("    {};\n".format(x))
            out.write('\n')
        # end of file
        out.write('}\n')

# writes DOT files for all the full hierarchies in the graph
# "top" is the result of Ctx.find_all_full_hierarchies()
def full_hierarchies(top, fname_pattern=None):
    if not fname_pattern:
        fname_pattern = "fh-{}.dot"
    def handle_fh(nodes, fname):
        _, root = nodes[0]
        leaves = []
        straight_edges = []
        fork_edges = []
        # helpers
        def get_node(i):
            if i < len(nodes):
                return nodes[i]
            else:
                return -1, None
        def help(i):
            depth, node = get_node(i)
            if not node: return
            child_depth, child1 = get_node(i+1)
            if child_depth != depth+1:
                leaves.append(node)
                return i
            # process child node
            i = help(i+1)
            next_depth, child2 = get_node(i+1)
            # pair decomp (two children)
            if next_depth == child_depth:
                i = help(i+1)
                fork_edges.append((node, child1, child2))
            # tag decomp (one child)
            else:
                straight_edges.append((node, child1))
            return i
        # get info recursively
        help(0)
        _output_dotfile(fname, root, leaves,
                tuple(n for _, n in nodes),
                straight_edges, fork_edges)
    # create a DOT file for each full hierarchy
    v = cncframework.lattice.IteratingHierarchyVisitor()
    full_hierarchies = tuple(top.accept(v))
    for i, fh in enumerate(full_hierarchies):
        handle_fh(fh, fname_pattern.format(i+1))

# writes DOT file for the hierarchy space of the graph
# "ctx" is the LatticeContext of the input graph
def hierarchy_space(ctx, fname=None):
    if not fname:
        fname = "hspace.dot"
    root = ctx.root
    nodes = tuple(ctx.nodes.values())
    leaves = []
    straight_edges = []
    fork_edges = []
    seen = set()
    def help(node):
        assert(node)
        if node.is_leaf():
            if not (node in seen):
                seen.add(node)
                leaves.append(node)
        else:
            for x in node.pairwise_decompositions:
                if not (x in seen):
                    seen.add(x)
                    fork_edges.append((node,) + x.children)
                    map(help, x.children)
            for x in node.tag_decompositions:
                if not (x in seen):
                    seen.add(x)
                    straight_edges.append((node, x.child))
                    help(x.child)
    help(root)
    _output_dotfile(fname, root, leaves, nodes, straight_edges, fork_edges)
