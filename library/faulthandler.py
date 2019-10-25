#!/usr/bin/env python3

import sys


_patch = _patch  # noqa: F821
_unimplemented = _unimplemented  # noqa: F821


def _fatal_error(message):
    _unimplemented()


def _fatal_error_c_thread():
    _unimplemented()


def _raise_exception(code, flags=0):
    """This function should only be used on Windows."""
    _unimplemented()


def _read_null():
    _unimplemented()


def _sigabrt():
    _unimplemented()


def _sigfpe():
    _unimplemented()


def _sigsegv(release_gil=False):
    _unimplemented()


def _stack_overflow():
    _unimplemented()


def cancel_dump_traceback_later():
    _unimplemented()


def disable():
    _unimplemented()


@_patch
def dump_traceback(file=sys.stderr, all_threads=True):
    pass


def dump_traceback_later(timeout, repeat=False, file=sys.stderr, exit=False):
    _unimplemented()


def enable(file=sys.stderr, all_threads=True):
    _unimplemented()


def is_enabled():
    _unimplemented()


def register(signum, file=sys.stderr, all_threads=True, chain=False):
    _unimplemented()


def unregister(signum):
    _unimplemented()
