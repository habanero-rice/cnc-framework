#!/usr/bin/env python
import sys
from collections import defaultdict

argc = len(sys.argv)
if argc == 2:
    infile = sys.argv[1]
elif argc == 1:
    infile = "cnc_events.log"
else:
    sys.exit("Too many arguments.")

items = defaultdict(set)
steps = defaultdict(dict)
deps = []

# Parse the file
with open(infile, 'r') as f:
    for line in f:
        # parse line
        lhs, rhs = line.split(" @ ")
        cmd, coll = lhs.split(" ")
        tag = tuple(map(int, rhs.split(", ")))
        # handle
        if cmd == 'PUT':
            items[coll].add(tag)
        elif cmd == 'GET-DEP':
            deps.append((coll, tag))
        elif cmd == 'PRESCRIBED':
            steps[coll][tag] = deps
            deps = []
        elif cmd == 'RUNNING':
            steps[coll][tag] = []
        elif cmd == 'DONE':
            del steps[coll][tag]
        else:
            print "Unknown command", cmd

def withFinalizerLast(x):
    collName = x[0]
    return (collName.endswith("_finalize"), x)

# Which steps are stuck?
running = []
print "*** STALLED STEPS ***"
for collname, coll in sorted(steps.iteritems(), key=withFinalizerLast):
    for tag, stepdeps in sorted(coll.iteritems()):
        missing = []
        for dep in sorted(stepdeps):
            icoll, itag = dep
            if not itag in items[icoll]:
                missing.append(dep)
        if missing:
            print collname, tag, missing
        else:
            running.append((collname, tag))

# Which steps were still running at exit?
print
print "*** RUNNING STEPS ***"
for collname, tag in sorted(running):
    print collname, tag
