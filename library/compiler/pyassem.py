"""A flow graph representation for Python bytecode"""
from __future__ import annotations

import opcode
import sys
from types import CodeType
from typing import Any, List, Optional

from .consts import CO_NEWLOCALS, CO_OPTIMIZED
from .peephole import Optimizer


try:
    import cinder  # pyre-ignore  # noqa: F401

    MAX_BYTECODE_OPT_ITERS = 5
except ImportError:
    MAX_BYTECODE_OPT_ITERS = 1

EXTENDED_ARG = opcode.opname.index("EXTENDED_ARG")


def sign(a):
    if not isinstance(a, float):
        raise TypeError(f"Must be a real number, not {type(a)}")
    if a != a:
        return 1.0  # NaN case
    return 1.0 if str(a)[0] != "-" else -1.0


def instrsize(oparg):
    if oparg <= 0xFF:
        return 1
    elif oparg <= 0xFFFF:
        return 2
    elif oparg <= 0xFFFFFF:
        return 3
    else:
        return 4


def cast_signed_byte_to_unsigned(i):
    if i < 0:
        i = 255 + i + 1
    return i


FVC_MASK = 0x3
FVC_NONE = 0x0
FVC_STR = 0x1
FVC_REPR = 0x2
FVC_ASCII = 0x3
FVS_MASK = 0x4
FVS_HAVE_SPEC = 0x4

OPNAMES = list(opcode.opname)
OPMAP = dict(opcode.opmap)

if "LOAD_METHOD" not in OPMAP:
    OPMAP["LOAD_METHOD"] = 160
    OPNAMES[160] = "LOAD_METHOD"

if "CALL_METHOD" not in OPMAP:
    OPMAP["CALL_METHOD"] = 161
    OPNAMES[161] = "CALL_METHOD"

if "STORE_ANNOTATION" not in OPMAP:
    OPMAP["STORE_ANNOTATION"] = 0
    OPNAMES[0] = "STORE_ANNOTATION"


class Instruction:
    __slots__ = ("opname", "oparg", "target")

    def __init__(self, opname: str, oparg: Any, target: Optional[Block] = None):
        self.opname = opname
        self.oparg = oparg
        self.target = target

    def __repr__(self):
        if self.target:
            return f"Instruction({self.opname!r}, {self.oparg!r}, {self.target!r})"

        return f"Instruction({self.opname!r}, {self.oparg!r})"


class FlowGraph:
    def __init__(self):
        self.block_count = 0
        # List of blocks in the order they should be output for linear
        # code. As we deal with structured code, this order corresponds
        # to the order of source level constructs. (The original
        # implementation from Python2 used a complex ordering algorithm
        # which more buggy and erratic than useful.)
        self.ordered_blocks = []
        # Current block being filled in with instructions.
        self.current = None
        self.entry = Block("entry")
        self.exit = Block("exit")
        self.startBlock(self.entry)

        # Source line number to use for next instruction.
        self.lineno = 0
        # Whether line number was already output (set to False to output
        # new line number).
        self.lineno_set = False
        # First line of this code block. This field is expected to be set
        # externally and serve as a reference for generating all other
        # line numbers in the code block. (If it's not set, it will be
        # deduced).
        self.firstline = 0
        # Line number of first instruction output. Used to deduce .firstline
        # if it's not set explicitly.
        self.first_inst_lineno = 0

    def startBlock(self, block):
        if self._debug:
            if self.current:
                print("end", repr(self.current))
                print("    next", self.current.next)
                print("    prev", self.current.prev)
                print("   ", self.current.get_children())
            print(repr(block))
        block.bid = self.block_count
        self.block_count += 1
        self.current = block
        if block and block not in self.ordered_blocks:
            if block is self.exit:
                if self.ordered_blocks and self.ordered_blocks[-1].has_return():
                    return
            self.ordered_blocks.append(block)

    def nextBlock(self, block=None, label=""):
        # XXX think we need to specify when there is implicit transfer
        # from one block to the next.  might be better to represent this
        # with explicit JUMP_ABSOLUTE instructions that are optimized
        # out when they are unnecessary.
        #
        # I think this strategy works: each block has a child
        # designated as "next" which is returned as the last of the
        # children.  because the nodes in a graph are emitted in
        # reverse post order, the "next" block will always be emitted
        # immediately after its parent.
        # Worry: maintaining this invariant could be tricky
        if block is None:
            block = self.newBlock(label=label)

        # Note: If the current block ends with an unconditional control
        # transfer, then it is techically incorrect to add an implicit
        # transfer to the block graph. Doing so results in code generation
        # for unreachable blocks.  That doesn't appear to be very common
        # with Python code and since the built-in compiler doesn't optimize
        # it out we don't either.
        self.current.addNext(block)
        self.startBlock(block)

    def newBlock(self, label=""):
        b = Block(label)
        return b

    def startExitBlock(self):
        self.nextBlock(self.exit)

    _debug = 0

    def _enable_debug(self):
        self._debug = 1

    def _disable_debug(self):
        self._debug = 0

    def emit(self, opcode, oparg=0):
        if not self.lineno_set and self.lineno:
            self.lineno_set = True
            self.emit("SET_LINENO", self.lineno)

        if self._debug:
            print("\t", inst)  # noqa: F821

        if opcode != "SET_LINENO" and isinstance(oparg, Block):
            self.current.addOutEdge(oparg)
            self.current.emit(Instruction(opcode, 0, oparg))
            return

        self.current.emit(Instruction(opcode, oparg))

        if opcode == "SET_LINENO" and not self.first_inst_lineno:
            self.first_inst_lineno = oparg

    def getBlocksInOrder(self):
        """Return the blocks in the order they should be output."""
        return self.ordered_blocks

    def getBlocks(self):
        if self.exit not in self.ordered_blocks:
            return self.ordered_blocks + [self.exit]
        return self.ordered_blocks

    def getRoot(self):
        """Return nodes appropriate for use with dominator"""
        return self.entry

    def getContainedGraphs(self):
        graph_list = []
        for b in self.getBlocks():
            graph_list.extend(b.getContainedGraphs())
        return graph_list


