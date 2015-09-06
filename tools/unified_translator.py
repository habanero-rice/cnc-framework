import argparse, re, sys, os, glob

from cncframework import graph, parser

from jinja2 import Environment, ChoiceLoader, PackageLoader, contextfilter

from ordereddict import OrderedDict

__version__ = "1.1.0"


################################
## Helper functions
################################

@contextfilter
def dispatch_macro(context, macro_name, *args, **kwargs):
    return context.vars[macro_name](*args, **kwargs)

def makeDirP(path):
    if not os.path.exists(path):
        os.makedirs(path)

def makeLink(target, name):
    if os.path.islink(name):
        os.remove(name)
    if os.path.isfile(name):
        print "Skipping link (already exists):", name
    else:
        print "Creating link: {0} -> {1}".format(name, target)
        os.symlink(target, name)

def set_exec(path):
    mode = os.stat(path).st_mode
    xbits = (mode & 0444) >> 2
    os.chmod(path, mode | xbits)


################################
## Unified translator class
################################

class UnifiedTranslator(object):
    def __init__(self, prog_name):
        self.prog_name = prog_name
        self.runtime_name = None
        self.cnc_type = None
        self.template_env = None
        self.args = None
        self.support_dir = None
        self.makefile = None
        # templates
        self.loaders = []
        self.support_files = []
        self.user_files = []
        # all supported platforms
        platforms = OrderedDict([
            ("ocr", self.ocr_x86_init),
            ("ocr/x86", self.ocr_x86_init),
            ("ocr/mpi", self.ocr_x86_mpi_init),
            ("ocr/tg", self.ocr_tg_init),
            ("icnc", self.icnc_x86_init),
            ("icnc/x86", self.icnc_x86_init),
            ("icnc/mpi", self.icnc_x86_mpi_init),
            ("icnc/tcp", self.icnc_x86_tcp_init)
        ])
        # argument parsing
        self.args_init(platforms)
        # platform-specific setup
        platforms[self.args.platform]()
        # parse graph spec
        graphAst = parser.cncGraphSpec.parseFile(self.args.specfile, parseAll=True)
        self.g = graph.CnCGraph(self.graph_name, graphAst)
        # parse tuning specs
        for tuningSpec in (self.args.tuning_spec or []):
            tuningAst = parser.cncTuningSpec.parseFile(tuningSpec, parseAll=True)
            self.g.addTunings(tuningAst)
        # set up template environment
        self.templates_init()

    def args_init(self, platforms):
        # args setup
        desc="CnC unified C API graph translator tool, version {0}. Parses a CnC graph specification, and generates a project from the specification.".format(__version__)
        self.arg_parser = argparse.ArgumentParser(prog=self.prog_name, description=desc)
        self.arg_parser.add_argument('--version', action='version', version='%(prog)s v{0}'.format(__version__))
        self.arg_parser.add_argument("-p", "--platform", choices=platforms.keys(), default="ocr", help="target code generation platform")
        self.arg_parser.add_argument("--ocr-pure", action='store_true', default=False, help="use pure OCR implementation (no platform-specific code)")
        self.arg_parser.add_argument("-t", "--tuning-spec", action='append', help="CnC tuning spec file")
        self.arg_parser.add_argument("specfile", nargs='?', default="", help="CnC graph spec file")
        # parse the args
        self.args = self.arg_parser.parse_args()
        # check platform name
        #if not re.match(r'^(?P<runtime>[^/]+)(?:/(?P<conduit>.+))?$', self.args.platform):
        if not self.args.platform in platforms:
            die("ERROR! Invalid platform name: " + self.args.platform)
        # find default spec file
        if not self.args.specfile:
            specs = glob.glob("*.cnc")
            if len(specs) > 1:
                self.die("ERROR! Conflicting spec files: " + " ".join(specs))
            elif specs:
                self.args.specfile = specs[0]
            else:
                self.die("ERROR! No graph spec file found (*.cnc)")
        # check spec file name
        nameMatch = re.match(r'^(.*/)?(?P<name>[a-zA-Z]\w*)(?P<cnc>\.cnc)?$', self.args.specfile)
        if not nameMatch:
            sys.exit("ERROR! Illegal spec file name: " + self.args.specfile)
        elif not nameMatch.group('cnc'):
            print "WARNING! Spec file name does not end with '.cnc' extension:", self.args.specfile
        self.graph_name = nameMatch.group('name')

    def templates_init(self):
        # the loaders were specified in reverse
        self.loaders = self.loaders[::-1]
        # templates shared by all implemenations using the unified C api
        self.add_template_path("common")
        # this last-level path lets you refer to a specific template by
        # directory (useful when extending a template with the same name)
        # e.g. {% extends "ocr-x86/cncocr_itemcoll.c" %}
        self.add_template_path(".")
        # global values to pass to the template engine
        self.template_params = {
            'cncRuntimeName': self.runtime_name,
            'exit': exit
        }
        # set up template environment
        loader = ChoiceLoader(self.loaders)
        self.template_env = Environment(loader=loader, extensions=['jinja2.ext.with_','jinja2.ext.do'], keep_trailing_newline=True)
        self.template_env.filters['macro'] = dispatch_macro
        # support dir
        self.support_dir = "./cnc_support/" + self.cnc_type

    def add_template_path(self, path):
        self.loaders.append(PackageLoader("cncframework.templates.unified_c_api", path))

    def add_support_file(self, path):
        self.support_files.append(path)

    def add_user_file(self, path):
        self.user_files.append(path)

    def die(self, msg):
        note="\nRun '{0} -h' for usage information.".format(self.arg_parser.prog)
        sys.exit(msg+note)

    def write_template(self, templatepath, filename=None, overwrite=True, destdir=None, executable=False, params=None):
        templatename = os.path.basename(templatepath)
        filename = filename or templatename
        destdir = destdir or self.support_dir
        params = params or self.template_params
        # prepend graph name to files starting with "_"
        if filename[0] == '_':
            filename = self.graph_name + filename
        outpath = os.path.join(destdir, filename)
        # don't overwrite user files
        if overwrite or not os.path.isfile(outpath):
            template = self.template_env.get_template(templatepath)
            contentsRaw = template.render(g=self.g, **params)
            # strips out whitespace errors (caused by lines with indented template commands)
            contents = re.sub(r"[ \t]+(?=[\r\n]|$)", "", contentsRaw)
            try:
                os.remove(filename)
            except OSError:
                pass
            with open(outpath, 'w') as outfile:
                outfile.write(contents)
                outfile.close()
            print "Writing file:", outpath
            if executable:
                set_exec(outpath)
        elif not overwrite:
            print "Skipping file (already exists):", outpath

    def write_files(self):
        # set up support dir
        makeDirP(self.support_dir)
        # support files
        for f in self.support_files:
            self.write_template(f)
        self.write_template("_defs.mk")
        # graph files
        fname = "{0}.h".format(self.graph_name)
        self.write_template("Graph.h", filename=fname)
        fname = "{0}.c".format(self.graph_name)
        self.write_template("Graph.c", filename=fname, overwrite=False, destdir=".")
        self.write_template("_defs.h", overwrite=False, destdir=".")
        # steps
        stepParams = dict(self.template_params)
        for stepName in self.g.stepFunctions.keys():
            stepParams['targetStep'] = stepName
            fname = "{0}_{1}.c".format(self.graph_name, stepName)
            self.write_template("StepFunc.c", filename=fname, overwrite=False, destdir=".", params=stepParams)
        # user files
        self.write_template("Main.c", overwrite=False, destdir=".")
        for f in self.user_files:
            self.write_template(f, overwrite=False, destdir=".")
        # default makefile link
        if self.makefile:
            makeLink(self.makefile, "Makefile")

    ################################
    ## OCR
    ################################

    def ocr_base_init(self):
        self.runtime_name = "cncocr"
        # common ocr templates
        self.add_template_path("ocr-common")
        # platform-specific templates
        self.add_template_path("ocr-" + self.cnc_type)
        # override with "pure" implementation, if requested
        if self.args.ocr_pure:
            self.add_template_path("ocr-pure")
        # add runtime files
        self.add_support_file("cncocr_platform.h")
        self.add_support_file("cncocr_internal.h")
        self.add_support_file("cnc_common.h")
        self.add_support_file("cnc_common.c")
        self.add_support_file("cncocr.h")
        self.add_support_file("cncocr.c")
        self.add_support_file("cncocr_itemcoll.c")
        # add graph scaffolding files
        self.add_support_file("_internal.h")
        self.add_support_file("_context.h")
        self.add_support_file("_step_ops.c")
        self.add_support_file("_item_ops.c")
        self.add_support_file("_graph_ops.c")
        # makefile
        self.makefile = "Makefile."+self.cnc_type
        self.add_user_file(self.makefile)

    def ocr_x86_init(self):
        self.cnc_type = "x86"
        self.ocr_base_init()

    def ocr_x86_mpi_init(self):
        self.cnc_type = "x86-mpi"
        # x86-mpi builds off the x86 templates
        self.add_template_path("ocr-x86")
        self.ocr_base_init()

    def ocr_tg_init(self):
        # TG always uses pure OCR implementation
        self.cnc_type = "tg"
        self.args.ocr_pure = True
        self.ocr_base_init()
        self.add_user_file("config.cfg")
        self.add_user_file("Makefile.tg-x86")

    ################################
    ## Intel CnC
    ################################

    def icnc_x86_init(self):
        self.runtime_name = "icnc"
        self.cnc_type = self.runtime_name
        self.add_template_path("icnc")
        # add runtime files
        self.add_support_file("icnc.h")
        self.add_support_file("icnc_internal.h")
        self.add_support_file("icnc.cpp")
        self.add_support_file("cnc_common.h")
        self.add_support_file("cnc_common.c")
        # add graph scaffolding files
        self.add_support_file("_internal.h")
        self.add_support_file("_context.h")
        self.add_support_file("_step_ops.c")
        self.add_support_file("_context.cpp")
        # makefile
        self.makefile = "Makefile.icnc"
        self.add_user_file(self.makefile)

    def icnc_x86_mpi_init(self):
        self.icnc_x86_init()
        self.makefile = "Makefile.icnc-mpi"
        self.add_user_file(self.makefile)

    def icnc_x86_tcp_init(self):
        self.icnc_x86_init()
        self.add_user_file("icnc-tcp-start.sh")
        self.makefile = "Makefile.icnc-tcp"
        self.add_user_file(self.makefile)


################################
## Invoke the translator
################################

UnifiedTranslator("ucnc_t").write_files()
