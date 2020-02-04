#!/usr/bin/env python3

import sys

from _builtins import _builtin, _unimplemented


def _fatal_error(message):
    _unimplemented()


def _fatal_error_c_thread():
    _unimplemented()


def _raise_exception(code, flags=0):
    """This function should only be used on Windows."""
    _unimplemented()


def _read_null():
    _builtin()


def _sigabrt():
    _builtin()


def _sigfpe():
    _builtin()


def _sigsegv(release_gil=False):
    _builtin()


def _stack_overflow():
    _unimplemented()


def cancel_dump_traceback_later():
    _unimplemented()


def disable():
    _builtin()


def dump_traceback(file=sys.stderr, all_threads=True):
    _builtin()


def dump_traceback_later(timeout, repeat=False, file=sys.stderr, exit=False):
    _unimplemented()


def enable(file=sys.stderr, all_threads=True):
    _builtin()


def is_enabled():
    _builtin()


def register(signum, file=sys.stderr, all_threads=True, chain=False):
    _unimplemented()


def unregister(signum):
    _unimplemented()
