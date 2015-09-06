# -*- coding: utf-8 -*-
import sys, re
from counter import Counter
from collections import defaultdict
from cncframework.events.dag import DAG
import cncframework.events.styles as styles
import cncframework.events.actions as actions

class EventGraph(DAG):
    '''
    Directed acyclic graph for a CnC-OCR event log. Assumes that execution is
    serialized, i.e. no two activities can be running at the same time.
    '''
    def __init__(self, event_log, prescribe=True, html=False):
        """
        EventGraph: create a DAG representing an event log

        event_log parameter should be a list of strings, each element being a
        line in the event log.
        prescribe indicates whether prescribe edges will be added to the graph.
        html indicates whether the graph should be prepared for HTML output.
        This option embeds a bit of extra ordering information in the graph.
        """
        super(EventGraph, self).__init__()
        self.init_vars(prescribe, html)
        for event in event_log:
            self.process_event(event)
        self.post_process()

    def init_vars(self, prescribe, html):
        """
        Initialize instance variables to track events.

        Initialize internal variables which track state changes during
        process_event. Also put the init step on the graph as node 0.
        """
        # whether to prescribe, whether it's html output
        self.prescribe = prescribe
        self.html = html
        # id of the next node (start at 1 since init = 0)
        self._id_count = 1
        # action_label_tag identifier -> node id
        self._cache_node_ids = {}
        # [(id,label,collection) for get events on the next activity prescribed]
        self._activity_gets = []
        # list of node id's for steps that have entered running state
        self._steps_run = []
        # prescribe node id -> (prescriber, [items in get list])
        self._steps_prescribed = {}
        # list of node id's for items that have been get
        self._items_gotten = []
        # list of node id's for items that have been put
        self._items_put = []
        # put init on the graph and style it like a step
        self.add_node(0)
        self.set_property(0, "label", "init")
        self.style_step(0)
        if self.html:
            self.mark_running_time(0, 1)
        # id of last step to enter running state
        self._last_running_activity_tag = 0
        # the id of the finalize node
        self.finalize_node = None

    def process_event(self, event):
        """
        Add event, a line from the event log, to the DAG.

        Requires that init_vars be called first, and expects that the rest of
        the event log up to this event has already been processed.
        """
        # skip anything without an @
        if "@" not in event:
            return
        # action is one of the things defined in actions
        # label is either the collection or the step name, depending on action
        # tag is the tag of the step or collection
        # format: ACTION  LABEL @ TAG
        pattern = re.compile(r'([^\s]+) ([^\s]+) @ (.+)')
        match = pattern.match(event)
        action, label, tag = [match.group(i+1) for i in range(3)]
        # make sure that cncPrescribe_StepName and StepName are treated the same
        node_id = self.create_node_id(action, label, tag)
        node_label = self.create_node_label(action, label, tag)
        if action == actions.PRESCRIBED:
            self.add_get_edges(node_id, node_label, self._activity_gets)
            if self.prescribe:
                self.add_prescribe_edge(self._last_running_activity_tag, node_id)
            self.style_step(node_id)
            # clear out the activity get list to prepare for next prescribe
            self._activity_gets = []
            # track the finalize node
            if label.endswith("_finalize"):
                self.finalize_node = node_id

        elif action == actions.RUNNING:
            # create a new id which notes when this node runs
            # only do this for html output because it's used for animations
            if self.html:
                self.mark_running_time(node_id,
                        self.create_node_id(action, label, tag, force = True))
            # record this tag as being the currently running activity
            self._last_running_activity_tag = node_id
            self._steps_run.append(node_id)

        elif action == actions.DONE:
            # nothing to do for this action
            pass

        elif action == actions.GET_DEP:
            # happens before a step is prescribed, so we keep track of these items
            self._activity_gets.append((node_id, node_label, label))
            self._items_gotten.append(node_id)

        elif action == actions.PUT:
            self.add_put_edges(node_id, node_label,
                               label, self._last_running_activity_tag)
            self._items_put.append(node_id)

        else:
            print >>sys.stderr, "Unrecognized action %s" % action

    def create_node_id(self, action, label, tag, force = False):
        """
        Return a node id for the given action, label, tag.

        Node id's are integers in the order of appearance in the log.
        If create_node_id is called more than once with the same parameters,
        the same id will be returned each time.
        However if force is True, then a new id will be created even if we have
        seen these arguments before.
        """
        ident = "%s %s" % (label, tag)
        if ident not in self._cache_node_ids or force:
            self._cache_node_ids[ident] = self._id_count
            self._id_count += 1
        return self._cache_node_ids[ident]

    def create_node_label(self, action, label, tag):
        """Return human-readable label for given action, label, tag."""
        return "%s: %s" % (label, tag.replace(", ", ","))

    def mark_running_time(self, step_id, running_time):
        """Do something to step_id to indicate that it runs at running_time."""
        self.set_property(step_id, "href", "%d" % running_time)

    def style_step(self, step_id):
        """Style the node for a step."""
        self.set_property(step_id, "color", styles.color('step'))

    def style_item(self, item_id, label, collection):
        """Style the node for an item."""
        self.set_property(item_id, "shape", styles.shape('item'))
        self.set_property(item_id, "color", styles.color('item', collection))
        self.set_property(item_id, "label", label)

    def add_get_edges(self, step, step_label, items):
        """
        Add get edges from each item node (id, label) to step node id.

        Adds, styles, and labels item nodes if they are not already on the
        graph.
        """
        self.add_node_with_parents(step, [i[0] for i in items])
        self.set_property(step, "label", step_label)
        # these are collection nodes, so make them green boxes
        for n, label, collection in items:
            self.style_item(n, label, collection)

    def add_put_edges(self, item_id, item_label, collection, step_id):
        """
        Add put edge from step node id to the item of given collection

        Also style and label the item node.
        Note: Both add_get_edges and add_put_edges will add and style item
        nodes. This is done because PUT ⇏  GET.
        """
        self.add_node_with_parents(item_id, [step_id])
        self.style_item(item_id, item_label, collection)

    def add_prescribe_edge(self, parent, child):
        """Add and style a prescribe edge from the parent id to the child id."""
        self.add_child(parent, child)
        self.set_edge_property(parent, child, "style", styles.style("prescribe"))

    def post_process(self):
        """
        Perform some post processing tasks on the completed graph.

        Emit warnings and/or highlight occurrences of possible bugs:
        -- Items put multiple times
        -- Steps prescribed multiple times
        -- Steps prescribed without being run
        -- Items gotten without a put and items put without a get
        -- Nodes with no path to the final finalize.
        """
        # warn for items in sequence of node ids appearing more than once
        def warn_on_duplicates(sequence, verb):
            # print a warning on duplicates in a sequence
            counts = Counter(sequence)
            for k in counts:
                if counts[k] > 1:
                    print >>sys.stderr, "Warning: %s %s %d times." % (
                            self.property(k, 'label', ''), verb, counts[k])
        # emit warning if set of node id's is nonempty, and set color if given
        def warn_on_existence(s, msg, color=None):
            if len(s) > 0:
                if color:
                    map(lambda n: self.set_property(n, 'color', color), s)
                else:
                    map(lambda n: self.set_property(n, 'penwidth', 2.5), s)
                print >>sys.stderr, "Highlighting in %s:\n%s: %s" % (
                        color if color else 'bold',
                        msg,
                        ', '.join(map(lambda i: self.property(i, 'label', ''), s)))
        warn_on_duplicates(self._steps_prescribed.keys(), "prescribed")
        warn_on_duplicates(self._items_put, "put")
        # warn on items gotten but not put or put without get
        gotten_without_put = set(self._items_gotten).difference(set(self._items_put))
        put_without_get = set(self._items_put).difference(set(self._items_gotten))
        warn_on_existence(gotten_without_put, "Items with GET without PUT",
                          styles.color('get_without_put'))
        warn_on_existence(put_without_get, "Items with PUT without GET",
                          styles.color('put_without_get'))
        # warn on nodes that have no path to the finalize
        if self.finalize_node is not None:
            trans = self.transpose()
            visits = {self.finalize_node}
            trans.bfs(self.finalize_node, visitor = visits.add)
            warn_on_existence(set(self).difference(visits),
                            "Nodes without path to FINALIZE")
