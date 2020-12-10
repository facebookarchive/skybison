#!/usr/bin/env python3
# $builtin-init-module$
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

import _io
import _struct
from builtins import code as CodeType

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
FLAG_REF = 0x80

_INT32_MAX = 0x7FFFFFFF
_INT32_MIN = -0x7FFFFFFF - 1

SHALLOW_TYPES = (int, float, complex, str)
version = 2

# This class contains all the information we need to track references when
# dumping an object. The reference count is a dictionary that holds a mapping
# between object identifiers and the number of times such object has been
# referenced. The reference table is another dictionary that maps object ids
# to reference addresses. The indirect reference table is a list that allows
# us to obtain an object identifier without using `id` calls (we avoid using
# it since it pins an object for the duratio nof the runtime): objects are
# appended to the array and it is linearly scanned to obtain the position of
# the object in such array; that position becomes the identifier for the
# specified object which will be used for reference counting and addressing.
class ReferenceData:
    def __init__(self):
        self.reference_count = {}
        self.reference_table = {}
        self.indirect_reference_table = []


def _get_indirect_ref(obj, indirect_reference_table):
    for i in range(len(indirect_reference_table)):
        if indirect_reference_table[i] is obj:
            return i

    indirect_reference_table.append(obj)
    return len(indirect_reference_table) - 1


def _incref(obj, reference_count, indirect_reference_table):
    indirect_id = _get_indirect_ref(obj, indirect_reference_table)
    reference_count[indirect_id] = reference_count.get(indirect_id, 0) + 1


def _count_references(obj, reference_data):
    _incref(
        obj,
        reference_data.reference_count,
        reference_data.indirect_reference_table,
    )
    if obj is None or obj is StopIteration or obj is ... or obj is True or obj is False:
        return True

    ty = _type(obj)

    if ty in SHALLOW_TYPES:
        return True

    if _byteslike_check(obj):
        return True

    if ty is tuple:
        _count_references_tuple(obj, reference_data)
        return True
    if ty is list:
        _count_references_list(obj, reference_data)
        return True
    if ty is dict:
        _count_references_dict(obj, reference_data)
        return True
    if ty is set:
        _count_references_set(obj, reference_data)
        return True
    if ty is frozenset:
        _count_references_frozenset(obj, reference_data)
        return True
    if ty is CodeType:
        _count_references_code(obj, reference_data)
        return True

    raise ValueError("unmarshallable object")


def _count_references_tuple(obj, reference_data):
    for item in obj:
        _count_references(item, reference_data)


def _count_references_list(obj, reference_data):
    for item in obj:
        _count_references(item, reference_data)


def _count_references_dict(obj, reference_data):
    for key, value in obj.items():
        _count_references(key, reference_data)
        _count_references(value, reference_data)


def _count_references_set(obj, reference_data):
    for item in obj:
        _count_references(item, reference_data)


def _count_references_frozenset(obj, reference_data):
    for item in obj:
        _count_references(item, reference_data)


def _count_references_code(obj, reference_data):
    _count_references(obj.co_code, reference_data)
    _count_references(obj.co_consts, reference_data)
    _count_references(obj.co_names, reference_data)
    _count_references(obj.co_varnames, reference_data)
    _count_references(obj.co_freevars, reference_data)
    _count_references(obj.co_cellvars, reference_data)
    _count_references(obj.co_filename, reference_data)
    _count_references(obj.co_name, reference_data)
    _count_references(obj.co_lnotab, reference_data)


def _apply_flag(type, flag):
    return bytes((type[0] | flag,))


def _dump(obj, f, version, reference_data):  # noqa: C901
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

    flag = 0
    if version >= 3:
        obj_id = _get_indirect_ref(obj, reference_data.indirect_reference_table)
        result = _dump_ref(obj_id, f, version, reference_data)
        if result:
            return result

        num_references = reference_data.reference_count.get(obj_id, 0)
        if num_references > 1:
            ref_address = len(reference_data.reference_table.keys())
            if ref_address >= 0x7FFFFFFF:
                raise ValueError("Out of addresses...")
            reference_data.reference_table[obj_id] = ref_address
            flag = FLAG_REF

    ty = _type(obj)
    if ty is int:
        return _dump_int(obj, f, flag)
    if ty is float:
        return _dump_float(obj, f, flag)
    if ty is complex:
        return _dump_complex(obj, f, flag)
    if ty is str:
        return _dump_str(obj, f, flag)
    if _byteslike_check(obj):
        return _dump_bytes(obj, f, flag)
    if ty is tuple:
        return _dump_tuple(obj, f, version, flag, reference_data)
    if ty is list:
        return _dump_list(obj, f, version, flag, reference_data)
    if ty is dict:
        return _dump_dict(obj, f, version, flag, reference_data)
    if ty is set:
        return _dump_set(obj, f, version, flag, reference_data)
    if ty is frozenset:
        return _dump_frozenset(obj, f, version, flag, reference_data)
    if ty is CodeType:
        return _dump_code(obj, f, version, flag, reference_data)
    raise ValueError("unmarshallable object")


