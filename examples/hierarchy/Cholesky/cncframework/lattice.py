import itertools

 # TODO - delete these
import collections
def _force_all(xs):
    if isinstance(xs, collections.Iterable):
        return map(_force_all, xs)
    else:
        return xs

class CompositionException(Exception): pass

class PairwiseComposition(object):
    """
    Heterogeneous composition
    """
    def __init__(self, parent, sibling):
        self.parent = parent
        self.sibling = sibling

class PairwiseDecomposition(object):
    """
    Heterogeneous decomposition
    """
    def __init__(self, children):
        assert len(children) == 2, "Should be a pair"
        self.children = tuple(children)

class TagComponentComposition(object):
    """
    Homogeneous composition
    """
    def __init__(self, parent, component):
        self.parent = parent
        self.component = component

class TagComponentDecomposition(object):
    """
    Homogeneous composition
    """
    def __init__(self, child, component):
        self.child = child
        self.component = component

class ComposedCollection(object):
    def __init__(self, name, tag, bases):
        self.name = name
        self.tag = tag
        self.base_collections = set(bases)
    def __str__(self):
        return "({}: {})".format(self.name, ", ".join(self.tag))

class ComposedPairCollection(ComposedCollection):
    def __init__(self, x, y):
        if len(x.tag) != len(y.tag):
            raise CompositionException("tag length mismatch")
        # sort collections alphabetically by name,
        # that way we get a consistent name for the compositions
        if x.name > y.name:
            x, y = y, x
        name = x.name + y.name
        tag = [ (tx if tx == ty else tx+ty)
                for tx, ty in zip(x.tag, y.tag) ]
        bases = x.base_collections | y.base_collections
        base_names = set(c.name for c in bases)
        assert len(base_names) == len(bases)
        super(ComposedPairCollection, self).__init__(name, tag, bases)
        self.x = x
        self.y = y

class ComposedTagCollection(ComposedCollection):
    def __init__(self, x, i):
        name = x.name
        tag = list(map(str, x.tag))
        tag.remove(str(i))
        assert len(x.tag) == 1+len(tag)
        bases = x.base_collections
        super(ComposedTagCollection, self).__init__(name, tag, bases)
        self.x = x
        self.i = i

class ProxyCollection(ComposedCollection):
    def __init__(self, x):
        assert x.init_args, "Must be a CnCObject"
        name = x.name
        tag = list(map(str, x.tag))
        bases = [x]
        super(ProxyCollection, self).__init__(name, tag, bases)
        self.x = x

class HierarchyElement(object):
    def __init__(self, element, tails):
        self.node = element
        self.tails = tails
    def accept(self, visitor):
        return visitor.for_element(self)

class HierarchyJoin(object):
    def __init__(self, left, right):
        self.left = left
        self.right = right
    def accept(self, visitor):
        return visitor.for_join(self)

class IteratingHierarchyVisitor(object):
    def __init__(self, depth=0):
        self.depth = depth
    def for_element(self, element):
        node = ((self.depth, element.node),)
        # Recursive case
        if element.tails:
            v = IteratingHierarchyVisitor(self.depth+1)
            return tuple(node + seq
                    for tail in element.tails
                    for seq in tail.accept(v))
        # Base case
        else:
            return (node,)
    def for_join(self, join):
        lhs = join.left.accept(self)
        rhs = join.right.accept(self)
        pairs = itertools.product(lhs, rhs)
        return tuple(x+y for x, y in pairs)

def print_full_hierarchies(top):
    v = IteratingHierarchyVisitor()
    for xs in top.accept(v):
        for depth, node in xs:
            print ("  " * depth) + str(node)
        print

# TODO - write a function to print DOT files for hierarchies

class CountingHierarchyVisitor(object):
    def __init__(self):
        self.memos = {}
    def for_element(self, element):
        node = element.node
        can_memoize = node.has_multiple_compositions()
        if can_memoize:
            memo = self.memos.get(node.id)
            if memo: return memo
        # Recursive case
        if element.tails:
            count = 0
            for x in element.tails:
                count += x.accept(self)
        # Base case
        else:
            count = 1
        if can_memoize: self.memos[node.id] = count
        return count
    def for_join(self, join):
        lhs = join.left.accept(self)
        rhs = join.right.accept(self)
        return lhs * rhs

def _build_hierarchy_space(context):
    s = None
    worklist = list(context.nodes.values())
    all_nodes = set(worklist)
    while len(worklist) > 0:
        s = worklist[0]
        sid = context.id_of(s.collection)
        worklist = worklist[1:]
        new_nodes = []
        # homogeneous components
        tag = s.collection.tag
        for i in tag:
            try:
                new_nodes.append(s.compose_across(i))
            except CompositionException:
                pass
        # heterogeneous components
        for u in all_nodes:
            try:
                if context.id_of(u.collection) < sid:
                    new_nodes.append(s.compose_with(u))
            except CompositionException:
                pass
        # record newly found nodes
        for x in new_nodes:
            if not (x in all_nodes):
                worklist.append(x)
        all_nodes.update(new_nodes)
    return s  # the last one is the root