class Block:
    def __init__(self, label=""):
        self.insts = []
        self.outEdges = set()
        self.label = label
        self.bid = None
        self.next = None
        self.prev = None
        self.returns = False
        self.offset = 0
        self.seen = False  # visited during stack depth calculation
        self.startdepth = -1

    def __repr__(self):
        data = []
        data.append(f"id={self.bid}")
        if self.next:
            data.append(f"next={self.next.bid}")
        extras = ", ".join(data)
        if self.label:
            return f"<block {self.label} {extras}>"
        else:
            return f"<block {extras}>"

    def __str__(self):
        insts = map(str, self.insts)
        return "<block %s %d:\n%s>" % (self.label, self.bid, "\n".join(insts))

    def emit(self, instr):
        if instr.opname == "RETURN_VALUE":
            self.returns = True

        self.insts.append(instr)

    def getInstructions(self):
        return self.insts

    def addOutEdge(self, block):
        self.outEdges.add(block)

    def addNext(self, block):
        assert self.next is None, next
        self.next = block
        assert block.prev is None, block.prev
        block.prev = self

    _uncond_transfer = (
        "RETURN_VALUE",
        "RAISE_VARARGS",
        "JUMP_ABSOLUTE",
        "JUMP_FORWARD",
        "CONTINUE_LOOP",
    )

    def has_unconditional_transfer(self):
        """Returns True if there is an unconditional transfer to an other block
        at the end of this block. This means there is no risk for the bytecode
        executer to go past this block's bytecode."""
        if self.insts and self.insts[-1][0] in self._uncond_transfer:
            return True

    def has_return(self):
        return self.insts and self.insts[-1].opname == "RETURN_VALUE"

    def get_children(self):
        return list(self.outEdges) + [self.next]

    def get_followers(self):
        """Get the whole list of followers, including the next block."""
        followers = {self.next}
        # Blocks that must be emitted *after* this one, because of
        # bytecode offsets (e.g. relative jumps) pointing to them.
        for inst in self.insts:
            if inst[0] in PyFlowGraph.hasjrel:
                followers.add(inst[1])
        return followers

    def getContainedGraphs(self):
        """Return all graphs contained within this block.

        For example, a MAKE_FUNCTION block will contain a reference to
        the graph for the function body.
        """
        contained = []
        for inst in self.insts:
            if len(inst) == 1:
                continue
            op = inst[1]
            if hasattr(op, "graph"):
                contained.append(op.graph)
        return contained


# flags for code objects

# the FlowGraph is transformed in place; it exists in one of these states
RAW = "RAW"
FLAT = "FLAT"
CONV = "CONV"
DONE = "DONE"

