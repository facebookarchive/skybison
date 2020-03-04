#!/usr/bin/env python3
"""The _path module provides path facilities very early in the bootstrapping
process. It is used primarily by the sys module before it is possible to bring
up the usual CPython {posix|nt}path module."""

from _builtins import _builtin, _str_guard


def dirname(path):
    _str_guard(path)
    sep = "/"
    i = path.rfind(sep) + 1
    head = path[:i]
    if head and head != sep * len(head):
        head = head.rstrip(sep)
    return head


def isdir(path):
    _builtin()


def isfile(path):
    _builtin()


def join(a, *p):
    _str_guard(a)
    sep = "/"
    path = a
    for b in p:
        _str_guard(b)
        if b.startswith(sep):
            path = b
        elif not path or path.endswith(sep):
            path += b
        else:
            path += sep + b
    return path
