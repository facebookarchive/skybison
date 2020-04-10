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
    if obj is None:
        return f.write(TYPE_NONE)
    if obj is StopIteration:
        return f.write(TYPE_STOPITER)
    if obj is ...:
        return f.write(TYPE_ELLIPSIS)
    if obj is False:
        return f.write(TYPE_FALSE)
    if obj is True:
        return f.write(TYPE_TRUE)
    ty = _type(obj)
    if ty is int:
        return _dump_int(obj, f)
    if ty is float:
        return _dump_float(obj, f)
    if ty is complex:
        return _dump_complex(obj, f)
    if ty is str:
        return _dump_str(obj, f)
    if _byteslike_check(obj):
        return _dump_bytes(obj, f)
    if ty is tuple:
        return _dump_tuple(obj, f)
    if ty is list:
        return _dump_list(obj, f)
    if ty is dict:
        return _dump_dict(obj, f)
    if ty is set:
        return _dump_set(obj, f)
    if ty is frozenset:
        return _dump_frozenset(obj, f)
    if ty is CodeType:
        return _dump_code(obj, f)
    raise ValueError("unmarshallable object")


def _dump_int(obj, f):
    result = 0
    if _INT32_MIN <= obj <= _INT32_MAX:
        result += f.write(TYPE_INT)
        result += _w_long(obj, f)
        return result
    result += f.write(TYPE_LONG)
    sign = 1
    if obj < 0:
        sign = -1
        obj = -obj
    digits = []
    while obj:
        digits.append(obj & 0x7FFF)
        obj >>= 15
    result += _w_long(len(digits) * sign, f)
    for d in digits:
        result += _w_short(d, f)
    return result


def _dump_float(obj, f):
    result = f.write(TYPE_BINARY_FLOAT)
    result += f.write(_struct.pack("d", obj))
    return result


def _dump_complex(obj, f):
    result = f.write(TYPE_BINARY_COMPLEX)
    result += f.write(_struct.pack("d", obj.real))
    result += f.write(_struct.pack("d", obj.imag))
    return result


def _dump_bytes(obj, f):
    result = f.write(TYPE_STRING)
    result += _w_long(len(obj), f)
    result += f.write(obj)
    return result


def _dump_str(obj, f):
    # TODO(T63932056): Check if a string is interned and emit TYPE_INTERNED
    # or TYPE_STRINGREF
    result = f.write(TYPE_UNICODE)
    s = obj.encode("utf8", "surrogatepass")
    result += _w_long(len(s), f)
    result += f.write(s)
    return result


def _dump_tuple(obj, f):
    result = f.write(TYPE_TUPLE)
    result += _w_long(len(obj), f)
    for item in obj:
        result += _dump(item, f)
    return result


def _dump_list(obj, f):
    result = f.write(TYPE_LIST)
    result += _w_long(len(obj), f)
    for item in obj:
        result += _dump(item, f)
    return result


def _dump_dict(obj, f):
    result = f.write(TYPE_DICT)
    for key, value in obj.items():
        result += _dump(key, f)
        result += _dump(value, f)
    result += f.write(TYPE_NULL)
    return result


def _dump_code(obj, f):
    result = f.write(TYPE_CODE)
    result += _w_long(obj.co_argcount, f)
    result += _w_long(obj.co_kwonlyargcount, f)
    result += _w_long(obj.co_nlocals, f)
    result += _w_long(obj.co_stacksize, f)
    result += _w_long(obj.co_flags, f)
    result += _dump(obj.co_code, f)
    result += _dump(obj.co_consts, f)
    result += _dump(obj.co_names, f)
    result += _dump(obj.co_varnames, f)
    result += _dump(obj.co_freevars, f)
    result += _dump(obj.co_cellvars, f)
    result += _dump(obj.co_filename, f)
    result += _dump(obj.co_name, f)
    result += _w_long(obj.co_firstlineno, f)
    result += _dump(obj.co_lnotab, f)
    return result


def _dump_set(obj, f):
    result = f.write(TYPE_SET)
    result += _w_long(len(obj), f)
    for item in obj:
        result += _dump(item, f)
    return result


def _dump_frozenset(obj, f):
    result = f.write(TYPE_FROZENSET)
    result += _w_long(len(obj), f)
    for item in obj:
        result += _dump(item, f)
    return result


def _w_long(obj, f):
    return f.write(obj.to_bytes(4, "little", signed=True))


def _w_short(obj, f):
    return f.write(obj.to_bytes(2, "little", signed=True))


def dump(obj, f, version=version):
    if version != 2:
        # TODO(T63932405): Support marshal versions other than 2
        _unimplemented()
    return _dump(obj, f)


def dumps(obj, version=version):
    if version != 2:
        # TODO(T63932405): Support marshal versions other than 2
        _unimplemented()
    with _io.BytesIO() as f:
        _dump(obj, f)
        return f.getvalue()


def load(f):
    _unimplemented()


def loads(bytes):
    _builtin()