STACK_EFFECTS = dict(  # noqa: C408
    POP_TOP=-1,
    ROT_TWO=0,
    ROT_THREE=0,
    DUP_TOP=1,
    DUP_TOP_TWO=2,
    UNARY_POSITIVE=0,
    UNARY_NEGATIVE=0,
    UNARY_NOT=0,
    UNARY_INVERT=0,
    SET_ADD=-1,
    LIST_APPEND=-1,
    MAP_ADD=-2,
    BINARY_POWER=-1,
    BINARY_MULTIPLY=-1,
    BINARY_MATRIX_MULTIPLY=-1,
    BINARY_MODULO=-1,
    BINARY_ADD=-1,
    BINARY_SUBTRACT=-1,
    BINARY_SUBSCR=-1,
    BINARY_FLOOR_DIVIDE=-1,
    BINARY_TRUE_DIVIDE=-1,
    INPLACE_FLOOR_DIVIDE=-1,
    INPLACE_TRUE_DIVIDE=-1,
    INPLACE_ADD=-1,
    INPLACE_SUBTRACT=-1,
    INPLACE_MULTIPLY=-1,
    INPLACE_MATRIX_MULTIPLY=-1,
    INPLACE_MODULO=-1,
    STORE_SUBSCR=-3,
    DELETE_SUBSCR=-2,
    BINARY_LSHIFT=-1,
    BINARY_RSHIFT=-1,
    BINARY_AND=-1,
    BINARY_XOR=-1,
    BINARY_OR=-1,
    INPLACE_POWER=-1,
    GET_ITER=0,
    PRINT_EXPR=-1,
    LOAD_BUILD_CLASS=1,
    INPLACE_LSHIFT=-1,
    INPLACE_RSHIFT=-1,
    INPLACE_AND=-1,
    INPLACE_XOR=-1,
    INPLACE_OR=-1,
    BREAK_LOOP=0,
    SETUP_WITH=7,
    WITH_CLEANUP_START=1,
    WITH_CLEANUP_FINISH=-1,  # XXX Sometimes more
    RETURN_VALUE=-1,
    IMPORT_STAR=-1,
    SETUP_ANNOTATIONS=0,
    YIELD_VALUE=0,
    YIELD_FROM=-1,
    POP_BLOCK=0,
    POP_EXCEPT=0,  #  -3 except if bad bytecode
    END_FINALLY=-1,  # or -2 or -3 if exception occurred
    STORE_NAME=-1,
    DELETE_NAME=0,
    UNPACK_SEQUENCE=lambda oparg, jmp=0: oparg - 1,
    UNPACK_EX=lambda oparg, jmp=0: (oparg & 0xFF) + (oparg >> 8),
    FOR_ITER=1,  # or -1, at end of iterator
    STORE_ATTR=-2,
    DELETE_ATTR=-1,
    STORE_GLOBAL=-1,
    DELETE_GLOBAL=0,
    LOAD_CONST=1,
    LOAD_NAME=1,
    BUILD_TUPLE=lambda oparg, jmp=0: 1 - oparg,
    BUILD_LIST=lambda oparg, jmp=0: 1 - oparg,
    BUILD_SET=lambda oparg, jmp=0: 1 - oparg,
    BUILD_STRING=lambda oparg, jmp=0: 1 - oparg,
    BUILD_LIST_UNPACK=lambda oparg, jmp=0: 1 - oparg,
    BUILD_TUPLE_UNPACK=lambda oparg, jmp=0: 1 - oparg,
    BUILD_TUPLE_UNPACK_WITH_CALL=lambda oparg, jmp=0: 1 - oparg,
    BUILD_SET_UNPACK=lambda oparg, jmp=0: 1 - oparg,
    BUILD_MAP_UNPACK=lambda oparg, jmp=0: 1 - oparg,
    BUILD_MAP_UNPACK_WITH_CALL=lambda oparg, jmp=0: 1 - oparg,
    BUILD_MAP=lambda oparg, jmp=0: 1 - 2 * oparg,
    BUILD_CONST_KEY_MAP=lambda oparg, jmp=0: -oparg,
    LOAD_ATTR=0,
    COMPARE_OP=-1,
    IMPORT_NAME=-1,
    IMPORT_FROM=1,
    JUMP_FORWARD=0,
    JUMP_IF_TRUE_OR_POP=0,  # -1 if jump not taken
    JUMP_IF_FALSE_OR_POP=0,  # ""
    JUMP_ABSOLUTE=0,
    POP_JUMP_IF_FALSE=-1,
    POP_JUMP_IF_TRUE=-1,
    LOAD_GLOBAL=1,
    CONTINUE_LOOP=0,
    SETUP_LOOP=0,
    # close enough...
    SETUP_EXCEPT=6,
    SETUP_FINALLY=6,  # can push 3 values for the new exception
    # + 3 others for the previous exception state
    LOAD_FAST=1,
    STORE_FAST=-1,
    DELETE_FAST=0,
    STORE_ANNOTATION=-1,
    RAISE_VARARGS=lambda oparg, jmp=0: -oparg,
    CALL_FUNCTION=lambda oparg, jmp=0: -oparg,
    CALL_FUNCTION_KW=lambda oparg, jmp=0: -oparg - 1,
    CALL_FUNCTION_EX=lambda oparg, jmp=0: -1 - ((oparg & 0x01) != 0),
    MAKE_FUNCTION=lambda oparg, jmp=0: -1
    - ((oparg & 0x01) != 0)
    - ((oparg & 0x02) != 0)
    - ((oparg & 0x04) != 0)
    - ((oparg & 0x08) != 0),
    BUILD_SLICE=lambda oparg, jmp=0: -2 if oparg == 3 else -1,
    LOAD_CLOSURE=1,
    LOAD_DEREF=1,
    LOAD_CLASSDEREF=1,
    STORE_DEREF=-1,
    DELETE_DEREF=0,
    GET_AWAITABLE=0,
    SETUP_ASYNC_WITH=6,
    BEFORE_ASYNC_WITH=1,
    GET_AITER=0,
    GET_ANEXT=1,
    GET_YIELD_FROM_ITER=0,
    # If there's a fmt_spec on the stack, we go from 2->1,
    # else 1->1.
    FORMAT_VALUE=lambda oparg, jmp=0: -1 if (oparg & FVS_MASK) == FVS_HAVE_SPEC else 0,
    SET_LINENO=0,
    LOAD_METHOD=1,
    CALL_METHOD=lambda oparg, jmp=0: -oparg - 1,
    EXTENDED_ARG=0,
    LOAD_ITERABLE_ARG=1,
    LOAD_MAPPING_ARG=lambda oparg, jmp=0: -1 if oparg == 2 else 1,
    INVOKE_FUNCTION=lambda oparg, jmp=0: -oparg,
    FAST_LEN=0,
)


