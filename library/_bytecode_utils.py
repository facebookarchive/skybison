#!/usr/bin/env python3
# $builtin-init-module$


# These values are all injected by our boot process. flake8 has no knowledge
# about their definitions and will complain without these lines.
CO_NEWLOCALS = CO_NEWLOCALS  # noqa: F821
CO_NOFREE = CO_NOFREE  # noqa: F821
CO_OPTIMIZED = CO_OPTIMIZED  # noqa: F821
opmap = opmap  # noqa: F821

EXTENDED_ARG = opmap["EXTENDED_ARG"]
LOAD_CONST = opmap["LOAD_CONST"]
LOAD_FAST = opmap["LOAD_FAST"]
BUILD_TUPLE = opmap["BUILD_TUPLE"]
CALL_FUNCTION = opmap["CALL_FUNCTION"]
RETURN_VALUE = opmap["RETURN_VALUE"]


def _emit_op(bytecode, opcode, arg):
    assert 0 <= arg <= 0xFFFFFFFF
    if arg > 0xFFFFFF:
        bytecode.append(EXTENDED_ARG)
        bytecode.append((arg >> 24) & 0xFF)
    if arg > 0xFFFF:
        bytecode.append(EXTENDED_ARG)
        bytecode.append((arg >> 16) & 0xFF)
    if arg > 0xFF:
        bytecode.append(EXTENDED_ARG)
        bytecode.append((arg >> 8) & 0xFF)
    bytecode.append(opcode)
    bytecode.append(arg & 0xFF)


def _make_dunder_new(modulename, name, arg_list, target_func):
    FunctionType = type(_make_dunder_new)
    CodeType = type(_make_dunder_new.__code__)

    argcount = len(arg_list) + 1
    posonlyargcount = 0
    kwonlyargcount = 0
    nlocals = argcount + 1
    stacksize = nlocals + 1
    flags = CO_OPTIMIZED | CO_NEWLOCALS | CO_NOFREE
    constants = (target_func,)
    names = ()
    varnames = ("_cls", *arg_list)
    filename = ""
    name = "__new__"
    firstlineno = 1
    lnotab = b""
    bytecode = bytearray()
    _emit_op(bytecode, LOAD_CONST, 0)
    for i in range(argcount):
        _emit_op(bytecode, LOAD_FAST, i)
    _emit_op(bytecode, BUILD_TUPLE, argcount - 1)
    _emit_op(bytecode, CALL_FUNCTION, 2)
    _emit_op(bytecode, RETURN_VALUE, 0)
    code = CodeType(
        argcount,
        posonlyargcount,
        kwonlyargcount,
        nlocals,
        stacksize,
        flags,
        bytes(bytecode),
        constants,
        names,
        varnames,
        filename,
        name,
        firstlineno,
        lnotab,
    )
    return FunctionType(code, {}, name)
