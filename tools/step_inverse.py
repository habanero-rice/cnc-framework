#!/usr/bin/env python

import os
from argparse import ArgumentParser
from pprint import pprint

from cncframework import graph, parser
from cncframework.events.eventgraph import EventGraph
from cncframework.inverse import find_step_inverses, find_blame_candidates, blame_deadlocks

def pprint_inverses(graphData):
    for (step, func) in graphData.stepFunctions.iteritems():
        print "Step {}:".format(step)
        pprint(find_step_inverses(func))


def main():
    bin_name = os.environ['BIN_NAME'] or "cncframework_inv"
    arg_parser = ArgumentParser(prog=bin_name,
                                description="Compute inverse output functions from CnC graph spec.")
    arg_parser.add_argument('specfile', help="CnC graph spec file")
    arg_parser.add_argument('--log', nargs='?', default=None, help="CnC debug log file")
    arg_parser.add_argument('--blame', nargs='?', default=None,
                            help="collection@tag or step@tag to blame")
    args = arg_parser.parse_args()

    # Parse graph spec
    graphAst = parser.cncGraphSpec.parseFile(args.specfile, parseAll=True)
    graphData = graph.CnCGraph("_", graphAst)

    # Construct the event graph if they give us a log file.
    event_graph = None
    if args.log:
        with open(args.log, 'r') as log:
            event_graph = EventGraph(log.readlines(), False, False)

    if not args.blame and not args.log:
        # nothing to blame and no log given, just print out the inverse
        # functions
        return pprint_inverses(graphData)
    if args.blame:
        print "Steps that could be blamed for {}:".format(args.blame)
        pprint(find_blame_candidates(args.blame, graphData, event_graph))
    else:
        # user gives us log without blame, do an "auto-blame"
        # i.e. we perform a blame on the set of all items with a get without a
        # put
        autoblames = blame_deadlocks(graphData, event_graph)
        print "Performing automatic blame on potentially deadlocked items from event log: {}".format(autoblames.keys())
        pprint(autoblames)

if __name__ == '__main__':
    main()
