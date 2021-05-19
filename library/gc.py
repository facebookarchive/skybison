#!/usr/bin/env python3

from _builtins import _builtin, _unimplemented, _gc


def collect():
    _gc()


def disable():
    pass


def enable():
    _unimplemented()


garbage = []


def immortalize_heap():
    _builtin()


def _is_immortal(obj):
    _builtin()


def isenabled():
    return False
