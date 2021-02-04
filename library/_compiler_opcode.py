from compiler.opcode38 import opcode as opcode38
from compiler.opcodebase import Opcode

cmp_op = (
    "<",
    "<=",
    "==",
    "!=",
    ">",
    ">=",
    "in",
    "not in",
    "is",
    "is not",
    "exception match",
    "BAD",
)
hasconst = []
hasname = []
hasjrel = []
hasjabs = []
haslocal = []
hascompare = []
hasfree = []
opmap = {}
opname = ["<%r>" % (op,) for op in range(256)]


def def_op(name, op):
    opname[op] = name
    opmap[name] = op


def name_op(name, op):
    def_op(name, op)
    hasname.append(op)


def jrel_op(name, op):
    def_op(name, op)
    hasjrel.append(op)


def jabs_op(name, op):
    def_op(name, op)
    hasjabs.append(op)


def compare_op(name, op):
    def_op(name, op)
    hascompare.append(op)


def local_op(name, op):
    def_op(name, op)
    haslocal.append(op)


def free_op(name, op):
    def_op(name, op)
    hasfree.append(op)


def const_op(name, op):
    def_op(name, op)
    hasconst.append(op)


def_op("POP_TOP", 1)
def_op("ROT_TWO", 2)
def_op("ROT_THREE", 3)
def_op("DUP_TOP", 4)
def_op("DUP_TOP_TWO", 5)
def_op("ROT_FOUR", 6)
def_op("NOP", 9)
def_op("UNARY_POSITIVE", 10)
def_op("UNARY_NEGATIVE", 11)
def_op("UNARY_NOT", 12)
def_op("UNARY_INVERT", 15)
def_op("BINARY_MATRIX_MULTIPLY", 16)
def_op("INPLACE_MATRIX_MULTIPLY", 17)
def_op("BINARY_POWER", 19)
def_op("BINARY_MULTIPLY", 20)
def_op("BINARY_MODULO", 22)
def_op("BINARY_ADD", 23)
def_op("BINARY_SUBTRACT", 24)
def_op("BINARY_SUBSCR", 25)
def_op("BINARY_FLOOR_DIVIDE", 26)
def_op("BINARY_TRUE_DIVIDE", 27)
def_op("INPLACE_FLOOR_DIVIDE", 28)
def_op("INPLACE_TRUE_DIVIDE", 29)
const_op("LOAD_BOOL", 48)
local_op("LOAD_FAST_REVERSE_UNCHECKED", 49)
def_op("GET_AITER", 50)
def_op("GET_ANEXT", 51)
def_op("BEFORE_ASYNC_WITH", 52)
def_op("BEGIN_FINALLY", 53)
def_op("END_ASYNC_FOR", 54)
def_op("INPLACE_ADD", 55)
def_op("INPLACE_SUBTRACT", 56)
def_op("INPLACE_MULTIPLY", 57)
def_op("INPLACE_MODULO", 59)
def_op("STORE_SUBSCR", 60)
def_op("DELETE_SUBSCR", 61)
def_op("BINARY_LSHIFT", 62)
def_op("BINARY_RSHIFT", 63)
def_op("BINARY_AND", 64)
def_op("BINARY_XOR", 65)
def_op("BINARY_OR", 66)
def_op("INPLACE_POWER", 67)
def_op("GET_ITER", 68)
def_op("GET_YIELD_FROM_ITER", 69)
def_op("PRINT_EXPR", 70)
def_op("LOAD_BUILD_CLASS", 71)
def_op("YIELD_FROM", 72)
def_op("GET_AWAITABLE", 73)
def_op("INPLACE_LSHIFT", 75)
def_op("INPLACE_RSHIFT", 76)
def_op("INPLACE_AND", 77)
def_op("INPLACE_XOR", 78)
def_op("INPLACE_OR", 79)
def_op("WITH_CLEANUP_START", 81)
def_op("WITH_CLEANUP_FINISH", 82)
def_op("RETURN_VALUE", 83)
def_op("IMPORT_STAR", 84)
def_op("SETUP_ANNOTATIONS", 85)
def_op("YIELD_VALUE", 86)
def_op("POP_BLOCK", 87)
def_op("END_FINALLY", 88)
def_op("POP_EXCEPT", 89)
HAVE_ARGUMENT = 90  # Opcodes from here have an argument:
name_op("STORE_NAME", 90)
name_op("DELETE_NAME", 91)
def_op("UNPACK_SEQUENCE", 92)
jrel_op("FOR_ITER", 93)
def_op("UNPACK_EX", 94)
name_op("STORE_ATTR", 95)
name_op("DELETE_ATTR", 96)
name_op("STORE_GLOBAL", 97)
name_op("DELETE_GLOBAL", 98)
const_op("LOAD_CONST", 100)
name_op("LOAD_NAME", 101)
def_op("BUILD_TUPLE", 102)
def_op("BUILD_LIST", 103)
def_op("BUILD_SET", 104)
def_op("BUILD_MAP", 105)
name_op("LOAD_ATTR", 106)
compare_op("COMPARE_OP", 107)
name_op("IMPORT_NAME", 108)
name_op("IMPORT_FROM", 109)
jrel_op("JUMP_FORWARD", 110)
jabs_op("JUMP_IF_FALSE_OR_POP", 111)
jabs_op("JUMP_IF_TRUE_OR_POP", 112)
jabs_op("JUMP_ABSOLUTE", 113)
jabs_op("POP_JUMP_IF_FALSE", 114)
jabs_op("POP_JUMP_IF_TRUE", 115)
name_op("LOAD_GLOBAL", 116)
jrel_op("SETUP_FINALLY", 122)
local_op("LOAD_FAST", 124)
local_op("STORE_FAST", 125)
local_op("DELETE_FAST", 126)
def_op("STORE_ANNOTATION", 127)
def_op("RAISE_VARARGS", 130)
def_op("CALL_FUNCTION", 131)
def_op("MAKE_FUNCTION", 132)
def_op("BUILD_SLICE", 133)
free_op("LOAD_CLOSURE", 135)
free_op("LOAD_DEREF", 136)
free_op("STORE_DEREF", 137)
free_op("DELETE_DEREF", 138)
def_op("CALL_FUNCTION_KW", 141)
def_op("CALL_FUNCTION_EX", 142)
jrel_op("SETUP_WITH", 143)
def_op("EXTENDED_ARG", 144)
EXTENDED_ARG = 144
def_op("LIST_APPEND", 145)
def_op("SET_ADD", 146)
def_op("MAP_ADD", 147)
free_op("LOAD_CLASSDEREF", 148)
def_op("BUILD_LIST_UNPACK", 149)
def_op("BUILD_MAP_UNPACK", 150)
def_op("BUILD_MAP_UNPACK_WITH_CALL", 151)
def_op("BUILD_TUPLE_UNPACK", 152)
def_op("BUILD_SET_UNPACK", 153)
jrel_op("SETUP_ASYNC_WITH", 154)
def_op("FORMAT_VALUE", 155)
def_op("BUILD_CONST_KEY_MAP", 156)
def_op("BUILD_STRING", 157)
def_op("BUILD_TUPLE_UNPACK_WITH_CALL", 158)
name_op("LOAD_METHOD", 160)
def_op("CALL_METHOD", 161)
jrel_op("CALL_FINALLY", 162)
def_op("POP_FINALLY", 163)
compare_op("COMPARE_NE_STR", 178)
jrel_op("FOR_ITER_GENERATOR", 179)
def_op("STORE_SUBSCR_DICT", 180)
def_op("BINARY_MUL_SMALLINT", 181)
def_op("BINARY_SUBSCR_DICT", 182)
def_op("BINARY_SUBSCR_TUPLE", 183)
def_op("INPLACE_SUB_SMALLINT", 184)
def_op("STORE_SUBSCR_LIST", 185)
def_op("BINARY_SUBSCR_LIST", 186)
name_op("LOAD_ATTR_INSTANCE_SLOT_DESCR", 187)
compare_op("COMPARE_IN_LIST", 188)
compare_op("COMPARE_IN_DICT", 189)
compare_op("COMPARE_IN_TUPLE", 190)
compare_op("COMPARE_IN_STR", 191)
compare_op("COMPARE_IN_POLYMORPHIC", 192)
compare_op("COMPARE_IN_MONOMORPHIC", 193)
compare_op("COMPARE_IN_ANAMORPHIC", 194)
def_op("BINARY_FLOORDIV_SMALLINT", 195)
def_op("BINARY_AND_SMALLINT", 196)
jrel_op("FOR_ITER_STR", 197)
jrel_op("FOR_ITER_RANGE", 198)
jrel_op("FOR_ITER_TUPLE", 199)
jrel_op("FOR_ITER_DICT", 200)
jrel_op("FOR_ITER_LIST", 201)
def_op("INPLACE_ADD_SMALLINT", 202)
def_op("COMPARE_EQ_STR", 203)
compare_op("COMPARE_LE_SMALLINT", 204)
compare_op("COMPARE_NE_SMALLINT", 205)
compare_op("COMPARE_GE_SMALLINT", 206)
compare_op("COMPARE_LT_SMALLINT", 207)
compare_op("COMPARE_GT_SMALLINT", 208)
compare_op("COMPARE_EQ_SMALLINT", 209)
def_op("BINARY_OR_SMALLINT", 210)
def_op("BINARY_SUB_SMALLINT", 211)
def_op("BINARY_ADD_SMALLINT", 212)
def_op("INPLACE_OP_POLYMORPHIC", 213)
def_op("INPLACE_OP_MONOMORPHIC", 214)
compare_op("COMPARE_OP_POLYMORPHIC", 215)
compare_op("COMPARE_OP_MONOMORPHIC", 216)
def_op("BINARY_OP_POLYMORPHIC", 217)
def_op("BINARY_OP_MONOMORPHIC", 218)
def_op("STORE_SUBSCR_POLYMORPHIC", 219)
def_op("STORE_SUBSCR_MONOMORPHIC", 220)
jrel_op("FOR_ITER_POLYMORPHIC", 221)
jrel_op("FOR_ITER_MONOMORPHIC", 222)
def_op("BINARY_SUBSCR_POLYMORPHIC", 223)
def_op("BINARY_SUBSCR_MONOMORPHIC", 224)
name_op("STORE_ATTR_INSTANCE_OVERFLOW_UPDATE", 225)
name_op("STORE_ATTR_INSTANCE_OVERFLOW", 226)
name_op("STORE_ATTR_INSTANCE", 227)
name_op("STORE_ATTR_POLYMORPHIC", 228)
name_op("LOAD_ATTR_POLYMORPHIC", 229)
name_op("LOAD_ATTR_INSTANCE_TYPE_BOUND_METHOD", 230)
name_op("LOAD_ATTR_INSTANCE", 231)
name_op("LOAD_METHOD_POLYMORPHIC", 232)
name_op("LOAD_METHOD_INSTANCE_FUNCTION", 233)
def_op("STORE_SUBSCR_ANAMORPHIC", 234)
name_op("LOAD_ATTR_INSTANCE_TYPE", 235)
name_op("LOAD_ATTR_INSTANCE_TYPE_DESCR", 236)
name_op("LOAD_ATTR_INSTANCE_PROPERTY", 237)
name_op("STORE_ATTR_INSTANCE_UPDATE", 238)
name_op("LOAD_ATTR_TYPE", 239)
name_op("LOAD_ATTR_MODULE", 240)
compare_op("COMPARE_IS_NOT", 241)
compare_op("COMPARE_IS", 242)
const_op("LOAD_IMMEDIATE", 243)
local_op("STORE_FAST_REVERSE", 244)
local_op("LOAD_FAST_REVERSE", 245)
name_op("LOAD_METHOD_ANAMORPHIC", 246)
def_op("STORE_GLOBAL_CACHED", 247)
name_op("LOAD_GLOBAL_CACHED", 248)
jrel_op("FOR_ITER_ANAMORPHIC", 249)
compare_op("COMPARE_OP_ANAMORPHIC", 250)
def_op("INPLACE_OP_ANAMORPHIC", 251)
def_op("BINARY_OP_ANAMORPHIC", 252)
def_op("BINARY_SUBSCR_ANAMORPHIC", 253)
name_op("STORE_ATTR_ANAMORPHIC", 254)
name_op("LOAD_ATTR_ANAMORPHIC", 255)


