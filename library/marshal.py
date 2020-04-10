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

import _io
from _builtins import _builtin, _byteslike_check, _type, _unimplemented


TYPE_BINARY_COMPLEX = b"y"
TYPE_BINARY_FLOAT = b"g"
TYPE_CODE = b"c"
TYPE_COMPLEX = b"x"
TYPE_DICT = b"{"
TYPE_ELLIPSIS = b"."
TYPE_FALSE = b"F"
TYPE_FLOAT = b"f"
TYPE_FROZENSET = b">"
TYPE_INT = b"i"
TYPE_INTERNED = b"t"
TYPE_LIST = b"["
TYPE_LONG = b"l"
TYPE_NONE = b"N"
TYPE_NULL = b"0"
TYPE_REF = b"r"
TYPE_SET = b"<"
TYPE_STOPITER = b"S"
TYPE_STRING = b"s"
TYPE_STRINGREF = b"R"
TYPE_TRUE = b"T"
TYPE_TUPLE = b"("
TYPE_UNICODE = b"u"
TYPE_UNKNOWN = b"?"

_INT32_MAX = 0x7FFFFFFF
_INT32_MIN = -0x7FFFFFFF - 1


version = 2


def _dump(obj, f):  # noqa: C901
    ty = _type(obj)
    if obj is None:
        f.write(TYPE_NONE)
    elif obj is StopIteration:
        f.write(TYPE_STOPITER)
    elif obj is ...:
        f.write(TYPE_ELLIPSIS)
    elif obj is False:
        f.write(TYPE_FALSE)
    elif obj is True:
        f.write(TYPE_TRUE)
    elif ty is int:
        _dump_int(obj, f)
    elif ty is float:
        _dump_float(obj, f)
    elif ty is complex:
        _dump_complex(obj, f)
    elif ty is str:
        _dump_str(obj, f)
    elif _byteslike_check(obj):
        _dump_bytes(obj, f)
    elif ty is tuple:
        _dump_tuple(obj, f)
    elif ty is list:
        _dump_list(obj, f)
    elif ty is dict:
        _dump_dict(obj, f)
    elif ty is set:
        _dump_set(obj, f)
    elif ty is frozenset:
        _dump_frozenset(obj, f)
    elif ty is CodeType:
        _dump_code(obj, f)
    else:
        raise ValueError("unmarshallable object")


def _dump_int(obj, f):
    if _INT32_MIN <= obj <= _INT32_MAX:
        f.write(TYPE_INT)
        _w_long(obj, f)
        return
    f.write(TYPE_LONG)
    sign = 1
    if obj < 0:
        sign = -1
        obj = -obj
    digits = []
    while obj:
        digits.append(obj & 0x7FFF)
        obj >>= 15
    _w_long(len(digits) * sign, f)
    for d in digits:
        _w_short(d, f)


def _dump_float(obj, f):
    f.write(TYPE_BINARY_FLOAT)
    f.write(_struct.pack("d", obj))


def _dump_complex(obj, f):
    f.write(TYPE_BINARY_COMPLEX)
    f.write(_struct.pack("d", obj.real))
    f.write(_struct.pack("d", obj.imag))


def _dump_bytes(obj, f):
    f.write(TYPE_STRING)
    _w_long(len(obj), f)
    f.write(obj)


def _dump_str(obj, f):
    # TODO(T63932056): Check if a string is interned and emit TYPE_INTERNED
    # or TYPE_STRINGREF
    f.write(TYPE_UNICODE)
    s = obj.encode("utf8", "surrogatepass")
    _w_long(len(s), f)
    f.write(s)


def _dump_tuple(obj, f):
    f.write(TYPE_TUPLE)
    _w_long(len(obj), f)
    for item in obj:
        _dump(item, f)


def _dump_list(obj, f):
    f.write(TYPE_LIST)
    _w_long(len(obj), f)
    for item in obj:
        _dump(item, f)


def _dump_dict(obj, f):
    f.write(TYPE_DICT)
    for key, value in obj.items():
        _dump(key, f)
        _dump(value, f)
    f.write(TYPE_NULL)


def _dump_code(obj, f):
    f.write(TYPE_CODE)
    _w_long(obj.co_argcount, f)
    _w_long(obj.co_kwonlyargcount, f)
    _w_long(obj.co_nlocals, f)
    _w_long(obj.co_stacksize, f)
    _w_long(obj.co_flags, f)
    _dump(obj.co_code, f)
    _dump(obj.co_consts, f)
    _dump(obj.co_names, f)
    _dump(obj.co_varnames, f)
    _dump(obj.co_freevars, f)
    _dump(obj.co_cellvars, f)
    _dump(obj.co_filename, f)
    _dump(obj.co_name, f)
    _w_long(obj.co_firstlineno, f)
    _dump(obj.co_lnotab, f)


def _dump_set(obj, f):
    f.write(TYPE_SET)
    _w_long(len(obj), f)
    for item in obj:
        _dump(item, f)


def _dump_frozenset(obj, f):
    f.write(TYPE_FROZENSET)
    _w_long(len(obj), f)
    for item in obj:
        _dump(item, f)


def _w_long(obj, f):
    f.write(obj.to_bytes(4, "little", signed=True))


def _w_short(obj, f):
    f.write(obj.to_bytes(2, "little", signed=True))


def dumps(obj, version=version):
    if version != 2:
        # TODO(T63932405): Support marshal versions other than 2
        _unimplemented()
    with _io.BytesIO() as f:
        _dump(obj, f)
        return f.getvalue()


def loads(bytes):
    _builtin()
