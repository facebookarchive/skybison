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

import _struct
from builtins import code as CodeType

from _builtins import _builtin, _byteslike_check, _type, _unimplemented


def loads(bytes):
    _builtin()


TYPE_NULL = b"0"
TYPE_NONE = b"N"
TYPE_FALSE = b"F"
TYPE_TRUE = b"T"
TYPE_STOPITER = b"S"
TYPE_ELLIPSIS = b"."
TYPE_INT = b"i"
TYPE_FLOAT = b"f"
TYPE_BINARY_FLOAT = b"g"
TYPE_COMPLEX = b"x"
TYPE_BINARY_COMPLEX = b"y"
TYPE_LONG = b"l"
TYPE_STRING = b"s"
TYPE_INTERNED = b"t"
TYPE_STRINGREF = b"R"
TYPE_TUPLE = b"("
TYPE_LIST = b"["
TYPE_DICT = b"{"
TYPE_CODE = b"c"
TYPE_UNICODE = b"u"
TYPE_UNKNOWN = b"?"
TYPE_SET = b"<"
TYPE_FROZENSET = b">"
TYPE_REF = b"r"

_INT32_MIN = -0x7FFFFFFF - 1
_INT32_MAX = 0x7FFFFFFF


class _Marshaller:
    def __init__(self, writefunc):
        self._write = writefunc

    def dump(self, x):  # noqa: C901
        ty = _type(x)
        if x is None:
            self._write(TYPE_NONE)
        elif x is StopIteration:
            self._write(TYPE_STOPITER)
        elif x is ...:
            self._write(TYPE_ELLIPSIS)
        elif x is False:
            self._write(TYPE_FALSE)
        elif x is True:
            self._write(TYPE_TRUE)
        elif ty is int:
            self.dump_int(x)
        elif ty is float:
            self.dump_float(x)
        elif ty is complex:
            self.dump_complex(x)
        elif ty is str:
            self.dump_str(x)
        elif _byteslike_check(x):
            self.dump_bytes(x)
        elif ty is tuple:
            self.dump_tuple(x)
        elif ty is list:
            self.dump_list(x)
        elif ty is dict:
            self.dump_dict(x)
        elif ty is set:
            self.dump_set(x)
        elif ty is frozenset:
            self.dump_frozenset(x)
        elif ty is CodeType:
            self.dump_code(x)
        else:
            raise ValueError("unmarshallable object")

    def w_long(self, x):
        self._write(x.to_bytes(4, "little", signed=True))

    def w_short(self, x):
        self._write(x.to_bytes(2, "little", signed=True))

    def dump_int(self, x):
        if _INT32_MIN <= x <= _INT32_MAX:
            self._write(TYPE_INT)
            self.w_long(x)
            return
        self._write(TYPE_LONG)
        sign = 1
        if x < 0:
            sign = -1
            x = -x
        digits = []
        while x:
            digits.append(x & 0x7FFF)
            x >>= 15
        self.w_long(len(digits) * sign)
        for d in digits:
            self.w_short(d)

    def dump_float(self, x):
        write = self._write
        write(TYPE_BINARY_FLOAT)
        write(_struct.pack("d", x))

    def dump_complex(self, x):
        write = self._write
        write(TYPE_BINARY_COMPLEX)
        write(_struct.pack("d", x.real))
        write(_struct.pack("d", x.imag))

    def dump_bytes(self, x):
        self._write(TYPE_STRING)
        self.w_long(len(x))
        self._write(x)

    def dump_str(self, x):
        # TODO(T63932056): Check if a string is interned and emit TYPE_INTERNED
        # or TYPE_STRINGREF
        self._write(TYPE_UNICODE)
        s = x.encode("utf8")
        self.w_long(len(s))
        self._write(s)

    def dump_tuple(self, x):
        self._write(TYPE_TUPLE)
        self.w_long(len(x))
        for item in x:
            self.dump(item)

    def dump_list(self, x):
        self._write(TYPE_LIST)
        self.w_long(len(x))
        for item in x:
            self.dump(item)

    def dump_dict(self, x):
        self._write(TYPE_DICT)
        for key, value in x.items():
            self.dump(key)
            self.dump(value)
        self._write(TYPE_NULL)

    def dump_code(self, x):
        self._write(TYPE_CODE)
        self.w_long(x.co_argcount)
        self.w_long(x.co_kwonlyargcount)
        self.w_long(x.co_nlocals)
        self.w_long(x.co_stacksize)
        self.w_long(x.co_flags)
        self.dump(x.co_code)
        self.dump(x.co_consts)
        self.dump(x.co_names)
        self.dump(x.co_varnames)
        self.dump(x.co_freevars)
        self.dump(x.co_cellvars)
        self.dump(x.co_filename)
        self.dump(x.co_name)
        self.w_long(x.co_firstlineno)
        self.dump(x.co_lnotab)

    def dump_set(self, x):
        self._write(TYPE_SET)
        self.w_long(len(x))
        for each in x:
            self.dump(each)

    def dump_frozenset(self, x):
        self._write(TYPE_FROZENSET)
        self.w_long(len(x))
        for each in x:
            self.dump(each)


version = 2


def dumps(x, version=version):
    if version != 2:
        # TODO(T63932405): Support marshal versions other than 2
        _unimplemented()
    buffer = bytearray()
    m = _Marshaller(buffer.extend)
    m.dump(x)
    return bytes(buffer)
