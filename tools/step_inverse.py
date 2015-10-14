#!/usr/bin/env python

import argparse
from cncframework import graph, parser
from cncframework.events.eventgraph import EventGraph
from cncframework.inverse import find_step_inverses, find_blame_candidates, blame_deadlocks
from pprint import pprint


def pprint_inverses(graphData):
    for (step, func) in graphData.stepFunctions.iteritems():
        print "Step {}:".format(step)
        pprint(find_step_inverses(func))


def main():
    argParser = argparse.ArgumentParser(prog="CnCInverse",
                                        description="Compute inverse output functions from CnC graph spec.")
    argParser.add_argument('specfile', help="CnC graph spec file")
    argParser.add_argument('--log', nargs='?', default=None, help="CnC debug log file")
    argParser.add_argument(
        '--blame', nargs='?', default=None, help="collection@tag or step@tag to blame")
    args = argParser.parse_args()

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
