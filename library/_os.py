#!/usr/bin/env python3
"""The _os module provides os facilities very early in the bootstrapping
process. It is used primarily by the _io module before it is possible to bring
up the usual CPython os module."""

from _builtins import _builtin


# TODO(emacs): Detect if "posix" or "nt" is in sys.builtin_module_names and set
# linesep accordingly
name = "posix"
linesep = "\n"
F_OK = 0
X_OK = 1
W_OK = 2
R_OK = 4


def access(path, mode):
    _builtin()


def close(fd):
    _builtin()


def fstat_size(fd):
    """Equivalent to os.stat(fd).st_size, but without having to reflect the
    stat struct into Python."""
    _builtin()


def ftruncate(fd, size):
    _builtin()


def isatty(fd):
    _builtin()


def isdir(fd):
    _builtin()


def lseek(fd, offset, whence):
    _builtin()


def open(fd, flags, mode=0o777, dir_fd=None):
    _builtin()


def parse_mode(mode):
    _builtin()


def read(fd, count):
    _builtin()


def set_noinheritable(fd):
    _builtin()
