from pyparsing import *


##################################################
# PARSER HELPERS

class InjectToken(Token):
    """Matches nothing and returns the provided string as a new token.
       Useful for tagging groups with an identifying string."""
    def __init__(self, strToken):
        super(InjectToken,self).__init__()
        self.mayReturnEmpty = False
        self.mayIndexError = False
        self.returnString = strToken
    def parseImpl(self, instring, loc, doActions=True):
        return loc, self.returnString

def rep1sep(repeated, separator=","):
    """Repeated pattern with ignored separator. Similar to
       pyparsing.delimitedList, but this is non-empty."""
    sep = Suppress(separator)
    return Group(repeated + ZeroOrMore(sep + repeated))

def notSpace(expr):
    """Prevents wrapped parser from matching only whitespace.
       Only when whitespace can be matched (e.g. CharsNotIn)."""
    return Optional(White()).suppress() + expr

def joined(parser, sep=""):
    """Join the resulting tokens from the parser into a single token"""
    return Combine(parser, joinString=sep, adjacent=False)

def kind(tokStr):
    """Inject a string literal to denote the kind of the current result."""
    return InjectToken(tokStr)('kind')


##################################################
# C-STYLE VARIABLES AND EXPRESSIONS
# (specify step I/O relationships for items/steps)

# C-style identifiers
cVar = Word("_"+alphas, "_"+alphanums)

# C-style expressions
cExpr = Forward() # forward-declaration
cSubExpr = "(" + cExpr + ")" | "[" + cExpr + "]" | "{" + cExpr + "}"
cExpr <<= ZeroOrMore(CharsNotIn("()[]{}") | cSubExpr) # concrete definition
cExpr.leaveWhitespace()
cTopExpr = joined(notSpace(ZeroOrMore(CharsNotIn("()[]{},") | cSubExpr)))

# Unit expression (zero-tuple, kind of like "void")
# (can be used as a singleton tag/key)
unitExpr = Group(Suppress("(") + Suppress(")"))


##################################################
# C-STYLE TYPES
# (used in item collection declarations)

cStar = Literal("*")
cTypeComponent = cVar + FollowedBy(cVar | cStar)

cTypeBase = joined(OneOrMore(cTypeComponent), " ")
cTypeStars = joined(ZeroOrMore(cStar))

cType = cTypeBase('baseType') + cTypeStars('stars')

cArraySuffix = "[" + cExpr('arraySize') + "]"

##################################################
# Context struct fields declaration
# (used to add custom parameters to the context)

cncContext = CaselessKeyword("$context").suppress() + Suppress("{")\
           + cExpr('fields') + Suppress("}") + Suppress(";")


##################################################
# SCALAR TAG FUNCTION COMPONENTS
# (used in tag functions)

scalarExpr = joined(notSpace(OneOrMore(CharsNotIn("()[]{},") | cSubExpr)))
scalarTC = Group(kind('SCALAR') + scalarExpr('expr'))


##################################################
# RANGED TAG FUNCTION COMPONENTS
# (used in tag functions)

# Old range syntax support
# Helper parsers for ranges (to avoid parsing the "..")
singleDotExpr = Regex(r"(\.?[^{}[\].])+")
rangeSafeExpr = joined(OneOrMore(singleDotExpr | cSubExpr))
rangeSafeExpr.leaveWhitespace()

def deprecatedRangeSyntaxWarning(s, loc, tok):
    lnum = lineno(loc, s)
    ltxt = line(loc, s)
    print "WARNING! Using deprecated range syntax on line {0}:"
    print "\t", ltxt
    print "\t(Please the $range or $rangeTo function instead.)\n"

oldRangeExpr = "{" + rangeSafeExpr('start') + ".." + rangeSafeExpr('end') + "}"
oldRangeExpr.addParseAction(deprecatedRangeSyntaxWarning)

# Newer-style range functions
rangeFn = CaselessKeyword("$rangeTo")('inclusive') | CaselessKeyword("$range")
rangeExpr = rangeFn + "(" + Optional(scalarExpr('start') + ",") + scalarExpr('end') + ")"

rangedTC = Group(kind('RANGED') + (rangeExpr | oldRangeExpr))


##################################################
# ATTRIBUTE DICTIONARY
# (key/value pairs for tuning hints, etc)

