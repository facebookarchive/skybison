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


@_patch
def _read_null():
    pass


@_patch
def _sigabrt():
    pass


@_patch
def _sigfpe():
    pass


@_patch
def _sigsegv(release_gil=False):
    pass


def _stack_overflow():
    _unimplemented()


def cancel_dump_traceback_later():
    _unimplemented()


@_patch
def disable():
    pass


@_patch
def dump_traceback(file=sys.stderr, all_threads=True):
    pass


def dump_traceback_later(timeout, repeat=False, file=sys.stderr, exit=False):
    _unimplemented()


@_patch
def enable(file=sys.stderr, all_threads=True):
    pass


@_patch
def is_enabled():
    pass


def register(signum, file=sys.stderr, all_threads=True, chain=False):
    _unimplemented()


def unregister(signum):
    _unimplemented()
