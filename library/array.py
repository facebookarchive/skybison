#!/usr/bin/env python3
# $builtin-init-module$
"""Array.array module TODO(T55711876): provide an implemenation"""

from _builtins import (
    _builtin,
    _str_check,
    _str_len,
    _type,
    _type_subclass_guard,
    _unimplemented,
)


def _array_check(obj):
    _builtin()


def _array_new(cls, typecode, init_len):
    _builtin()


class array(bootstrap=True):
    def __new__(cls, typecode, initializer=None):
        _type_subclass_guard(cls, array)
        if not _str_check(typecode) or _str_len(typecode) != 1:
            raise TypeError(
                "array() argument 1 must be a unicode character, not "
                f"{_type(typecode).__name__}"
            )
        if initializer is None:
            return _array_new(cls, typecode, 0)

        if typecode != "u":
            if _str_check(initializer):
                raise TypeError(
                    "cannot use a str to initialize an array with typecode "
                    f"'{typecode}'"
                )
            if _array_check(initializer) and initializer.typecode == "u":
                raise TypeError(
                    "cannot use a unicode array to initialize an array with "
                    f"typecode '{typecode}'"
                )

        _unimplemented()

    def itemsize(self):
        _unimplemented()

    def append(self, value):
        _unimplemented()

    def buffer_info(self):
        _unimplemented()

    def byteswap(self):
        _unimplemented()

    def count(self, value):
        _unimplemented()

    def extend(self, item):
        _unimplemented()

    def frombytes(self, string_value):
        _unimplemented()

    def fromfile(self, file_object, num_items):
        _unimplemented()

    def fromlist(self, list_in):
        _unimplemented()

    def fromstring(self):
        _unimplemented()

    def fromunicode(self, unicode_str):
        _unimplemented()

    def index(self, value):
        _unimplemented()

    def insert(self, value, position):
        _unimplemented()

    def pop(self, index):
        _unimplemented()

    def remove(self, value):
        _unimplemented()

    def reverse(self):
        _unimplemented()

    def tobytes(self):
        _unimplemented()

    def tofile(self, file_object):
        _unimplemented()

    def tolist(self):
        _unimplemented()

    def tostring(self):
        _unimplemented()

    def tounicode(self):
        _unimplemented()