class PyFlowGraph(FlowGraph):
    super_init = FlowGraph.__init__
    EFFECTS = STACK_EFFECTS

    def __init__(
        self,
        name,
        filename,
        scope,
        args=(),
        kwonlyargs=(),
        starargs=(),
        optimized=0,
        klass=None,
        peephole_enabled=True,
    ):
        self.super_init()
        self.name = name
        self.filename = filename
        self.scope = scope
        self.docstring = None
        self.args = args
        self.kwonlyargs = kwonlyargs
        self.starargs = starargs
        self.klass = klass
        self.peephole_enabled = peephole_enabled
        if optimized:
            self.flags = CO_OPTIMIZED | CO_NEWLOCALS
        else:
            self.flags = 0
        self.consts = {}
        self.names = []
        # Free variables found by the symbol table scan, including
        # variables used only in nested scopes, are included here.
        self.freevars = []
        self.cellvars = []
        # The closure list is used to track the order of cell
        # variables and free variables in the resulting code object.
        # The offsets used by LOAD_CLOSURE/LOAD_DEREF refer to both
        # kinds of variables.
        self.closure = []
        self.varnames = list(args) + list(kwonlyargs) + list(starargs)
        self.stage = RAW
        self.first_inst_lineno = 0
        self.lineno_set = False
        self.lineno = 0
        # Add any extra consts that were requested to the const pool
        self.extra_consts = []

    def setDocstring(self, doc):
        self.docstring = doc

    def setFlag(self, flag):
        self.flags = self.flags | flag

    def checkFlag(self, flag):
        if self.flags & flag:
            return 1

    def setFreeVars(self, names):
        self.freevars = list(names)

    def setCellVars(self, names):
        self.cellvars = names

    def getCode(self):
        """Get a Python code object"""
        assert self.stage == RAW
        self.computeStackDepth()
        # We need to convert into numeric opargs before flattening so we
        # know the sizes of our opargs
        self.convertArgs()
        assert self.stage == CONV
        self.flattenGraph()
        assert self.stage == FLAT
        self.makeByteCode()
        assert self.stage == DONE
        code = self.newCodeObject()
        return code

    def dump(self, io=None):
        if io:
            save = sys.stdout
            sys.stdout = io
        pc = 0
        for block in self.getBlocks():
            print(repr(block))
            for instr in block.getInstructions():
                opname = instr.opname
                if opname == "SET_LINENO":
                    continue
                if instr.target is None:
                    print("\t", "%3d" % pc, opname, instr.oparg)
                elif instr.target.label:
                    print(
                        "\t",
                        f"{pc:3} {opname} {instr.target.bid} ({instr.target.label})",
                    )
                else:
                    print("\t", f"{pc:3} {opname} {instr.target.bid}")
                pc += 2
        if io:
            sys.stdout = save

    def stackdepth_walk(self, block, depth=0, maxdepth=0):  # noqa: C901
        assert block is not None
        if block.seen or block.startdepth >= depth:
            return maxdepth
        block.seen = True
        block.startdepth = depth
        for instr in block.getInstructions():
            effect = self.EFFECTS.get(instr.opname)
            if effect is None:
                raise ValueError(
                    f"Error, opcode {instr.opname} was not found, please update STACK_EFFECTS"
                )
            if isinstance(effect, int):
                depth += effect
            else:
                depth += effect(instr.oparg)

            assert depth >= 0

            if depth > maxdepth:
                maxdepth = depth

            if instr.target:
                target_depth = depth
                if instr.opname == "FOR_ITER":
                    target_depth = depth - 2
                elif instr.opname in ("SETUP_FINALLY", "SETUP_EXCEPT"):
                    target_depth = depth + 3
                    if target_depth > maxdepth:
                        maxdepth = target_depth
                elif instr.opname in ("JUMP_IF_TRUE_OR_POP", "JUMP_IF_FALSE_OR_POP"):
                    depth -= 1
                maxdepth = self.stackdepth_walk(instr.target, target_depth, maxdepth)
                if instr.opname in ("JUMP_ABSOLUTE", "JUMP_FORWARD"):
                    # Remaining code is dead
                    block.seen = False
                    return maxdepth

        if block.next:
            maxdepth = self.stackdepth_walk(block.next, depth, maxdepth)

        block.seen = False
        return maxdepth

    def computeStackDepth(self):
        """Compute the max stack depth.

        Find the flow path that needs the largest stack.  We assume that
        cycles in the flow graph have no net effect on the stack depth.
        """
        for block in self.getBlocksInOrder():
            # We need to get to the first block which actually has instructions
            if block.getInstructions():
                self.stacksize = self.stackdepth_walk(block)
                break

    def flattenGraph(self):
        """Arrange the blocks in order and resolve jumps"""
        assert self.stage == CONV
        # This is an awful hack that could hurt performance, but
        # on the bright side it should work until we come up
        # with a better solution.
        #
        # The issue is that in the first loop blocksize() is called
        # which calls instrsize() which requires i_oparg be set
        # appropriately. There is a bootstrap problem because
        # i_oparg is calculated in the second loop.
        #
        # So we loop until we stop seeing new EXTENDED_ARGs.
        # The only EXTENDED_ARGs that could be popping up are
        # ones in jump instructions.  So this should converge
        # fairly quickly.
        extended_arg_recompile = True
        while extended_arg_recompile:
            extended_arg_recompile = False
            self.insts = insts = []
            pc = 0
            for b in self.getBlocksInOrder():
                b.offset = pc

                for inst in b.getInstructions():
                    insts.append(inst)
                    if inst.opname != "SET_LINENO":
                        pc += instrsize(inst.oparg)

            pc = 0
            for inst in insts:
                if inst.opname != "SET_LINENO":
                    pc += instrsize(inst.oparg)

                opname = inst.opname
                if opname in self.hasjrel or opname in self.hasjabs:
                    oparg = inst.oparg
                    target = inst.target

                    offset = target.offset
                    if opname in self.hasjrel:
                        offset -= pc

                    offset *= 2
                    if instrsize(oparg) != instrsize(offset):
                        extended_arg_recompile = True

                    assert offset >= 0, "Offset value: %d" % offset
                    inst.oparg = offset
        self.stage = FLAT

    hasjrel = set()
    for i in opcode.hasjrel:
        hasjrel.add(OPNAMES[i])
    hasjabs = set()
    for i in opcode.hasjabs:
        hasjabs.add(OPNAMES[i])

    def convertArgs(self):
        """Convert arguments from symbolic to concrete form"""
        assert self.stage == RAW
        # Docstring is first entry in co_consts for normal functions
        # (Other types of code objects deal with docstrings in different
        # manner, e.g. lambdas and comprehensions don't have docstrings,
        # classes store them as __doc__ attribute.
        if self.name == "<lambda>":
            self.consts[self.get_const_key(None)] = 0
        elif not self.name.startswith("<") and not self.klass:
            if self.docstring is not None:
                self.consts[self.get_const_key(self.docstring)] = 0
            else:
                self.consts[self.get_const_key(None)] = 0
        self.sort_cellvars()

        for b in self.getBlocksInOrder():
            for instr in b.insts:
                conv = self._converters.get(instr.opname)
                if conv:
                    instr.oparg = conv(self, instr.oparg)

        self.stage = CONV

    def sort_cellvars(self):
        self.closure = self.cellvars + self.freevars

    def _lookupName(self, name, list):
        """Return index of name in list, appending if necessary

        This routine uses a list instead of a dictionary, because a
        dictionary can't store two different keys if the keys have the
        same value but different types, e.g. 2 and 2L.  The compiler
        must treat these two separately, so it does an explicit type
        comparison before comparing the values.
        """
        t = type(name)
        for i in range(len(list)):
            if t == type(list[i]) and list[i] == name:  # noqa: E721
                return i
        end = len(list)
        list.append(name)
        return end

    def _convert_LOAD_CONST(self, arg):
        if hasattr(arg, "getCode"):
            arg = arg.getCode()
        key = self.get_const_key(arg)
        res = self.consts.get(key, self)
        if res is self:
            res = self.consts[key] = len(self.consts)
        return res

    def get_const_key(self, value):
        if isinstance(value, float):
            return type(value), value, sign(value)
        elif isinstance(value, complex):
            return type(value), value, sign(value.real), sign(value.imag)
        elif isinstance(value, (tuple, frozenset)):
            return (
                type(value),
                value,
                tuple(self.get_const_key(const) for const in value),
            )

        return type(value), value

    def _convert_LOAD_FAST(self, arg):
        return self._lookupName(arg, self.varnames)

    def _convert_LOAD_LOCAL(self, arg):
        return self._convert_LOAD_CONST(
            (self._lookupName(arg[0], self.varnames), arg[1])
        )

    def _convert_NAME(self, arg):
        return self._lookupName(arg, self.names)

    def _convert_DEREF(self, arg):
        # Sometimes, both cellvars and freevars may contain the same var
        # (e.g., for class' __class__). In this case, prefer freevars.
        if arg in self.freevars:
            return self._lookupName(arg, self.freevars) + len(self.cellvars)
        return self._lookupName(arg, self.closure)

    _cmp = list(opcode.cmp_op)

    # similarly for other opcodes...
    _converters = {
        "LOAD_CONST": _convert_LOAD_CONST,
        "INVOKE_METHOD": _convert_LOAD_CONST,
        "LOAD_FIELD": _convert_LOAD_CONST,
        "STORE_FIELD": _convert_LOAD_CONST,
        "CAST": _convert_LOAD_CONST,
        "CHECK_ARGS": _convert_LOAD_CONST,
        "LOAD_FAST": _convert_LOAD_FAST,
        "STORE_FAST": _convert_LOAD_FAST,
        "DELETE_FAST": _convert_LOAD_FAST,
        "LOAD_LOCAL": _convert_LOAD_LOCAL,
        "STORE_LOCAL": _convert_LOAD_LOCAL,
        "LOAD_NAME": _convert_NAME,
        "LOAD_CLOSURE": lambda self, arg: self._lookupName(arg, self.closure),
        "COMPARE_OP": lambda self, arg: self._cmp.index(arg),
        "LOAD_GLOBAL": _convert_NAME,
        "STORE_GLOBAL": _convert_NAME,
        "DELETE_GLOBAL": _convert_NAME,
        "CONVERT_NAME": _convert_NAME,
        "STORE_NAME": _convert_NAME,
        "STORE_ANNOTATION": _convert_NAME,
        "DELETE_NAME": _convert_NAME,
        "IMPORT_NAME": _convert_NAME,
        "IMPORT_FROM": _convert_NAME,
        "STORE_ATTR": _convert_NAME,
        "LOAD_ATTR": _convert_NAME,
        "DELETE_ATTR": _convert_NAME,
        "LOAD_METHOD": _convert_NAME,
        "RAISE_IF_NONE": _convert_NAME,
        "LOAD_DEREF": _convert_DEREF,
        "STORE_DEREF": _convert_DEREF,
        "DELETE_DEREF": _convert_DEREF,
        "LOAD_CLASSDEREF": _convert_DEREF,
    }

    def makeByteCode(self):
        assert self.stage == FLAT
        self.lnotab = lnotab = LineAddrTable()
        lnotab.setFirstLine(self.firstline or self.first_inst_lineno or 1)

        for t in self.insts:
            if t.opname == "SET_LINENO":
                lnotab.nextLine(t.oparg)
                continue

            oparg = t.oparg
            try:
                assert 0 <= oparg <= 0xFFFFFFFF
                if oparg > 0xFFFFFF:
                    lnotab.addCode(EXTENDED_ARG, (oparg >> 24) & 0xFF)
                if oparg > 0xFFFF:
                    lnotab.addCode(EXTENDED_ARG, (oparg >> 16) & 0xFF)
                if oparg > 0xFF:
                    lnotab.addCode(EXTENDED_ARG, (oparg >> 8) & 0xFF)
                lnotab.addCode(OPMAP[t.opname], oparg & 0xFF)
            except ValueError:
                print(t.opname, oparg)
                print(OPMAP[t.opname], oparg)
                raise
        self.stage = DONE

    def newCodeObject(self):
        assert self.stage == DONE
        if (self.flags & CO_NEWLOCALS) == 0:
            nlocals = 0
        else:
            nlocals = len(self.varnames)

        firstline = self.firstline
        # For module, .firstline is initially not set, and should be first
        # line with actual bytecode instruction (skipping docstring, optimized
        # out instructions, etc.)
        if not firstline:
            firstline = self.first_inst_lineno
        # If no real instruction, fallback to 1
        if not firstline:
            firstline = 1

        consts = self.getConsts()
        code = self.lnotab.getCode()
        lnotab = self.lnotab.getTable()
        if self.peephole_enabled:
            for _i in range(MAX_BYTECODE_OPT_ITERS):
                opt = Optimizer(code, consts, lnotab).optimize()
                if opt is not None:
                    if code == opt.byte_code:
                        break
                    code, consts, lnotab = opt.byte_code, opt.consts, opt.lnotab

        consts = consts + tuple(self.extra_consts)

        return CodeType(
            len(self.args),
            len(self.kwonlyargs),
            nlocals,
            self.stacksize,
            self.flags,
            code,
            consts,
            tuple(self.names),
            tuple(self.varnames),
            self.filename,
            self.name,
            firstline,
            lnotab,
            tuple(self.freevars),
            tuple(self.cellvars),
        )

    def getConsts(self):
        """Return a tuple for the const slot of the code object"""
        # Just return the constant value, removing the type portion.
        return tuple(const[1] for const in self.consts)


