#!/usr/bin/env python2
# -*- coding: utf-8 -*-

import subprocess, os
from argparse import ArgumentParser
from base64 import b64encode
from os.path import join
from jinja2 import Environment, PackageLoader, Markup
from cncframework.events.eventgraph import EventGraph

loader = PackageLoader('cncframework.events.eventgraph')
templateEnv = Environment(loader = loader)

# Add a base64-encoding filter to Jinja (for embedding icons)
def embed_b64(name):
    path = loader.provider.get_resource_filename(loader.manager,
                                                 join(loader.package_path,
                                                      name))
    with open(path, "rb") as f:
        return b64encode(f.read())

# Include a file directly without any parsing
def include_raw(name):
    return Markup(loader.get_source(templateEnv, name)[0])

templateEnv.globals['embed_b64'] = embed_b64
templateEnv.globals['include_raw'] = include_raw

def main():
    bin_name = os.environ['BIN_NAME'] or "cncframework_eg"
    arg_parser = ArgumentParser(prog=bin_name,
            description="Turn CnC event logs into graphs.")
    arg_parser.add_argument('logfile', help="CnC log file to process")
    arg_parser.add_argument('--html', action="store_true",
            help="Write to stdout as HTML. Requires dot to be installed.")
    arg_parser.add_argument('--no-prescribe', action="store_true",
            help="Do not add prescribe edges to the graph produced.")
    arg_parser.add_argument('--horizontal', action="store_true",
            help="Set rankdir=LR for horizontal graph layout.")
    args = arg_parser.parse_args()

    rankdir = "LR" if args.horizontal else "TB"
    with open(args.logfile, 'r') as log:
        graph = EventGraph(log.readlines(), not args.no_prescribe, args.html)
        if args.html:
            template = templateEnv.get_template("index.html")
            gv = subprocess.Popen(['dot', '-Tsvg'], stdin=subprocess.PIPE,
                    stdout=subprocess.PIPE)
            gv.stdin.write(graph.dump_graph_dot(rankdir=rankdir))
            data = gv.communicate()[0]
            print template.render({
                    'graph_title': args.logfile,
                    'image_data' : data,
                }).encode('utf-8')
        else:
            print graph.dump_graph_dot(rankdir=rankdir)

if __name__ == '__main__':
    main()
