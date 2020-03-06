#!/usr/bin/env python3
"""ctypes support module"""

from weakref import ref

from _builtins import (
    _builtin,
    _bytes_check,
    _int_check,
    _str_check,
    _str_len,
    _tuple_check,
    _tuple_len,
    _unimplemented,
    _weakref_check,
)


_SIMPLE_TYPE_CHARS = "cbBhHiIlLdfuzZqQPXOv?g"

FUNCFLAG_STDCALL = 0x0
FUNCFLAG_CDECL = 0x1
FUNCFLAG_HRESULT = 0x2
FUNCFLAG_PYTHONAPI = 0x4
FUNCFLAG_USE_ERRNO = 0x8
FUNCFLAG_USE_LASTERROR = 0x10


__version__ = "1.1.0"


# Metaclasses

# Note: CPython does not define _CDataType as a metaclass, but rather as a
# set of methods which are added to the other metaclasses.
class _CDataType(type):
    def from_address(cls, *args, **kwargs):
        _unimplemented()

    def from_buffer(cls, *args, **kwargs):
        _unimplemented()

    def from_buffer_copy(cls, *args, **kwargs):
        _unimplemented()

    def from_param(cls, *args, **kwargs):
        _unimplemented()

    def in_dll(cls, *args, **kwargs):
        _unimplemented()

    def __mul__(cls, length):
        if length < 0:
            raise ValueError(f"Array length must be >= 0, not {length}")
        if not isinstance(cls, type):
            raise TypeError(f"Expected a type object")

        key = (cls, length)
        result = _array_from_ctype_cache.get(key)
        if result is not None:
            return result() if _weakref_check(result) else result

        name = f"{cls.__name__}_Array_{length}"
        result = PyCArrayType(name, (Array,), {"_length_": length, "_type_": cls})
        _array_from_ctype_cache[key] = ref(
            result, lambda _: _array_from_ctype_cache.pop(key)
        )
        return result


class PyCArrayType(_CDataType):
    pass


class PyCFuncPtrType(_CDataType):
    pass


class PyCPointerType(_CDataType):
    def from_param(cls, *args, **kwargs):  # noqa: B902
        _unimplemented()

    def set_type(cls, *args, **kwargs):  # noqa: B902
        _unimplemented()


class PyCSimpleType(_CDataType):
    def __new__(cls, name, bases, namespace):
        result = super().__new__(cls, name, bases, namespace)

        # The base class (_SimpleCData), no checking necessary
        if bases[0] == _CData:
            return result

        if not hasattr(result, "_type_"):
            raise AttributeError("class must define a '_type_' attribute")

        proto = result._type_
        if not _str_check(proto):
            raise TypeError("class must define a '_type_' string attribute")

        if _str_len(proto) != 1:
            raise ValueError(
                "class must define a '_type_' attribute which must be a string \
of length 1"
            )

        if proto not in _SIMPLE_TYPE_CHARS:
            raise AttributeError(
                "class must define a '_type_' attribute which must be\n"
                + f"a single character string containing one of '{_SIMPLE_TYPE_CHARS}'."
            )

        # TODO(T61319024): CPython does more work here

        return result

    def from_param(cls, *args, **kwargs):  # noqa: B902
        _unimplemented()


class PyCStructType(_CDataType):
    def __setattr__(self, name, value):
        _unimplemented()


class UnionType(_CDataType):
    def __setattr__(self, name, value):
        _unimplemented()


# End Metaclasses


class _CData:
    def __ctypes_from_outparam__(self):
        _unimplemented()

    def __reduce__(self):
        _unimplemented()

    def __setstate__(self):
        _unimplemented()


class _Pointer(_CData, metaclass=PyCPointerType):
    def __new__(cls, name, bases, namespace):
        _unimplemented()


class _SimpleCData(_CData, metaclass=PyCSimpleType):
    def __new__(cls, name, bases, namespace):
        _unimplemented()


class ArgumentError(Exception):
    pass


class Array(_CData, metaclass=PyCArrayType):
    def __len__(self):
        _unimplemented()

    def __getitem__(self):
        _unimplemented()

    def __setitem__(self):
        _unimplemented()

    def __delitem__(self):
        _unimplemented()


class CFuncPtr(_CData, metaclass=PyCFuncPtrType):
    def __new__(cls, *args, **kwargs):
        if _tuple_len(args) != 1:
            _unimplemented()

        arg = args[0]
        if _int_check(arg):
            result = super().__new__(cls)
            result.fn_pointer = arg
        elif callable(arg):
            result = super().__new__(cls)
            result.callable = arg
        elif _tuple_check(arg):
            name = arg[0]
            if _bytes_check(name):
                _unimplemented()
            if not _str_check(name):
                # Note: integer argument only supported on WIN32
                raise TypeError("function name must be string, bytes object or integer")
            dll = arg[1]
            handle = dll._handle
            if not _int_check(handle):
                raise TypeError(
                    "the _handle attribute of the second argument must be an integer"
                )
            result = super().__new__(cls)
            result.fn_pointer = _shared_object_symbol_address(handle, name)
        else:
            raise TypeError("argument must be callable or integer function address")

        return result


class Structure(_CData, metaclass=PyCStructType):
    def __init__(self, *args, **kwargs):
        _unimplemented()

    def __new__(cls, *args, **kwargs):
        _unimplemented()


class Union(_CData, metaclass=UnionType):
    def __init__(self, *args, **kwargs):
        _unimplemented()

    def __new__(cls, *args, **kwargs):
        _unimplemented()


_array_from_ctype_cache = {}


_pointer_type_cache = {}


def _shared_object_symbol_address(handle, name):
    _builtin()


def addressof(obj):
    _unimplemented()


def alignment(obj_or_type):
    _unimplemented()


def byref(obj, offset=0):
    _unimplemented()


def dlopen(name, flag):
    _builtin()


def get_errno():
    _unimplemented()


def POINTER(cls):
    result = _pointer_type_cache.get(cls)
    if result is not None:
        return result

    if isinstance(cls, str):
        result = PyCPointerType("LP_" + cls, (_Pointer,), {})
        key = id(result)
    elif isinstance(cls, type):
        result = PyCPointerType("LP_" + cls.__name__, (_Pointer,), {"_type_": cls})
        key = cls
    else:
        raise TypeError("must be a ctypes type")

    _pointer_type_cache[key] = result
    return result


def pointer(obj):
    _unimplemented()


def resize(obj, size):
    _unimplemented()


def set_errno(value):
    _unimplemented()


def sizeof(obj_or_type):
    _builtin()
