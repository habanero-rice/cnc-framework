from pyparsing import *


##################################################
# ERROR HELPERS

closestMatchError = None

def updateClosestMatchError(err):
    global closestMatchError
    if not closestMatchError:
        closestMatchError = err
    else:
        currentLoc = (closestMatchError.lineno, closestMatchError.col)
        newLoc = (err.lineno, err.col)
        if newLoc > currentLoc:
            closestMatchError = err

class FailureHint(Token):
    """Stores an error for the current clause. When used in conjunction
       with parserErrorWrapper, this helps to return the error for the
       *longest* match rather than the *most recent* match (which is the
       pyparsing module's default behavior)."""
    def __init__(self, msg):
        super(FailureHint,self).__init__()
        self.mayReturnEmpty = True
        self.mayIndexError = False
        self.msg = msg
    def parseImpl(self, s, loc, doActions=True):
        lnum = lineno(loc, s)
        cnum = col(loc, s)
        ex = ParseException(s, loc, self.msg, self)
        updateClosestMatchError(ex)
        raise ex

def failure(msg):
    return FailureHint(msg)

def parserErrorWrapper(parser, specPath):
    try:
        return parser.parseFile(specPath, parseAll=True)
    except ParseException as err:
        updateClosestMatchError(err)
        raise closestMatchError


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
    return Empty() + expr

def joined(parser, sep=""):
    """Join the resulting tokens from the parser into a single token"""
    return Combine(parser, joinString=sep, adjacent=False)

def kind(tokStr):
    """Inject a string literal to denote the kind of the current result."""
    return InjectToken(tokStr)('kind')

def closing(tok):
    """Tries to match a closing token (e.g., a closing bracket),
       and gives a failure if it is not found."""
    return tok | failure("Expected closing '{0}'".format(tok))

def delimiter(tok):
    """Tries to match a delimiter token (e.g., a semicolon),
       and gives a failure if it is not found."""
    return Suppress(tok) | failure("Expected delimiter '{0}'".format(tok))


##################################################
# C-STYLE VARIABLES AND EXPRESSIONS
# (specify step I/O relationships for items/steps)

# C-style identifiers
cVar = Word("_"+alphas, "_"+alphanums)

# C-style expressions
cExpr = Forward() # forward-declaration
cSubExpr = ( "(" + cExpr + closing(")")
           | "[" + cExpr + closing("]")
           | "{" + cExpr + closing("}") )
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

cArraySuffix = "[" + cExpr('arraySize') + closing("]")

##################################################
# Context struct fields declaration
# (used to add custom parameters to the context)

cncContext = ( CaselessKeyword("$context").suppress() + Suppress("{")
             + cExpr('fields') + closing("}").suppress()
             + delimiter(";") )


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

oldRangeExpr = ( "{" + rangeSafeExpr('start') + delimiter("..")
               + rangeSafeExpr('end') + closing("}") )
oldRangeExpr.addParseAction(deprecatedRangeSyntaxWarning)

# Newer-style range functions
rangeFn = CaselessKeyword("$rangeTo")('inclusive') | CaselessKeyword("$range")
rangeExpr = ( rangeFn + delimiter("(") + Optional(scalarExpr('start')
            + ",") + scalarExpr('end') + closing(")") )

rangedTC = Group(kind('RANGED') + (rangeExpr | oldRangeExpr))


##################################################
# ATTRIBUTE DICTIONARY
# (key/value pairs for tuning hints, etc)

attrKey = cVar
attrVal = cTopExpr
attrPair = Group(attrKey + delimiter(":") + attrVal)
attrDictSep = Suppress(",")
attrDict = Suppress("{") + Optional(Dict(attrPair + ZeroOrMore(attrDictSep + attrPair))) + closing("}").suppress()
optAttrs = Optional(attrDict)


##################################################
# TAGS
# (keys for items and tags for steps)

tagDecl = rep1sep(cVar | failure("Expected tag identifier")) | unitExpr
tagExpr = unitExpr | rep1sep(rangedTC | scalarTC | failure("Expected tag expression"))
scalarTagExpr = rep1sep(scalarExpr | failure("Expected tag expression"))


##################################################
# ITEM INSTANCE REFERENCE
# (used in step input/output relationships)

