#!/usr/bin/env python3
"""Array.array module TODO(T55711876): provide an implemenation"""

from builtins import _float, _index, _int
from operator import length_hint

from _builtins import (
    _builtin,
    _bytearray_check,
    _bytes_check,
    _float_check,
    _int_check,
    _list_check,
    _list_getitem,
    _list_len,
    _object_type_hasattr,
    _slice_check,
    _str_check,
    _str_len,
    _tuple_check,
    _tuple_getitem,
    _tuple_len,
    _type,
    _type_subclass_guard,
    _Unbound,
    _unimplemented,
)


def _array_append(obj, val):
    _builtin()


def _array_check(obj):
    _builtin()


def _array_getitem(obj, index):
    _builtin()


def _array_new(cls, typecode, init_len):
    _builtin()


def _array_reserve(self, size):
    _builtin()


def _array_setitem(obj, index, value):
    _builtin()


def _array_value(typecode, value):
    if typecode == "f" or typecode == "d":
        return _float(value)
    elif typecode == "u":
        _unimplemented()
    if _float_check(value):
        raise TypeError("array item must be integer")
    return _int(value)


class array(bootstrap=True):
    def __getitem__(self, key):
        result = _array_getitem(self, key)
        if result is not _Unbound:
            return result
        if _slice_check(key):
            _unimplemented()
        return _array_getitem(self, _index(key))

    def __len__(self):
        _builtin()

    @staticmethod
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

        if typecode == "u":
            _unimplemented()

        result = _array_new(cls, typecode, 0)
        result.extend(initializer)
        return result

    def __setitem__(self, key, value):
        result = _array_setitem(self, key, value)
        if result is not _Unbound:
            return result
        if _slice_check(key):
            _unimplemented()
        return _array_setitem(self, _index(key), _array_value(self.typecode, value))

    def itemsize(self):
        _unimplemented()

    def append(self, value):
        result = _array_append(self, value)
        if result is not _Unbound:
            return result
        return _array_append(self, _array_value(self.typecode, value))

    def buffer_info(self):
        _unimplemented()

    def byteswap(self):
        _unimplemented()

    def count(self, value):
        _unimplemented()

    def extend(self, iterable):
        if _array_check(iterable):
            _unimplemented()
        _array_reserve(self, self.__len__() + length_hint(iterable))
        for item in iterable:
            array.append(self, item)

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