STACK_EFFECTS_37 = dict(
    STACK_EFFECTS,
    SETUP_WITH=lambda oparg, jmp=0: 6 if jmp else 1,
    WITH_CLEANUP_START=2,  # or 1, depending on TOS
    WITH_CLEANUP_FINISH=-3,
    POP_EXCEPT=-3,
    END_FINALLY=-6,
    FOR_ITER=lambda oparg, jmp=0: -1 if jmp > 0 else 1,
    JUMP_IF_TRUE_OR_POP=lambda oparg, jmp=0: 0 if jmp else -1,
    JUMP_IF_FALSE_OR_POP=lambda oparg, jmp=0: 0 if jmp else -1,
    SETUP_EXCEPT=lambda oparg, jmp: 6 if jmp else 0,
    SETUP_FINALLY=lambda oparg, jmp: 6 if jmp else 0,
    CALL_METHOD=lambda oparg, jmp: -oparg - 1,
    SETUP_ASYNC_WITH=lambda oparg, jmp: (-1 + 6) if jmp else 0,
    LOAD_METHOD=1,
    INT_LOAD_CONST=1,
    LOAD_LOCAL=1,
    STORE_LOCAL=-1,
    INT_BOX=0,
    INT_UNBOX=0,
    INT_DUP_TOP_TWO=2,
    POP_JUMP_IF_NONZERO=-1,
    POP_JUMP_IF_ZERO=-1,
    INVOKE_METHOD=lambda oparg, jmp: -oparg[1],
    STORE_FIELD=-2,
    LOAD_FIELD=0,
    RAISE_IF_NONE=0,
    CAST=0,
    INT_BINARY_OP=lambda oparg, jmp: -1,
    INT_COMPARE_OP=lambda oparg, jmp: -1,
    INT_UNARY_OP=lambda oparg, jmp: 0,
    JUMP_IF_NONZERO_OR_POP=lambda oparg, jmp=0: 0 if jmp else -1,
    JUMP_IF_ZERO_OR_POP=lambda oparg, jmp=0: 0 if jmp else -1,
    CONVERT_PRIMITIVE=0,
    CHECK_ARGS=0,
)