itemRef = (Group("[" + kind('ITEM') + Optional(cVar('binding') + "@")
                + cVar('collName') + delimiter(":")
                + tagExpr('key') + closing("]"))
           | failure("Expected input item reference"))


##################################################
# ITEM COLLECTION DECLARATION
# (specifies the data type,  and key shape)

# mappings from virtual to concrete item collections can be
# specified by a function name, or inline as a tag expression
externalMapping = CaselessKeyword("using") + cVar('funcName')
inlineMapping = delimiter(":") + scalarTagExpr('keyFunc')
mappingFunction = externalMapping | inlineMapping
itemMapping = ( cVar('targetCollName') + mappingFunction
              | failure("Expected item-key mapping function") )

cTypedVar = cType('type') + cVar('collName') + Optional(cArraySuffix)('vecSuffix')
itemDecl = Group("[" + cTypedVar + delimiter(":") + tagDecl('key')
                + Optional("=" + itemMapping)('virtualMapping')
                + closing("]") + delimiter(";"))


##################################################
# STEP INSTANCE REFERENCE
# (used in step output relationships)

stepRef = Group("(" + kind('STEP') + cVar('collName')
               + delimiter(":") + tagExpr('tag') + closing(")"))


##################################################
# I/O CONDITIONALS
# (used to predicate a step's input or output)

# Helpers
kwIf = CaselessKeyword("$if") + kind('IF')
kwElse = CaselessKeyword("$else") + kind('ELSE')
kwWhen = CaselessKeyword("$when") + kind('IF')
instanceRef = itemRef | stepRef | failure("Expected output step/item reference")

def condBlock(ref):
    refBlock = ( "{" + rep1sep(ref)('refs') + closing("}")
               | failure("Expecting a block of instance references") )
    cond = ( "(" + cExpr('cond') + closing(")")
           | failure("Expected a conditional") )
    return ( Group(kwIf + cond + refBlock) + Optional(Group(kwElse + refBlock))
           | Group(Group(ref)('refs') + (kwWhen + cond | kind('ALWAYS'))) )

condItemRefs = rep1sep(condBlock(itemRef))
condInstanceRefs = rep1sep(condBlock(instanceRef))


##################################################
# STEP I/O RELATIONSHIPS
# (specifies tag functions for a step collection)

# make $init and $initialize equivalent
initFnName =  CaselessKeyword("$init") | CaselessKeyword("$initialize").suppress() + InjectToken("$init")

# Helpers for parsing references to other items/steps
stepName = cVar | initFnName | CaselessKeyword("$finalize")
stepDecl = Group("(" + stepName('collName') + delimiter(":")
                + tagDecl('tag') + closing(")"))

stepRelation = ( Group(stepDecl('step')
                      + Optional(delimiter("<-") + condItemRefs('inputs'))
                      + Optional(delimiter("->") + condInstanceRefs('outputs'))
                      + delimiter(";"))
               | failure("Expected step function declaration") )


##################################################
# CnC GRAPH SPEC
# (parses an entire spec file)

graphCtx = Optional(joined(cncContext))
itemColls = ZeroOrMore(itemDecl)
stepColls = OneOrMore(stepRelation)
cncGraphSpec = graphCtx('ctx') + itemColls('itemColls') + stepColls('stepRels')
cncGraphSpec.ignore(cppStyleComment)

def parseGraphFile(specPath):
    return parserErrorWrapper(cncGraphSpec, specPath)


##################################################
# CnC TUNING SPEC
# (parses an entire tuning spec file)

itemTune = Group("[" + cVar('collName') + closing("]") + delimiter(":")
                + attrDict('attrs') + delimiter(";"))
inputTune = delimiter("<-") + "[" + cVar('inputName') + closing("]")
stepTune = Group("(" + cVar('collName') + closing(")") + Optional(inputTune)
                + delimiter(":") + attrDict('attrs') + delimiter(";"))

itemTunings = ZeroOrMore(itemTune)('itemTunings')
stepTunings = ZeroOrMore(stepTune)('stepTunings')

cncTuningSpec = itemTunings + stepTunings
cncTuningSpec.ignore(cppStyleComment)

def parseTuningFile(specPath):
    return parserErrorWrapper(cncTuningSpec, specPath)

