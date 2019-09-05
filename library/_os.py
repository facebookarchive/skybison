#!/usr/bin/env python3
"""The _os module provides os facilities very early in the bootstrapping
process. It is used primarily by the _io module before it is possible to bring
up the usual CPython os module."""

# This is the patch decorator, injected by our boot process. flake8 has no
# knowledge about its definition and will complain without this gross circular
# helper here.
_Unbound = _Unbound  # noqa: F821
_bytes_check = _bytes_check  # noqa: F821
_object_type_getattr = _object_type_getattr  # noqa: F821
_patch = _patch  # noqa: F821
_str_check = _str_check  # noqa: F821


@_patch
def close(fd):
    pass


@_patch
def fstat_size(fd):
    """Equivalent to os.stat(fd).st_size, but without having to reflect the
stat struct into Python."""
    pass


@_patch
def ftruncate(fd, size):
    pass


@_patch
def isatty(fd):
    pass


@_patch
def isdir(fd):
    pass


@_patch
def lseek(fd, offset, whence):
    pass


@_patch
def open(fd, flags, mode=0o777, dir_fd=None):
    pass


@_patch
def parse_mode(mode):
    pass


@_patch
def read(fd, count):
    pass


@_patch
def set_noinheritable(fd):
    pass
