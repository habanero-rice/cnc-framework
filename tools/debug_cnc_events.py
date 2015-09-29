#!/usr/bin/env python
import sys, os, os.path
from collections import defaultdict

def die_with_usage():
    bin_name = os.environ['BIN_NAME'] or sys.argv[0]
    sys.exit("Usage: {0} LOG_FILE".format(bin_name))

argc = len(sys.argv)
if argc == 2:
    infile = sys.argv[1]
else:
    die_with_usage()

if not os.path.exists(infile):
    print "ERROR! File not found:", infile
    die_with_usage()

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