class PyFlowGraph37(PyFlowGraph):
    EFFECTS = STACK_EFFECTS_37

    def push_block(self, worklist: List[Block], block: Block, depth: int):
        assert (
            block.startdepth < 0 or block.startdepth >= depth
        ), f"{block!r}: {block.startdepth} vs {depth}"
        if block.startdepth < depth:
            block.startdepth = depth
            worklist.append(block)

    def stackdepth_walk(self, block):  # noqa: C901
        maxdepth = 0
        worklist = []
        self.push_block(worklist, block, 0)
        while worklist:
            block = worklist.pop()
            next = block.next
            depth = block.startdepth
            assert depth >= 0

            for instr in block.getInstructions():
                if instr.opname == "SET_LINENO":
                    continue

                effect = self.EFFECTS.get(instr.opname)
                if effect is None:
                    raise ValueError(
                        f"Error, opcode {instr.opname} was not found, please update STACK_EFFECTS"
                    )
                if isinstance(effect, int):
                    delta = effect
                else:
                    delta = effect(instr.oparg, 0)

                new_depth = depth + delta
                if new_depth > maxdepth:
                    maxdepth = new_depth

                assert depth >= 0

                op = OPMAP[instr.opname]
                if op in opcode.hasjabs or op in opcode.hasjrel:
                    if isinstance(effect, int):
                        delta = effect
                    else:
                        delta = effect(instr.oparg, 1)

                    target_depth = depth + delta
                    if target_depth > maxdepth:
                        maxdepth = target_depth

                    assert target_depth >= 0
                    if instr.opname == "CONTINUE_LOOP":
                        # Pops a variable number of values from the stack,
                        # but the target should be already proceeding.
                        assert instr.target.startdepth >= 0
                        assert instr.target.startdepth <= depth
                        # remaining code is dead
                        next = None
                        break

                    self.push_block(worklist, instr.target, target_depth)

                depth = new_depth

                if instr.opname in (
                    "JUMP_ABSOLUTE",
                    "JUMP_FORWARD",
                    "RETURN_VALUE",
                    "RAISE_VARARGS",
                    "BREAK_LOOP",
                ):
                    # Remaining code is dead
                    next = None
                    break

            # TODO(dinoviehland): we could save the delta we came up with here and
            # reapply it on subsequent walks rather than having to walk all of the
            # instructions again.
            if next:
                self.push_block(worklist, next, depth)

        return maxdepth


