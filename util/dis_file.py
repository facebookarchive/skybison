#!/usr/bin/env python3

import dis
import inspect
import sys
import types


filename = sys.argv[1]
with open(filename) as f:
    source = f.read()
code = compile(source, filename, "exec")


def header(text, width=80):
    print("\n{{:-^{}}}".format(width).format(" {} ".format(text)))


def dump_obj(o):
    for name, value in inspect.getmembers(o):
        print("{}: {}".format(name, value))


SIMPLE_ATTRS = [
    "co_argcount",
    "co_kwonlyargcount",
    "co_nlocals",
    "co_filename",
    "co_firstlineno",
    "co_stacksize",
]

CO_FLAGS = [
    "CO_OPTIMIZED",
    "CO_NEWLOCALS",
    "CO_VARARGS",
    "CO_VARKEYWORDS",
    "CO_NESTED",
    "CO_GENERATOR",
    "CO_NOFREE",
    "CO_COROUTINE",
    "CO_ITERABLE_COROUTINE",
    "CO_ASYNC_GENERATOR",
]


def dump_code(code):
    sub_codes = []

    header("code object '{}'".format(code.co_name))

    for name in SIMPLE_ATTRS:
        print("{:<20}: {}".format(name, getattr(code, name)))

    if code.co_flags != 0:
        header("co_flags", 40)
        attrs = [name for name in CO_FLAGS if code.co_flags & getattr(inspect, name)]
        print(" | ".join(attrs))

    def list_names(name):
        tup = getattr(code, name)
        if len(tup) == 0:
            return
        header(name, 40)
        for i, var in enumerate(tup):
            print("{:3}: {}".format(i, var))

    list_names("co_cellvars")
    list_names("co_freevars")
    list_names("co_names")
    list_names("co_varnames")

    header("co_consts", 40)
    for i, const in enumerate(code.co_consts):
        if isinstance(const, types.CodeType):
            sub_codes.append(const)
        print("{:3}: {:<20} | {}".format(i, str(type(const)), const))

    header("co_code ({} bytes)".format(len(code.co_code)), 40)
    print(dis.Bytecode(code).dis())

    for c in sub_codes:
        dump_code(c)


dump_code(code)
