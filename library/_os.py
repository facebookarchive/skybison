#!/usr/bin/env python3
"""The _os module provides os facilities very early in the bootstrapping
process. It is used primarily by the _io module before it is possible to bring
up the usual CPython os module."""

from _builtins import _patch


# TODO(emacs): Detect if "posix" or "nt" is in sys.builtin_module_names and set
# linesep accordingly
name = "posix"
linesep = "\n"


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