class LatticeContext(object):
    def __init__(self, graph, restrictions={}):
        self.graph = graph
        self.nodes = {}
        self.restrictions = restrictions
        map(self.get_node_for, graph.step_colls.values())
        self.root = _build_hierarchy_space(self)
        assert not (self.root is None)
    def id_of(self, collection):
        return (collection.name, tuple(collection.tag))
    def get_node_for(self, collection):
        assert not (self.nodes is None)
        if hasattr(collection, 'init_args'):
            collection = ProxyCollection(collection)
        node_id = self.id_of(collection)
        node = self.nodes.get(node_id)
        if node is None:
            node = Node(self, collection)
            self.nodes[node_id] = node
        return node
    def lookup(self, collection_string):
        cname, tag_string = collection_string.split(':')
        tag = tuple(x.strip() for x in tag_string.split(','))
        if tag == ('',):
            tag = ()
        return self.nodes[(cname, tag)]
    def make_slice(self, *node_strings):
        return Slice(map(self.lookup, node_strings))
    def find_all_slices(self):
        slices = []
        elements = list(self.nodes.values())
        base_coll_count = len(self.graph.step_colls)
        def _find_slices(xs, covered, i):
            if i < len(elements):
                x = elements[i]
                x_covered = x.collection.base_collections
                # Don't include element x, and recur
                _find_slices(xs, covered, i+1)
                # Try to include x and recur
                if not (x_covered & covered):
                    xs_prime = xs + [x]
                    covered_prime = set(covered)
                    covered_prime.update(x_covered)
                    if len(covered_prime) == base_coll_count:
                        # Found complete slice
                        slices.append(tuple(xs_prime))
                    else:
                        # The current slice is incomplete, so keep searching
                        _find_slices(xs_prime, covered_prime, i+1)
        _find_slices([], set([]), 0)
        return [ Slice(xs) for xs in slices ]
    def find_all_full_hierarchies(self):
        memos = {}
        base_coll_count = len(self.graph.step_colls)
        def _handle_pairwise_decomp(pair_decomp):
            childX, childY = map(_find_full_hierachies, pair_decomp.children)
            return HierarchyJoin(childX, childY)
        def _find_full_hierachies(element):
            # Check for memoized answer
            can_memoize = element.has_multiple_compositions()
            if can_memoize:
                memo = memos.get(element.id)
                if memo: return memo
            # Choose homogeneous decompositions
            tails = [_find_full_hierachies(x.child)
                    for x in element.tag_decompositions]
            # Choose heterogeneous decompositions
            tails.extend(itertools.imap(
                _handle_pairwise_decomp,
                element.pairwise_decompositions))
            # Save memo and return result
            result = HierarchyElement(element, tails)
            if can_memoize:
                memos[element.id] = result
            return result
        return _find_full_hierachies(self.root)

class Node(object):
    def __init__(self, context, collection):
        self.context = context
        self.collection = collection
        self.id = context.id_of(collection)
        self.pairwise_compositions = []
        self.pairwise_decompositions = []
        self.tag_compositions = []
        self.tag_decompositions = []
        self.descendents = set()
    def compose_with(self, sibling, parent=None):
        # sort collections alphabetically by name,
        # that way we get a consistent name for the compositions
        x, y = self, sibling
        if x.collection.name > y.collection.name:
            x, y = y, x
        xc = x.collection
        yc = y.collection
        if self.context.restrictions.get(xc.name+yc.name):
            raise CompositionException("restricted pair compose")
        if xc.base_collections & yc.base_collections:  # intersecting bases
            raise CompositionException("self pair compose")
        if parent is None:
            parent_coll = ComposedPairCollection(xc, yc)
            parent = x.context.get_node_for(parent_coll)
        compA = PairwiseComposition(parent, y)
        x.pairwise_compositions.append(compA)
        compB = PairwiseComposition(parent, x)
        y.pairwise_compositions.append(compA)
        decomp = PairwiseDecomposition([x, y])
        parent.pairwise_decompositions.append(decomp)
        # parent's descendents are the descendents of
        # both children, plus the two children
        parent.descendents.update(self.descendents)
        parent.descendents.update(sibling.descendents)
        parent.descendents.update([self, sibling])
        return parent
    def compose_across(self, component, parent=None):
        allowed = self.context.restrictions.get(component, ())
        if allowed and not (self.collection.name in allowed):
            raise CompositionException("restricted tag")
        if parent is None:
            parent_coll = ComposedTagCollection(self.collection, component)
            parent = self.context.get_node_for(parent_coll)
        assert not (parent is None)
        comp = TagComponentComposition(parent, component)
        self.tag_compositions.append(comp)
        decomp = TagComponentDecomposition(self, component)
        parent.tag_decompositions.append(decomp)
        # parent's descendents are the descendents of
        # this node, plus this node
        parent.descendents.update(self.descendents)
        parent.descendents.add(self)
        return parent
    def has_multiple_compositions(self):
        pcs = len(self.pairwise_compositions)
        tcs = len(self.tag_compositions)
        return pcs + tcs > 0
    def is_first_child(self):
        if self.pairwise_compositions:
            parent = self.pairwise_compositions[0].parent
            firstSibling = parent.pairwise_decompositions[0].children[0]
            return self == firstSibling
        elif self.tag_compositions:
            parent = self.tag_compositions[0].parent
            firstSibling = parent.tag_decompositions[0].child
            return self == firstSibling
        else:
            return True
    def __str__(self):
        return str(self.collection)
    def __repr__(self):
        return "<Node {:#x}>{}".format(hash(self), self.collection)

class Slice(object):
    def __init__(self, nodes):
        assert len(nodes) > 0
        self.nodes = tuple(nodes)
        self.context = nodes[0].context
        self.mapping = {}
        for n in nodes:
            for c in n.collection.base_collections:
                self.mapping[c.name] = n
        graph = self.context.graph
        assert len(self.mapping) == len(graph.step_colls), "Incomplete slice"
    def node_for(self, coll_name):
        return self.mapping[coll_name]
    def __str__(self):
        return "{" + ", ".join(map(str, self.nodes)) + "}"
    def __repr__(self):
        return "<Slice {:#x}>{}".format(hash(self), self)
