/**
 * Return an object with methods to provide animations for the dag.
 * Assumes that the node id's are distinct nonnegative numbers.
 */
function Animate(dag) {
    "use strict";
    // time between showing nodes (milliseconds)
    var timestep = 100;
    var max_time = (function() {
        var m = 0;
        onAll(function(n) {
            // for integers, ~~x = x and ~~undefined = 0
            m = Math.max(m,n,~~dag.property(n, "running"));
        });
        return m+1;
    })();
    // the current time we are at, in range [0, max time]
    var current_time = 0;
    // flag to determine whether we are paused
    var paused = false;
    // flag to determine whether to autoscroll to new elements
    var autoscroll = false;

    // map of node id -> time connected (i.e. to turn opaque)
    var time_connected = (function() {
        var c = {};
        // steps turn opaque when they "run"
        onAll(function(n) {
            var r = dag.property(n, "running");
            c[n] = (r)?r:(max_time+1);
        });
        // items turn opaque when their in-degree exceeds 0 from a running step
        onAll(null, function(f, t) {
            if (dag.property(t, "type") === "item")
                c[t] = Math.min(c[t], c[f]);
        });
        return c;
    })();
    // map of time -> [nodes that turn opaque at this time]
    // one-to-many inverse map of time_connected
    var connect_timings = (function() {
        var ic = {}
        for (var n in time_connected) {
            if (!time_connected.hasOwnProperty(n)) continue;
            if (ic[time_connected[n]] === undefined)
                ic[time_connected[n]] = [];
            ic[time_connected[n]].push(n);
        }
        return ic;
    })();

    // pre-compute the times at which to show each edge in (O(M))
    // map of time -> [{from:n1, to:n2}] for each edge at given time
    var show_timings = (function() {
        // go by show time for get and prescribe edges, but only show put edges
        // when the step is connected (i.e. run)
        function pickTime(f, t) {
            if (dag.property(f, "type") === "item" ||
                    dag.edgeProperty(f,t,"prescribe"))
                return Math.max(f, t);
            return Math.max(time_connected[f],t);
        }
        var m = {};
        onAll(null, function(f, t) {
            var time = pickTime(f, t);
            if (!m[time])
                m[time] = [];
            m[time].push({from: f, to: t});
        });
        return m;
    })();

    function nodeDom(node_id) {
        // Return the DOM node for the given graph node.
        return dag.property(node_id, '_dom');
    }
    function edgeDom(from, to) {
        // Return the DOM node for the given graph edge.
        return dag.edgeProperty(from, to, '_dom');
    }

    function hide(node1, node2) {
        // Set node or edge style to display:none
        // If one arg given, hide node. If two, hide edge.
        if (node2 === undefined)
            nodeDom(node1).style.display = 'none';
        else
            edgeDom(node1, node2).style.display = 'none'
    }

    function show(node1, node2) {
        // Set node or edge style to display:block
        // If one arg given, show node. If two, show edge.
        if (node2 === undefined) {
            nodeDom(node1).style.display = 'block';
            if (autoscroll)
                scrollTo(node1);
        }
        else
            edgeDom(node1, node2).style.display = 'block'
    }

    function connect(node) {
        // Set the opacity of the node to 1.
        nodeDom(node).style.opacity = '1';
        if (autoscroll)
            scrollTo(node);
    }

    function disconnect(node) {
        // Set the opacity of the node to 0.4.
        nodeDom(node).style.opacity = '0.4';
    }

    function onAll(onNodes, onEdges) {
        // Call onNodes(n) on each node n, onEdges(f,t) on each edge (f->t).
        for (var n in dag.graph()) {
            if (!dag.graph().hasOwnProperty(n)) continue;
            if (onNodes) onNodes(n);
            if (onEdges) {
                var edges = dag.graph()[n];
                for (var i = 0; i < edges.length; i++)
                    onEdges(n, edges[i]);
            }
        }
    }

    function hideAll() {
        // Set graph state to time = 0.
        onAll(hide, hide);
        // initially disconnect everything except the source
        onAll(disconnect);
        connect(0);
        current_time = 0;
    }
    function showAll() {
        // Advance graph state to time = max_time.
        onAll(show, show);
        for (var t in connect_timings) {
            if (!connect_timings.hasOwnProperty(t)) continue;
            if (t > max_time) continue;
            for (var i = 0; i < connect_timings[t].length; i++)
                connect(connect_timings[t][i]);
        }
        current_time = max_time;
    }
    function scrollTo(node) {
        // Scroll the window to this node.
        // If the node is already visible, do not scroll.
        // Otherwise move the viewport such that the node is at top of screen.
        var d = nodeDom(node),
            r = d.getBoundingClientRect();
        // check that node is not in current viewport, then scroll.
        if (r.top < 0 ||
                r.left < 0 ||
                r.bottom > window.innerHeight ||
                r.right > window.innerWidth)
            window.scrollTo(r.left - r.width, r.top - r.height);
    }
    function showNext() {
        // Show the next node and any induced edges.
        // Increment current_time.
        if (dag.hasNode(current_time)) {
            show(current_time);
        }
        if (show_timings[current_time])
            for (var i = 0; i < show_timings[current_time].length; i++)
                show(show_timings[current_time][i].from, show_timings[current_time][i].to);
        if (connect_timings[current_time])
            for (var i = 0; i < connect_timings[current_time].length; i++)
                connect(connect_timings[current_time][i]);
        current_time++;
    }
    function hidePrev() {
        // Hide the last node shown, and decrement current_time.
        current_time--;
        if (dag.hasNode(current_time)) {
            hide(current_time);
        }
        if (show_timings[current_time])
            for (var i = 0; i < show_timings[current_time].length; i++)
                hide(show_timings[current_time][i].from, show_timings[current_time][i].to);
        if (connect_timings[current_time])
            for (var i = 0; i < connect_timings[current_time].length; i++)
                disconnect(connect_timings[current_time][i]);
    }
    function showInOrder() {
        // Show the nodes and induced edges in order at predefined intervals (ms).
        // Stop if the pause() method is called.
        function recur() {
            // show next while we're not paused and not at the end
            if (current_time < max_time && !paused) {
                showNext();
                // recur sets itself on a timeout using given timestep
                setTimeout(recur, getTimestep());
            }
        }
        recur();
    }
    function pause() {
        // Pause current animations.
        paused = true;
    }
    function unpause() {
        // Unset the pause flag. Note: Does not resume animations.
        paused = false;
    }
    // underscore to avoid name conflict. we remove _ on export.
    function _paused() {
        // Return whether animation is paused.
        return paused;
    }
    function getTimestep() {
        // Return current timestep in node playback.
        return timestep;
    }
    function setTimestep(ts) {
        // Set timestep between showing nodes to given value in milliseconds.
        timestep = ts;
    }
    function setAutoscroll(bool) {
        // Set whether to autoscroll during animation.
        autoscroll = bool;
    }
    function isAutoscroll() {
        // Return whether autoscrolling is set.
        return autoscroll;
    }

    return {
        show: show,
        hide: hide,
        onAll: onAll,
        hideAll: hideAll,
        showAll: showAll,
        showInOrder: showInOrder,
        showNext: showNext,
        hidePrev: hidePrev,
        paused: _paused,
        pause: pause,
        unpause: unpause,
        getTimestep: getTimestep,
        setTimestep: setTimestep,
        setAutoscroll: setAutoscroll,
        isAutoscroll: isAutoscroll,
    };
}
