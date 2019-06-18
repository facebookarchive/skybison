#!/usr/bin/env python3


# These values are injected by our boot process. flake8 has no knowledge about
# their definitions and will complain without these circular assignments.
_patch = _patch  # noqa: F821
_traceback = _traceback  # noqa: F821
_Unbound = _Unbound  # noqa: F821


@_patch
def _bytes_check(obj):
    pass


@_patch
def _int_check(obj):
    pass


@_patch
def _str_check(obj):
    pass


@_patch
def _strarray_iadd(self, other):
    pass


@_patch
def _tuple_check(obj):
    pass


@_patch
def _type(obj):
    pass


@_patch
def _unimplemented():
    """Prints a message and a stacktrace, and stops the program execution."""
    pass