def _dump_ref(obj_id, f, version, reference_data):
    ref = reference_data.reference_table.get(obj_id, None)
    if ref is None:
        return 0

    result = f.write(TYPE_REF)
    result += _w_long(reference_data.reference_table[obj_id], f)
    return result


def _dump_int(obj, f, flag):
    result = 0
    if _INT32_MIN <= obj <= _INT32_MAX:
        result += f.write(_apply_flag(TYPE_INT, flag))
        result += _w_long(obj, f)
        return result
    result += f.write(_apply_flag(TYPE_LONG, flag))
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


def _dump_float(obj, f, flag):
    result = f.write(_apply_flag(TYPE_BINARY_FLOAT, flag))
    result += f.write(_struct.pack("d", obj))
    return result


def _dump_complex(obj, f, flag):
    result = f.write(_apply_flag(TYPE_BINARY_COMPLEX, flag))
    result += f.write(_struct.pack("d", obj.real))
    result += f.write(_struct.pack("d", obj.imag))
    return result


def _dump_bytes(obj, f, flag):
    result = f.write(_apply_flag(TYPE_STRING, flag))
    result += _w_long(len(obj), f)
    result += f.write(obj)
    return result


def _dump_str(obj, f, flag):
    # TODO(T63932056): Check if a string is interned and emit TYPE_INTERNED
    # or TYPE_STRINGREF
    result = f.write(_apply_flag(TYPE_UNICODE, flag))
    s = obj.encode("utf8", "surrogatepass")
    result += _w_long(len(s), f)
    result += f.write(s)
    return result


def _dump_tuple(obj, f, version, flag, reference_data):
    result = f.write(_apply_flag(TYPE_TUPLE, flag))
    result += _w_long(len(obj), f)
    for item in obj:
        result += _dump(item, f, version, reference_data)
    return result


def _dump_list(obj, f, version, flag, reference_data):
    result = f.write(_apply_flag(TYPE_LIST, flag))
    result += _w_long(len(obj), f)
    for item in obj:
        result += _dump(item, f, version, reference_data)
    return result


def _dump_dict(obj, f, version, flag, reference_data):
    result = f.write(_apply_flag(TYPE_DICT, flag))
    for key, value in obj.items():
        result += _dump(key, f, version, reference_data)
        result += _dump(value, f, version, reference_data)
    result += f.write(TYPE_NULL)
    return result


def _dump_code(obj, f, version, flag, reference_data):
    result = f.write(_apply_flag(TYPE_CODE, flag))
    result += _w_long(obj.co_argcount, f)
    result += _w_long(obj.co_kwonlyargcount, f)
    result += _w_long(obj.co_nlocals, f)
    result += _w_long(obj.co_stacksize, f)
    result += _w_long(obj.co_flags, f)
    result += _dump(obj.co_code, f, version, reference_data)
    result += _dump(obj.co_consts, f, version, reference_data)
    result += _dump(obj.co_names, f, version, reference_data)
    result += _dump(obj.co_varnames, f, version, reference_data)
    result += _dump(obj.co_freevars, f, version, reference_data)
    result += _dump(obj.co_cellvars, f, version, reference_data)
    result += _dump(obj.co_filename, f, version, reference_data)
    result += _dump(obj.co_name, f, version, reference_data)
    result += _w_long(obj.co_firstlineno, f)
    result += _dump(obj.co_lnotab, f, version, reference_data)
    return result


def _dump_set(obj, f, version, flag, reference_data):
    result = f.write(_apply_flag(TYPE_SET, flag))
    result += _w_long(len(obj), f)
    for item in obj:
        result += _dump(item, f, version, reference_data)
    return result


def _dump_frozenset(obj, f, version, flag, reference_data):
    result = f.write(_apply_flag(TYPE_FROZENSET, flag))
    result += _w_long(len(obj), f)
    for item in obj:
        result += _dump(item, f, version, reference_data)
    return result


def _w_long(obj, f):
    return f.write(obj.to_bytes(4, "little", signed=True))


def _w_short(obj, f):
    return f.write(obj.to_bytes(2, "little", signed=True))


def dump(obj, f, version=version):
    if version not in (2, 3):
        # TODO(T63932405): Support marshal versions other than 2 and 3
        _unimplemented()

    reference_data = ReferenceData()

    if version == 3:
        _count_references(obj, reference_data)

    return _dump(obj, f, version, reference_data)


def dumps(obj, version=version):
    if version not in (2, 3):
        # TODO(T63932405): Support marshal versions other than 2 and 3
        _unimplemented()

    reference_data = ReferenceData()

    if version == 3:
        _count_references(obj, reference_data)

    with _io.BytesIO() as f:
        _dump(obj, f, version, reference_data)
        return f.getvalue()


def load(f):
    _unimplemented()


def loads(bytes):
    _builtin()
