import cncframework.hierarchy
import cncframework.lattice
import cncframework.mapping
import sys

graph = cncframework.hierarchy.Graph()

N = graph.add_param("N")

# Step collections
C = graph.add_step_coll(
        name="C",
        tag=["i"],
        dims=["N"],
        inputs=[{'name': "MU", 'tag': ("i", "i", "i")}],
        outputs=[{'name': "MC", 'tag': ("i+1",)}])

T = graph.add_step_coll(
        name="T",
        tag=["i", "r"],
        dims=["N", "N"],
        inputs=[{'name': "MU", 'tag': ("i", "r", "i")},
                {'name': "MC", 'tag': ("i+1",)}],
        outputs=[{'name': "MT", 'tag': ("i+1", "r")}])

U = graph.add_step_coll(name="U",
        tag=["i", "r", "c"],
        dims=["N", "N", "N"],
        inputs=[{'name': "MU", 'tag': ("i", "r", "c")},
                {'name': "MT", 'tag': ("i+1", "r")},
                {'name': "MT", 'tag': ("i+1", "c")}],
        outputs=[{'name': "MU", 'tag': ("i+1", "r", "c")}])

# Item collections
MC = graph.add_item_coll(
        name="MC",
        tag=["i"],
        produced_by=[{'name': "C", 'tag': ("i-1",)}],
        consumed_by=[{'name': "T", 'tag': ("i-1", "r"),
                      'constraints': ["i-1<r", "r<N"]}])

MT = graph.add_item_coll(
        name="MT",
        tag=["i", "r"],
        produced_by=[{'name': "T", 'tag': ("i-1", "r")}],
        consumed_by=[{'name': "U", 'tag': ("i-1", "r", "c"),
                       'constraints': ["i-1<c", "c<=r"]},
                      {'name': "U", 'tag': ("i-1", "p", "r"),
                       'constraints': ["i-1<p", "p<N"]}])

MU = graph.add_item_coll(
        name="MU",
        tag=["i", "r", "c"],
        produced_by=[{'name': "U", 'tag': ("i-1", "r", "c")}],
        consumed_by=[{'name': "C", 'tag': ("r",)},
                      {'name': "T", 'tag': ("c", "r")},
                      {'name': "U", 'tag': ("i", "r", "c"),
                       'constraints': ["i<r", "r<c"]}])

Ctx = cncframework.lattice.LatticeContext(graph,
           # XXX: should compute these using dependence functions
           {"CU": 'illegal', 'i': set(["CTU"])})


Slices = Ctx.find_all_slices()
Full = Ctx.find_all_full_hierarchies()
FullCount = Full.accept(cncframework.lattice.CountingHierarchyVisitor())

def print_full_hierarchies():
    cncframework.lattice.print_full_hierarchies(Full)


####################################

MATRIX_DIM = 128
NODE_COUNT = 8
ALPHA = 0.33
SZ=4

def distC_0(sz=1):  return lambda i: 0
def distC_i(sz=1): return lambda i: i/sz

def distT_0(sz=1):  return lambda i, r: 0
def distT_i(sz=1): return lambda i, r: i//sz
def distT_r(sz=1): return lambda i, r: r//sz

def distU_0(sz=1):  return lambda i, r, c: 0
def distU_i(sz=1): return lambda i, r, c: i//sz
def distU_r(sz=1): return lambda i, r, c: r//sz
def distU_c(sz=1): return lambda i, r, c: c//sz

####################################

def place_items(fn_c, fn_t, fn_u):
    bins = [0] * NODE_COUNT
    def placer(x):
        bins[ x % NODE_COUNT ] += 1
    def messenger(x, y):
        messenger.total_count += 1
        if x == y:
            messenger.local_count += 1
    messenger.local_count = 0
    messenger.total_count = 0
    for i in range(0, MATRIX_DIM):
        placer(fn_c(i))
        messenger(0, fn_u(0, i, i))
        for r in range(i+1, MATRIX_DIM):
            messenger(fn_c(i), fn_t(i, r))
            placer(fn_t(i, r))
            messenger(0, fn_u(0, r, i))
            for c in range(i+1, r+1):
                messenger(fn_t(i, r), fn_u(i, r, c))
                messenger(fn_t(i, r), fn_u(i, c, r))
                placer(fn_u(i, r, c))
                if i == 0:
                    messenger(0, fn_u(i, r, c))
                else:
                    messenger(fn_u(i-1, r, c), fn_u(i, r, c))
    msg_ratio = float(messenger.local_count) / messenger.total_count
    return bins, msg_ratio

def score(fn_c, fn_t, fn_u):
    bins, msg_ratio = place_items(fn_c, fn_t, fn_u)
    # parallelism score
    total_steps = sum(bins)
    max_local_steps = max(bins)
    par_score = total_steps / float(NODE_COUNT * max_local_steps)
    # overall score
    a = par_score
    b =  msg_ratio
    return ALPHA*a + (1-ALPHA)*b, a, b

####################################

# writes DOT files for all the full hierarchies in the graph
# "top" is the result of Ctx.find_all_full_hierarchies()
def dot_full_hierarchies(top, fname_pattern="cfh{}.dot"):
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
        help(0)
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
            for _, n in nodes:
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
    #========
    v = cncframework.lattice.IteratingHierarchyVisitor()
    full_hierarchies = tuple(top.accept(v))
    for i, fh in enumerate(full_hierarchies):
        handle_fh(fh, fname_pattern.format(i+1))
