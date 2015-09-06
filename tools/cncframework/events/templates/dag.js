function DAG(graph) {
    "use strict";
    if (graph === undefined) {
        graph = {};
    }
    var g = graph,
        nprops = {}, // map node id -> {properties}
        eprops = {}; // map edge id -> {properties}

    function _graph() {
        return g;
    }

    function transpose() {
        // Return the transpose (reverse edges) of the graph.
        var p = {};
        for (var n in g) {
            if (!g.hasOwnProperty(n)) continue;
            p[n] = [];
        }
        for (var n in g) {
            if (!g.hasOwnProperty(n)) continue;
            for (var c = 0; c < g[n].length; c++) {
                p[g[n][c]].push(n);
            }
        }
        return DAG(p);
    }

    function numNodes() {
        // Return the number of nodes in the graph.
        var total = 0;
        for (var n in g) {
            if (!g.hasOwnProperty(n)) continue;
            total++;
        }
        return total;
    }

    function children(node) {
        // Return an array of the children of the given node.
        return g[node];
    }

    function addNode(id, children) {
        // Add node with given id and optional children to the graph.
        if (children === undefined) {
            children = [];
        }
        g[id] = children;
    }

    function hasNode(id) {
        // Return whether id is defined.
        return g[id] !== undefined;
    }

    function addEdge(from, to) {
        // Add an edge from one node to another.
        g[from].push(to);
    }

    function setProperty(node, prop, val) {
        // Set property prop of node to val.
        if (nprops[node] === undefined) {
            nprops[node] = {};
        }
        nprops[node][prop] = val;
    }

    function property(node, prop) {
        // Return the value of prop for given node.
        return nprops[node][prop];
    }

    function setEdgeProperty(fr, to, prop, val) {
        // Set given property of edge fr->to to val.
        if (eprops[[fr,to]] === undefined) {
            eprops[[fr,to]] = {};
        }
        eprops[[fr,to]][prop] = val;
    }

    function edgeProperty(fr, to, prop) {
        // Return given property of the edge fr->to.
        return eprops[[fr,to]][prop];
    }
    // module exports
    return {
        // edge creation and retrieval
        graph: _graph,
        addNode: addNode,
        addEdge: addEdge,
        hasNode: hasNode,
        children: children,

        // properties
        property: property,
        setProperty: setProperty,
        edgeProperty: edgeProperty,
        setEdgeProperty: setEdgeProperty,
        numNodes: numNodes,
    };
}