class LineAddrTable:
    """lnotab

    This class builds the lnotab, which is documented in compile.c.
    Here's a brief recap:

    For each SET_LINENO instruction after the first one, two bytes are
    added to lnotab.  (In some cases, multiple two-byte entries are
    added.)  The first byte is the distance in bytes between the
    instruction for the last SET_LINENO and the current SET_LINENO.
    The second byte is offset in line numbers.  If either offset is
    greater than 255, multiple two-byte entries are added -- see
    compile.c for the delicate details.
    """

    def __init__(self):
        self.code = []
        self.codeOffset = 0
        self.firstline = 0
        self.lastline = 0
        self.lastoff = 0
        self.lnotab = []

    def setFirstLine(self, lineno):
        self.firstline = lineno
        self.lastline = lineno

    def addCode(self, opcode, oparg):
        self.code.append(opcode)
        self.code.append(oparg)
        self.codeOffset += 2

    def nextLine(self, lineno):
        if self.firstline == 0:
            self.firstline = lineno
            self.lastline = lineno
        else:
            # compute deltas
            addr_delta = self.codeOffset - self.lastoff
            line_delta = lineno - self.lastline
            if not addr_delta and not line_delta:
                return

            push = self.lnotab.append
            while addr_delta > 255:
                push(255)
                push(0)
                addr_delta -= 255
            if line_delta < -128 or 127 < line_delta:
                if line_delta < 0:
                    k = -128
                    ncodes = (-line_delta) // 128
                else:
                    k = 127
                    ncodes = line_delta // 127
                line_delta -= ncodes * 127
                push(addr_delta)
                push(cast_signed_byte_to_unsigned(k))
                addr_delta = 0
                for _j in range(ncodes - 1):
                    push(0)
                    push(cast_signed_byte_to_unsigned(k))

            push(addr_delta)
            push(cast_signed_byte_to_unsigned(line_delta))

            self.lastline = lineno
            self.lastoff = self.codeOffset

    def getCode(self):
        return bytes(self.code)

    def getTable(self):
        return bytes(self.lnotab)
