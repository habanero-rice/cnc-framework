from collections import deque

class DAG(object):
    """Directed Acyclic Graph"""
    def __init__(self, nodes = None):
        """
        Init the graph.

        If nodes is given, it should be a mapping {node_id: {children}}
        """
        self._nodes = nodes if nodes else {}
        self._properties = {n: {} for n in nodes} if nodes else {}
        self._eproperties = {}

    def __contains__(self, arg):
        """Test existence of a node in the graph."""
        return arg in self._nodes

    def __len__(self):
        """Return the number of nodes in the graph."""
        return len(self._nodes)

    def __iter__(self):
        """Return an iterator for all the nodes in the graph."""
        return self._nodes.__iter__()

    def __eq__(self, other):
        """Equality between graphs, True if self is likely isomorphic to other. """
        if len(self) != len(other):
            return False
        selfsort = self.topsort()
        othersort = other.topsort()
        # make sure each layer is the same size
        selflayers = self.dist_layers(selfsort[0])
        otherlayers = other.dist_layers(othersort[0])
        if len(selflayers) != len(otherlayers):
            return False
        for layer in selflayers:
            if layer not in otherlayers or len(selflayers[layer]) != len(otherlayers[layer]):
                return False
        # same number of leaves
        if len(self.collect_leaves(selfsort[0])) != len(other.collect_leaves(othersort[0])):
            return False
        # same critical path length
        if self.critical_path_length() != other.critical_path_length():
            return False
        return True

    def __str__(self):
        """Return string representation of DAG."""
        return str(self._nodes)

    def transpose(self):
        """Return the transpose (reversed edges) of the graph as a new DAG."""
        p = {n: set() for n in self}
        for n in self._nodes:
            for c in self.children(n):
                p[c].add(n)
        return DAG(p)

    def in_degree(self, node):
        """The number of parents of some node."""
        return len(self.parents(node))

    def out_degree(self, node):
        """The number of children of some node."""
        return len(self.children(node))

    def set_edge_color(self, fr, to, val):
        """Set the color of the edge from fr to to."""
        self.set_edge_property(fr, to, 'color', val)

    def edge_color(self, fr, to, default = None):
        """Return the color of the edge from fr to to, default if undefined."""
        return self.edge_property(fr, to, 'color', default)

    def set_edge_label(self, fr, to, val):
        """Set the label of the edge from fr to to."""
        self.set_edge_property(fr, to, 'label', val)

    def edge_label(self, fr, to, default = None):
        """Return the label of the edge from fr to to, default if undefined."""
        return self.edge_property(fr, to, 'label', default)

    def set_edge_property(self, fr, to, prop, val):
        """Set property prop of edge from fr to to."""
        if (fr,to) not in self._eproperties:
            self._eproperties[(fr,to)] = {}
        self._eproperties[(fr,to)][prop] = val

    def set_edge_properties(self, fr, to, propertyDict):
        """Set properties from fr to to to the given propertyDict."""
        self._eproperties[(fr,to)] = propertyDict.copy()

    def edge_property(self, fr, to, prop, default=None):
        """Return property prop of edge from fr to to."""
        return self._eproperties[(fr,to)].get(prop, default)

    def edge_properties(self, fr, to):
        """Return a reference to a dict of the properties of the edge from fr to to."""
        return self._eproperties.get((fr,to), {})

    def set_property(self, node, prop, value):
        """Set some property of a node to given value."""
        self._properties[node][prop] = value

    def has_property(self, node, prop):
        """Return whether a node has some property defined."""
        return prop in self._properties[node]

    def property(self, node, prop, default = None):
        """Get property prop of some node, default if prop not defined for node."""
        if prop not in self._properties[node] or not self._properties[node][prop]:
            return default
        return self._properties[node][prop]

    def properties(self, node):
        """Return a reference to a dict of all the properties of a node."""
        return self._properties[node]

    def remove_node(self, node, parents = None):
        """Remove a node from the graph."""
        if not parents:
            parents = self.parents(node)
        for parent in self.parents(node):
            self.remove_child(parent, node)

        del self._nodes[node]
        del self._properties[node]

    def contract(self, node, parent):
        """Contract parent node into node, labeling and coloring contracted edges."""
        for p in self.parents(parent):
            # parents of parent become parents of node
            color = self.edge_color(p, parent, 'black')
            label = self.edge_label(p, parent, '')
            self.add_child(p, node)
            self.set_edge_label(p, node, label)
            self.set_edge_color(p, node, color)
        for c in self.children(parent):
            if c == node: continue
            # children of parent become children of node
            color = self.edge_color(parent, c, 'black')
            label = self.edge_label(parent, c, '')
            self.add_child(node, c)
            self.set_edge_label(node, c, label)
            self.set_edge_color(node, c, color)
        self.remove_node(parent)

    def add_node(self, node):
        """Add node to graph if it's not already there."""
        if node not in self:
            self._nodes[node] = set()
            self._properties[node] = {}

    def add_node_with_children(self, node, children):
        """
        Add node to the graph with given iterable of children.

        Adds children to the graph if they are not already there.
        """
        self.add_node(node)
        for child in children:
            if child not in self:
                self.add_node(child)
            self.add_child(node, child)

    def add_node_with_parents(self, node, parents):
        """
        Add node to the graph with given iterable of parents.

        Adds parents to the graph if they are not already there.
        """
        self.add_node(node)
        for parent in parents:
            if parent not in self:
                self.add_node(parent)
            self.add_child(parent, node)

    def add_child(self, node, child):
        """Add some child to some node; create child node if it does not exist."""
        if child in self:
            self._nodes[node].add(child)
            if (node,child) not in self._eproperties:
                self._eproperties[(node,child)] = {}
        else:
            self.add_node_with_parents(child, [node])

    def add_parent(self, node, parent):
        """Add some parent to some node; create parent if it does not exist."""
        self.add_child(parent, node)

    def remove_child(self, node, child):
        """Remove child from set of children of node."""
        self._nodes[node].discard(child)

    def remove_all_children(self, node):
        """Remove all children of node."""
        self._nodes[node] = set()

    def remove_all_parents(self, node):
        """Remove all parents of node."""
        for parent in self.parents(node):
            self.remove_child(parent, node)

    def children(self, node):
        """Return the set of children of some node."""
        return self._nodes[node]

    def parents(self, node):
        """
        Return the set of parents of some node.

        Use transpose and children instead if parents is called repeatedly.
        """
        parents = set()
        for p in self:
            if node in self.children(p):
                parents.add(p)
        return parents

    def bfs(self, start_node = None, visitor = lambda x: x):
        """
        Perform a breadth-first search starting at start_node along parent -> children edges.

        Call visitor(node) on each visited node in traversal order.
        If start_node not given, visit all nodes.
        Return a mapping of {visited nodes : distance from start}
        """
        distances = {} # track visited nodes
        while len(distances) < len(self._nodes):
            break_first = bool(start_node)
            if not break_first:
                start_node = list(set(self._nodes).difference(set(distances.keys())))[0]
            distances[start_node] = 0
            que = deque([start_node])
            while len(que) > 0:
                current_id = que.popleft()
                for child_id in self.children(current_id):
                    if child_id not in distances:
                        que.append(child_id)
                        distances[child_id] = distances[current_id] + 1
                        visitor(child_id)
            if break_first: break
        return distances

    def dist_layers(self, start_node = None, visitor = lambda x:x):
        """
        Return an inverse mapping of BFS distances.

        Perform breadth-first search, but instead of returning a mapping of
        node -> distance, return a mapping of distance -> {nodes}.
        """
        distances = self.bfs(start_node, visitor)
        layers = {}
        for n in distances:
            if distances[n] not in layers:
                layers[distances[n]] = set()
            layers[distances[n]].add(n)
        return layers

    def dfs(self, start_node = None, visitor = lambda x: x):
        """
        Perform a depth-first search starting at start_node.

        Travel along edges connecting parents to children and call
        visitor(node) on each visited node.
        If start_node not given, visit all nodes.
        """
        history = set()
        def visit(node):
            if node not in history:
                history.add(node)
                for child in self.children(node):
                    visit(child)
                visitor(node) # callback comes here
        if not start_node:
            while len(history) < len(self._nodes):
                visit(list(set(self._nodes).difference(history))[0])
        else:
            visit(start_node)

    def dfs_pred(self, start_node, predicate):
        """
        Perform a depth-first search starting at start_node, calling predicate on each node visited.

        If predicate is True, immediately return True.
        Otherwise return False (predicate never is True).
        """
        history = set()
        # the flag needs to be reassignable, so we put it into a list
        flag = [predicate(start_node) is True]
        def visit(node):
            # check for eureka and history
            if node not in history and not flag[0]:
                history.add(node)
                for child in self.children(node):
                    visit(child)
            flag[0] = predicate(node) is True
        visit(start_node)
        return flag[0]

    def topsort(self):
        """Return a list representing a topological ordering of the graph's nodes."""
        ordering = []
        self.dfs(visitor = lambda x: ordering.append(x))
        return ordering[::-1]

    def collect_leaves(self, node_id):
        """Return a list of all leaves below node_id."""
        leafs = []
        def leaf_collector(node):
            if self.out_degree(node) == 0:
                leafs.append(node)
        self.bfs(node_id, leaf_collector)
        return leafs

    def critical_path_length(self):
        """Return length of the longest path in the graph."""
        nodes = self.topsort()
        tpose = self.transpose()
        pl = {}
        for n in nodes:
            # path work at node is work done at the node plus the maximum work on an incoming path
            pl[n] = 1 + max([0]+[pl.get(p, 0) for p in tpose.children(n)])
        return pl[nodes[-1]]

    def dump_graph_dot(self, name='DAG', **kwargs):
        """
        Return string of graph in .dot format.

        All edge and node properties not prefixed with _ are included in the dot output.
        kwargs are assumed to be graph-level attributes, like rankdir.
        """
        output = ['node [fontname="%s",fontsize="%s"]' % ("sans-serif", "12")]
        for i in self:
            # node
            output.append("%s [id=%s,%s]" % (i, '"%s"' % i, ','.join(['%s="%s"' % (k,v) for k, v in
                self.properties(i).items() if not k.startswith('_')])))

            for child in self.children(i):
                # edges
                output.append("%s -> %s [id=%s,%s]" % (i, child, '"%s->%s"' % (i, child),
                    ','.join(['%s="%s"' % (k,v) for k, v in
                    self.edge_properties(i,child).items() if not k.startswith('_')])))
        opts = '\n'.join(["%s=%s" % (k,v) for (k,v) in kwargs.items()])
        output = 'digraph "%s" {\n%s\n%s}\n' % (name, opts, '\n'.join(output))
        return output