attrKey = cVar
attrVal = cTopExpr
attrPair = Group(attrKey + Suppress(":") + attrVal)
attrDictSep = Suppress(",")
attrDict = Suppress("{") + Optional(Dict(attrPair + ZeroOrMore(attrDictSep + attrPair))) + Suppress("}")
optAttrs = Optional(attrDict)


##################################################
# TAGS
# (keys for items and tags for steps)

tagDecl = rep1sep(cVar) | unitExpr
tagExpr = unitExpr | rep1sep(rangedTC | scalarTC)
scalarTagExpr = rep1sep(scalarExpr)


##################################################
# ITEM INSTANCE REFERENCE
# (used in step input/output relationships)

itemRef = Group("[" + kind('ITEM') + Optional(cVar('binding') + "@") \
               + cVar('collName') + ":" + tagExpr('key') + "]")


##################################################
# ITEM COLLECTION DECLARATION
# (specifies the data type,  and key shape)

# mappings from virtual to concrete item collections can be
# specified by a function name, or inline as a tag expression
externalMapping = CaselessKeyword("using") + cVar('funcName')
inlineMapping = ":" + scalarTagExpr('keyFunc')
mappingFunction = externalMapping | inlineMapping
itemMapping = cVar('targetCollName') + mappingFunction

cTypedVar = cType('type') + cVar('collName') + Optional(cArraySuffix)('vecSuffix')
itemDecl = Group("[" + cTypedVar + ":" + tagDecl('key') \
                + Optional("=" + itemMapping)('virtualMapping') + "]" + ";")


##################################################
# STEP INSTANCE REFERENCE
# (used in step output relationships)

stepRef = Group("(" + kind('STEP') + cVar('collName') \
               + ":" + tagExpr('tag') + ")")


##################################################
# I/O CONDITIONALS
# (used to predicate a step's input or output)

# Helpers
kwIf = CaselessKeyword("$if") + kind('IF')
kwElse = CaselessKeyword("$else") + kind('ELSE')
kwWhen = CaselessKeyword("$when") + kind('IF')
instanceRef = itemRef | stepRef

def condBlock(ref):
    refBlock = "{" + rep1sep(ref)('refs') + "}"
    cond = "(" + cExpr('cond') + ")"
    return Group(kwIf + cond + refBlock) + Optional(Group(kwElse + refBlock)) \
         | Group(Group(ref)('refs') + (kwWhen + cond | kind('ALWAYS')))

condItemRefs = rep1sep(condBlock(itemRef))
condInstanceRefs = rep1sep(condBlock(instanceRef))


##################################################
# STEP I/O RELATIONSHIPS
# (specifies tag functions for a step collection)

# make $init and $initialize equivalent
initFnName =  CaselessKeyword("$init") | CaselessKeyword("$initialize").suppress() + InjectToken("$init")

# Helpers for parsing references to other items/steps
stepName = cVar | initFnName | CaselessKeyword("$finalize")
stepDecl = Group("(" + stepName('collName') + ":" + tagDecl('tag') + ")")

stepRelation = Group(stepDecl('step') \
                    + Optional("<-" + condItemRefs('inputs')) \
                    + Optional("->" + condInstanceRefs('outputs')) + ";")


##################################################
# CNC GRAPH SPEC
# (parses an entire spec file)

graphCtx = Optional(joined(cncContext))
itemColls = ZeroOrMore(itemDecl)
stepColls = OneOrMore(stepRelation)
cncGraphSpec = graphCtx('ctx') + itemColls('itemColls') + stepColls('stepRels')
cncGraphSpec.ignore(cppStyleComment)


##################################################
# CNC TUNING SPEC
# (parses an entire tuning spec file)

itemTune = Group("[" + cVar('collName') + "]" + ":" + attrDict('attrs') + ";")
inputTune = Literal("<-") + "[" + cVar('inputName') + "]"
stepTune = Group("(" + cVar('collName') + ")" + Optional(inputTune) + ":" + attrDict('attrs') + ";")

itemTunings = ZeroOrMore(itemTune)('itemTunings')
stepTunings = ZeroOrMore(stepTune)('stepTunings')

cncTuningSpec = itemTunings + stepTunings
cncTuningSpec.ignore(cppStyleComment)
