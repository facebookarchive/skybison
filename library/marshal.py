#!/usr/bin/env python3
"""This module contains functions that can read and write Python values in
a binary format. The format is specific to Python, but independent of
machine architecture issues.

Not all Python object types are supported; in general, only objects
whose value is independent from a particular invocation of Python can be
written and read by this module. The following types are supported:
None, integers, floating point numbers, strings, bytes, bytearrays,
tuples, lists, sets, dictionaries, and code objects, where it
should be understood that tuples, lists and dictionaries are only
supported as long as the values contained therein are themselves
supported; and recursive lists and dictionaries should not be written
(they will cause infinite loops).

Variables:

version -- indicates the format that the module uses. Version 0 is the
    historical format, version 1 shares interned strings and version 2
    uses a binary format for floating point numbers.
    Version 3 shares common object references (New in version 3.4).

Functions:

dump() -- write value to a file
load() -- read value from a file
dumps() -- marshal value as a bytes object
loads() -- read value from a bytes-like object"""


# This is the patch decorator, injected by our boot process. flake8 has no
# knowledge about its definition and will complain without this gross circular
# helper here.
_patch = _patch  # noqa: F821


@_patch
def loads(bytes):
    pass
