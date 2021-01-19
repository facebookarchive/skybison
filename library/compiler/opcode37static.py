from .opcode37 import opcode as opcode37
from .opcodebase import Opcode

opcode: Opcode = opcode37.copy()
opcode.def_op("INVOKE_METHOD", 162)
opcode.hasconst.add(162)
opcode.def_op("INVOKE_METHOD_CACHED", 163)
opcode.def_op("LOAD_FIELD", 164)
opcode.hasconst.add(164)
opcode.def_op("STORE_FIELD", 165)
opcode.hasconst.add(165)
opcode.name_op("RAISE_IF_NONE", 166)
opcode.def_op("CAST", 167)
opcode.hasconst.add(167)
opcode.def_op("LOAD_LOCAL", 168)
opcode.hasconst.add(168)
opcode.def_op("STORE_LOCAL", 169)
opcode.hasconst.add(169)
opcode.def_op("INT_LOAD_CONST", 170)
opcode.def_op("INT_BOX", 171)
opcode.def_op("CHECK_ARGS", 172)
opcode.hasconst.add(172)
opcode.jabs_op("POP_JUMP_IF_ZERO", 180)
opcode.jabs_op("POP_JUMP_IF_NONZERO", 181)
opcode.def_op("INT_UNBOX", 182)
opcode.def_op("INT_BINARY_OP", 191)
opcode.def_op("INT_UNARY_OP", 192)
opcode.def_op("INT_COMPARE_OP", 193)
opcode.def_op("LOAD_ITERABLE_ARG", 194)
opcode.def_op("LOAD_MAPPING_ARG", 195)
opcode.def_op("INVOKE_FUNCTION", 196)
opcode.def_op("FAST_LEN", 197)
opcode.def_op("CONVERT_PRIMITIVE", 198)
# facebook begin t39538061
opcode.jabs_op("JUMP_IF_ZERO_OR_POP", 184)
opcode.jabs_op("JUMP_IF_NONZERO_OR_POP", 185)
opcode.def_op("ARRAY_GET", 186)
opcode.def_op("ARRAY_SET", 187)
# facebook end t39538061

opcode.stack_effects.update(
    # Static opcodes
    ARRAY_GET=-1,
    ARRAY_SET=-3,
    CAST=0,
    CHECK_ARGS=0,
    CONVERT_PRIMITIVE=0,
    INT_BINARY_OP=lambda oparg, jmp: -1,
    INT_BOX=0,
    INT_COMPARE_OP=lambda oparg, jmp: -1,
    INT_DUP_TOP_TWO=2,
    INT_LOAD_CONST=1,
    INT_UNARY_OP=lambda oparg, jmp: 0,
    INT_UNBOX=0,
    INVOKE_METHOD=lambda oparg, jmp: -oparg[1],
    JUMP_IF_NONZERO_OR_POP=lambda oparg, jmp=0: 0 if jmp else -1,
    JUMP_IF_ZERO_OR_POP=lambda oparg, jmp=0: 0 if jmp else -1,
    LOAD_FIELD=0,
    LOAD_LOCAL=1,
    POP_JUMP_IF_NONZERO=-1,
    POP_JUMP_IF_ZERO=-1,
    RAISE_IF_NONE=0,
    STORE_FIELD=-2,
    STORE_LOCAL=-1,
)
