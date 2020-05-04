#!/usr/bin/env python3
"""ctypes support module"""

from builtins import _int
from weakref import ref

from _builtins import (
    _builtin,
    _bytes_check,
    _bytes_len,
    _float_check,
    _int_check,
    _memoryview_check,
    _memoryview_start,
    _mmap_check,
    _property,
    _str_check,
    _str_len,
    _tuple_check,
    _tuple_getitem,
    _tuple_len,
    _type,
    _type_check,
    _type_issubclass,
    _Unbound,
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


def _SimpleCData_value_to_type(value, typ, offset):
    _builtin()


# Metaclasses

# Note: CPython does not define _CDataType as a metaclass, but rather as a
# set of methods which are added to the other metaclasses.
class _CDataType(type):
    def from_address(cls, *args, **kwargs):
        _unimplemented()

    def from_buffer(cls, obj, offset=0):
        if not hasattr(cls, "_type_"):
            raise TypeError("abstract class")
        if not _type_issubclass(cls, _SimpleCData) and not _type_issubclass(cls, Array):
            _unimplemented()
        if not _memoryview_check(obj) or not _mmap_check(obj.obj):
            _unimplemented()
        if offset < 0:
            raise ValueError("offset cannot be negative")
        if obj.readonly:
            raise TypeError("underlying buffer is not writable")
        if obj.strides != (1,):
            raise TypeError("underlying buffer is not C contiguous")
        obj_len = obj.shape[0]
        type_size = sizeof(cls)
        if type_size > obj_len - offset:
            raise ValueError(
                "Buffer size too small "
                f"({obj_len} instead of at least {type_size + offset} bytes)"
            )
        result = cls.__new__(cls)
        result._value = obj.obj
        result._offset = offset + _memoryview_start(obj)
        return result

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


def _CharArray_value_to_bytes(obj, le):
    _builtin()


def _CharArray_value_getter(self):
    return _CharArray_value_to_bytes(self._value, self._length_)


def _CharArray_value_setter(self, value):
    if not _bytes_check(value):
        raise TypeError(f"bytes expected instead of {_type(value).__name__} instance")
    val_len = _bytes_len(value)
    if val_len > self._length_:
        raise ValueError("byte string too long")
    if _mmap_check(self._value):
        memoryview(self._value)[:val_len] = value
    else:
        self._value[:val_len] = value
        if val_len < self._length_:
            self._value[val_len] = 0


class PyCArrayType(_CDataType):
    def __new__(cls, name, bases, namespace):
        result = super().__new__(cls, name, bases, namespace)
        # The base class (Array), no checking necessary
        if bases[0] == _CData:
            return result

        length = getattr(result, "_length_", None)
        if not _int_check(length):
            raise AttributeError(
                "class must define a '_length_' attribute, "
                "which must be a positive integer"
            )
        if length < 0:
            raise ValueError("The '_length_' attribute must not be negative")

        if not hasattr(result, "_type_"):
            raise AttributeError("class must define a '_type_' attribute")
        result_type = result._type_
        if not _type_check(result_type) or not hasattr(result_type, "_type_"):
            raise TypeError("_type_ must have storage info")
        if result_type._type_ == "c":
            result.value = _property(_CharArray_value_getter, _CharArray_value_setter)

        return result


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
    pass


class UnionType(_CDataType):
    pass


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
    def __init__(self, value=_Unbound):
        self._value_setter(value)
        self._offset = 0

    def __new__(cls, *args, **kwargs):
        if not hasattr(cls, "_type_"):
            raise TypeError("abstract class")
        return super().__new__(cls, args, kwargs)

    def _value_getter(self):
        return _SimpleCData_value_to_type(self._value, self._type_, self._offset)

    def _value_setter(self, value):
        if self._type_ == "H" or self._type_ == "L":
            if _float_check(value):
                raise TypeError("int expected instead of float")
            if value is _Unbound:
                self._value = 0
            else:
                self._value = _int(value)
        else:
            _unimplemented()

    value = _property(_value_getter, _value_setter)


class ArgumentError(Exception):
    pass


class Array(_CData, metaclass=PyCArrayType):
    def __init__(self, *args, **kwargs):
        self._offset = 0
        if _tuple_len(args) > 1:
            _unimplemented()
        elif _tuple_len(args) == 1:
            if self._type_._type_ != "c":
                _unimplemented()
            arg0 = _tuple_getitem(args, 0)
            if not _bytes_check(arg0) or _bytes_len(arg0) != 1:
                _unimplemented()
            self.value = arg0

    def __len__(self):
        _unimplemented()

    def __getitem__(self):
        _unimplemented()

    def __new__(cls, *args, **kwargs):
        if not hasattr(cls, "_type_"):
            raise TypeError("abstract class")
        result = super().__new__(cls, args, kwargs)
        result._value = bytearray(bytes(cls._length_))
        return result

    def __setitem__(self):
        _unimplemented()

    def __delitem__(self):
        _unimplemented()


class CFuncPtr(_CData, metaclass=PyCFuncPtrType):
    def __call__(self, *args, **kwargs):
        if _tuple_len(args) != 0:
            _unimplemented()
        if hasattr(self, "callable") and self.callable is not None:
            _unimplemented()
        return _call_cfuncptr(self.fn_pointer, self.restype._type_)

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

        result.restype = cls._restype_
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


def _addressof(obj):
    _builtin()


_array_from_ctype_cache = {}


_pointer_type_cache = {}


def _call_cfuncptr(function_ptr, result_type):
    _builtin()


def _sizeof_typeclass(cdata_type):
    _builtin()


def _shared_object_symbol_address(handle, name):
    _builtin()


def addressof(obj):
    if not issubclass(type(obj), _CData):
        raise TypeError("invalid type")
    if (
        not _type_issubclass(_type(obj), _SimpleCData)
        and not _type_issubclass(_type(obj), Array)
    ) or not _mmap_check(obj._value):
        _unimplemented()
    return _addressof(obj._value)


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
    if not _type_check(obj_or_type):
        ty = _type(obj_or_type)
    else:
        ty = obj_or_type
    if not _type_issubclass(ty, _CData):
        raise TypeError("this type has no size")
    if _type_issubclass(ty, Array):
        return ty._length_ * sizeof(ty._type_)
    if _type_issubclass(ty, _SimpleCData):
        return _sizeof_typeclass(ty._type_)
    _unimplemented()
