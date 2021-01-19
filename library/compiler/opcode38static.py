from .opcode38 import opcode as opcode38
from .opcodebase import Opcode

opcode: Opcode = opcode38.copy()
opcode.def_op("INVOKE_METHOD", 164)
opcode.hasconst.add(164)
opcode.def_op("LOAD_FIELD", 165)
opcode.hasconst.add(165)
opcode.def_op("STORE_FIELD", 166)
opcode.hasconst.add(166)
opcode.name_op("RAISE_IF_NONE", 169)
opcode.def_op("CAST", 170)
opcode.hasconst.add(170)
opcode.def_op("LOAD_LOCAL", 171)
opcode.hasconst.add(171)
opcode.def_op("STORE_LOCAL", 172)
opcode.hasconst.add(172)
opcode.def_op("INT_LOAD_CONST", 173)
opcode.def_op("INT_BOX", 174)
opcode.jabs_op("POP_JUMP_IF_ZERO", 175)
opcode.jabs_op("POP_JUMP_IF_NONZERO", 176)
opcode.def_op("INT_UNBOX", 177)
opcode.def_op("INT_BINARY_OP", 178)
opcode.def_op("INT_UNARY_OP", 179)
opcode.def_op("INT_COMPARE_OP", 180)
opcode.def_op("LOAD_ITERABLE_ARG", 181)
opcode.def_op("LOAD_MAPPING_ARG", 182)
opcode.def_op("INVOKE_FUNCTION", 183)
opcode.hasconst.add(183)
opcode.jabs_op("JUMP_IF_ZERO_OR_POP", 184)
opcode.jabs_op("JUMP_IF_NONZERO_OR_POP", 185)
opcode.def_op("FAST_LEN", 186)
opcode.def_op("CONVERT_PRIMITIVE", 187)
opcode.def_op("CHECK_ARGS", 188)
opcode.hasconst.add(188)
opcode.def_op("ARRAY_GET", 189)
opcode.def_op("ARRAY_SET", 190)
opcode.def_op("BUILD_CHECKED_MAP", 191)
opcode.hasconst.add(191)
opcode.def_op("LIST_GET", 192)
opcode.def_op("LIST_SET", 193)
opcode.def_op("LIST_DEL", 194)

opcode.stack_effects.update(  # noqa: C408
    # Static opcodes
    ARRAY_GET=-1,
    ARRAY_SET=-3,
    BUILD_CHECKED_MAP=lambda oparg, jmp: 1 - 2 * oparg[1],
    CAST=0,
    CHECK_ARGS=0,
    CONVERT_PRIMITIVE=0,
    FAST_LEN=0,
    INT_BINARY_OP=lambda oparg, jmp: -1,
    INT_BOX=0,
    INT_COMPARE_OP=lambda oparg, jmp: -1,
    INT_DUP_TOP_TWO=2,
    INT_LOAD_CONST=1,
    INT_UNARY_OP=lambda oparg, jmp: 0,
    INT_UNBOX=0,
    INVOKE_FUNCTION=lambda oparg, jmp=0: (-oparg[1]) + 1,
    INVOKE_METHOD=lambda oparg, jmp: -oparg[1],
    JUMP_IF_NONZERO_OR_POP=lambda oparg, jmp=0: 0 if jmp else -1,
    JUMP_IF_ZERO_OR_POP=lambda oparg, jmp=0: 0 if jmp else -1,
    LIST_DEL=-2,
    LIST_GET=-1,
    LIST_SET=-3,
    LOAD_FIELD=0,
    LOAD_ITERABLE_ARG=1,
    LOAD_LOCAL=1,
    LOAD_MAPPING_ARG=lambda oparg, jmp=0: -1 if oparg == 2 else 1,
    POP_JUMP_IF_NONZERO=-1,
    POP_JUMP_IF_ZERO=-1,
    RAISE_IF_NONE=0,
    STORE_FIELD=-2,
    STORE_LOCAL=-1,
)