# Copy CPython3.8's opcode for the opcodes-as-attributes (needed by the
# compiler) and the stack effects (also needed by the compiler)
opcode: Opcode = opcode38.copy()
opcode.opmap = opmap.copy()
opcode.hasconst = set(hasconst)
opcode.hasname = set(hasname)
opcode.hasjrel = set(hasjrel)
opcode.hasjabs = set(hasjabs)
opcode.haslocal = set(haslocal)
opcode.hascompare = set(hascompare)
opcode.hasfree = set(hasfree)


def add_synonym(old, new):
    opcode.stack_effects[new] = opcode.stack_effects[old]
    setattr(opcode, new, opcode.opmap[old])


add_synonym("BINARY_ADD", "BINARY_OP_ANAMORPHIC")
add_synonym("BINARY_SUBSCR", "BINARY_SUBSCR_ANAMORPHIC")
add_synonym("COMPARE_OP", "COMPARE_IN_ANAMORPHIC")
add_synonym("COMPARE_OP", "COMPARE_IS")
add_synonym("COMPARE_OP", "COMPARE_IS_NOT")
add_synonym("COMPARE_OP", "COMPARE_OP_ANAMORPHIC")
add_synonym("FOR_ITER", "FOR_ITER_ANAMORPHIC")
add_synonym("INPLACE_ADD", "INPLACE_OP_ANAMORPHIC")
add_synonym("LOAD_ATTR", "LOAD_ATTR_ANAMORPHIC")
add_synonym("LOAD_CONST", "LOAD_BOOL")
add_synonym("LOAD_FAST", "LOAD_FAST_REVERSE_UNCHECKED")
add_synonym("LOAD_METHOD", "LOAD_METHOD_ANAMORPHIC")
add_synonym("STORE_ATTR", "STORE_ATTR_ANAMORPHIC")
add_synonym("STORE_SUBSCR", "STORE_SUBSCR_ANAMORPHIC")
