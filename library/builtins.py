#!/usr/bin/env python3
# isort:skip_file
"""Built-in classes, functions, and constants."""


def __import__(name, globals=None, locals=None, fromlist=(), level=0):
    """This function is used for builtins.__import__ during early bootstrap
    until importlib is fully initialized."""
    _builtin()  # noqa: F821


# TODO(T61007763): We currently have to split the `from import` statement into
# multiple chunks or the peephole optimizer bails out because
# "the lnotab table is too complex".
from _builtins import (
    _address,
    _anyset_check,
    _bool_check,
    _bool_guard,
    _bound_method,
    _builtin,
    _bytearray_append,
    _bytearray_check,
    _bytearray_clear,
    _bytearray_contains,
    _bytearray_copy,
    _bytearray_delitem,
    _bytearray_delslice,
    _bytearray_getitem,
    _bytearray_getslice,
    _bytearray_guard,
    _bytearray_join,
    _bytearray_len,
    _bytearray_setitem,
    _bytearray_setslice,
    _bytes_check,
    _bytes_contains,
    _bytes_decode,
    _bytes_decode_utf_8,
    _bytes_from_bytes,
    _bytes_from_ints,
    _bytes_getitem,
    _bytes_getslice,
    _bytes_guard,
    _bytes_join,
    _bytes_len,
    _bytes_maketrans,
    _bytes_repeat,
    _bytes_replace,
    _bytes_split,
    _bytes_split_whitespace,
    _byteslike_check,
    _byteslike_count,
    _byteslike_endswith,
    _byteslike_find_byteslike,
    _byteslike_find_int,
    _byteslike_guard,
    _byteslike_rfind_byteslike,
    _byteslike_rfind_int,
    _byteslike_startswith,
)
from _builtins import (
    _caller_function,
    _caller_locals,
    _classmethod,
    _classmethod_isabstract,
    _code_check,
    _code_guard,
    _code_new,
    _code_set_posonlyargcount,
    _compile_flags_mask,
    _complex_check,
    _complex_checkexact,
    _complex_imag,
    _complex_new,
    _complex_real,
    _dict_check,
    _dict_check_exact,
    _dict_get,
    _dict_guard,
    _dict_items_guard,
    _dict_keys_guard,
    _dict_setitem,
    _dict_update,
    _divmod,
    _exec,
)
from _builtins import (
    _float_check,
    _float_check_exact,
    _float_divmod,
    _float_format,
    _float_guard,
    _float_new_from_byteslike,
    _float_new_from_float,
    _float_new_from_str,
    _frozenset_guard,
    _function_annotations,
    _function_closure,
    _function_defaults,
    _function_globals,
    _function_guard,
    _function_kwdefaults,
    _function_lineno,
    _function_new,
    _function_set_annotations,
    _function_set_defaults,
    _function_set_kwdefaults,
    _get_member_byte,
    _get_member_char,
    _get_member_double,
    _get_member_float,
    _get_member_int,
    _get_member_long,
    _get_member_pyobject,
    _get_member_short,
    _get_member_string,
    _get_member_ubyte,
    _get_member_uint,
    _get_member_ulong,
    _get_member_ushort,
)
from _builtins import (
    _instance_delattr,
    _instance_getattr,
    _instance_guard,
    _instance_overflow_dict,
    _instance_setattr,
    _int_check,
    _int_check_exact,
    _int_from_bytes,
    _int_guard,
    _int_new_from_bytearray,
    _int_new_from_bytes,
    _int_new_from_int,
    _int_new_from_str,
    _iter,
    _list_check,
    _list_check_exact,
    _list_delitem,
    _list_delslice,
    _list_extend,
    _list_getitem,
    _list_getslice,
    _list_guard,
    _list_len,
    _list_new,
    _list_setitem,
    _list_setslice,
    _list_sort,
    _list_swap,
    _mappingproxy_guard,
    _mappingproxy_mapping,
    _mappingproxy_set_mapping,
    _memoryview_getitem,
    _memoryview_getslice,
    _memoryview_guard,
    _memoryview_itemsize,
    _memoryview_nbytes,
    _memoryview_setitem,
    _memoryview_setslice,
    _module_dir,
    _module_proxy,
    _module_proxy_check,
    _module_proxy_guard,
    _module_proxy_keys,
    _module_proxy_setitem,
    _module_proxy_values,
    _object_class_set,
    _object_keys,
    _object_type_getattr,
    _object_type_hasattr,
    _os_error_subclass_from_errno,
    _property,
    _property_isabstract,
    _pyobject_offset,
)
from _builtins import (
    _range_check,
    _range_guard,
    _range_len,
    _repr_enter,
    _repr_leave,
    _seq_index,
    _seq_iterable,
    _seq_set_index,
    _seq_set_iterable,
    _set_check,
    _set_guard,
    _set_len,
    _set_member_double,
    _set_member_float,
    _set_member_integral,
    _set_member_pyobject,
    _slice_check,
    _slice_guard,
    _slice_start,
    _slice_start_long,
    _slice_step,
    _slice_step_long,
    _slice_stop,
    _slice_stop_long,
    _staticmethod_isabstract,
    _str_check,
    _str_check_exact,
    _str_count,
    _str_encode,
    _str_endswith,
    _str_escape_non_ascii,
    _str_find,
    _str_from_str,
    _str_getitem,
    _str_getslice,
    _str_guard,
    _str_ischr,
    _str_join,
    _str_len,
    _str_mod_fast_path,
    _str_partition,
    _str_replace,
    _str_rfind,
    _str_rpartition,
    _str_split,
    _str_splitlines,
    _str_startswith,
    _str_translate,
)
from _builtins import (
    _strarray_iadd,
    _structseq_getitem,
    _structseq_new_type,
    _structseq_setitem,
    _super,
    _tuple_check,
    _tuple_check_exact,
    _tuple_getitem,
    _tuple_getslice,
    _tuple_guard,
    _tuple_len,
    _tuple_new,
    _type,
    _type_abstractmethods_del,
    _type_abstractmethods_get,
    _type_abstractmethods_set,
    _type_bases_del,
    _type_bases_get,
    _type_bases_set,
    _type_check,
    _type_check_exact,
    _type_dunder_call,
    _type_guard,
    _type_init,
    _type_issubclass,
    _type_new,
    _type_proxy,
    _type_proxy_check,
    _type_proxy_get,
    _type_proxy_guard,
    _type_proxy_keys,
    _type_proxy_len,
    _type_proxy_values,
    _type_subclass_guard,
    _Unbound,
    _unimplemented,
    _warn,
)


# Begin: Early definitions that are necessary to process the rest of the file:

# This function is called after all builtins modules are initialized to enable
# __import__ to work correctly.
def _init():
    global _codecs
    import _codecs

    global _str_mod_format
    from _str_mod import format as _str_mod_format

    global _sys
    import sys as _sys


# TODO(T59042197): Remove in favor of Python 3.8 parameter syntax
def _positional_only(value):
    def _positional_only_decorator(func):
        _code_set_posonlyargcount(func.__code__, value)
        return func

    return _positional_only_decorator


def __build_class__(func, name, *bases, metaclass=_Unbound, bootstrap=False, **kwargs):
    _builtin()


def _function_dict(self):
    # TODO(T56646836) we should not need to define a custom __dict__.
    _function_guard(self)
    return _instance_overflow_dict(self)


def _function_set_dict(self, rhs):
    _function_guard(self)
    _dict_guard(rhs)
    d = _instance_overflow_dict(self)
    d.clear()
    d.update(rhs)


class function(bootstrap=True):
    __annotations__ = _property(_function_annotations, _function_set_annotations)

    def __call__(self, *args, **kwargs):
        _function_guard(self)
        return self(*args, **kwargs)

    __closure__ = _property(_function_closure)

    __defaults__ = _property(_function_defaults, _function_set_defaults)

    __dict__ = _property(_function_dict, _function_set_dict)

    def __get__(self, instance, owner):
        _builtin()

    __globals__ = _property(_function_globals)

    __kwdefaults__ = _property(_function_kwdefaults, _function_set_kwdefaults)

    def __new__(cls, code, globals_dict, name=None, argdefs=None, closure=None):
        if _module_proxy_check(globals_dict):
            mod = globals_dict.__module_object__
        elif _dict_check(globals_dict):
            _unimplemented()
        return _function_new(cls, code, mod, name, argdefs, closure)

    def __repr__(self):
        _function_guard(self)
        return f"<function {self.__name__} at {_address(self):#x}>"


class classmethod(bootstrap=True):
    __func__ = _unimplemented

    def __get__(self, instance, owner):
        _builtin()

    def __init__(self, fn):
        _builtin()

    __isabstractmethod__ = _property(_classmethod_isabstract)

    def __new__(cls, fn):
        _builtin()


class property(bootstrap=True):
    def __delete__(self, instance):
        _builtin()

    def __get__(self, instance, owner=None):
        _builtin()

    def __init__(self, fget=None, fset=None, fdel=None, doc=None):
        _builtin()

    __isabstractmethod__ = _property(_property_isabstract)

    def __new__(cls, fget=None, fset=None, fdel=None, doc=None):
        _builtin()

    def __set__(self, instance, value):
        _builtin()

    def deleter(self, fn):
        _builtin()

    def getter(self, fn):
        _builtin()

    def setter(self, fn):
        _builtin()


class staticmethod(bootstrap=True):
    def __get__(self, instance, owner):
        _builtin()

    def __init__(self, fn):
        _builtin()

    __isabstractmethod__ = _property(_staticmethod_isabstract)

    def __new__(cls, fn):
        _builtin()


class _traceback(bootstrap=True):
    pass


class type(bootstrap=True):
    __abstractmethods__ = _property(
        _type_abstractmethods_get, _type_abstractmethods_set, _type_abstractmethods_del
    )

    @_property
    def __base__(self):
        _builtin()

    __bases__ = _property(_type_bases_get, _type_bases_set, _type_bases_del)

    @_property
    def __dict__(self):
        return mappingproxy(_type_proxy(self))

    __call__ = _type_dunder_call

    def __delattr_(self, other):
        _unimplemented()

    def __dir__(self):
        _type_guard(self)
        result = set()
        type._merge_class_dict_keys(self, result)
        return list(result)

    def __getattribute__(self, name):
        _builtin()

    def __init__(self, *args, **kwargs):
        if _tuple_len(args) != 1 and _tuple_len(args) != 3:
            raise TypeError("type.__init__() takes 1 or 3 arguments")

    def __instancecheck__(self, obj) -> bool:
        return _isinstance_type(obj, _type(obj), self)

    def __new__(cls, name_or_object, bases=_Unbound, type_dict=_Unbound, **kwargs):
        if cls is type and bases is _Unbound and type_dict is _Unbound:
            # If the first argument is exactly type, and there are no other
            # arguments, then this call returns the type of the argument.
            return _type(name_or_object)
        _type_guard(cls)
        _str_guard(name_or_object)
        _tuple_guard(bases)
        _dict_guard(type_dict)
        instance = _type_new(cls, bases)
        mro = _Unbound if cls is type else tuple(cls.mro(instance))
        new_type = _type_init(instance, name_or_object, type_dict, mro)
        _super(new_type).__init_subclass__(**kwargs)
        return new_type

    @_classmethod
    def __prepare__(self, *args, **kwargs):
        return {}

    def __repr__(self):
        return f"<class '{self.__name__}'>"

    def __setattr__(self, name, value):
        _builtin()

    def __subclasscheck__(self, subclass) -> bool:
        return _issubclass(subclass, self)

    def __subclasses__(self):
        _builtin()

    def _merge_class_dict_keys(self, result):
        result.update(self.__dict__.keys())
        for base in self.__bases__:
            type._merge_class_dict_keys(base, result)

    def mro(self):
        _builtin()


def _object_reduce_getnewargs(self):
    getnewargs_ex = _object_type_getattr(self, "__getnewargs_ex__")
    if getnewargs_ex is not _Unbound:
        newargs = getnewargs_ex()
        _tuple_guard(newargs)
        if _tuple_len(newargs) != 2:
            raise ValueError(
                "__getnewargs_ex__ should return a tuple of "
                f"length 2 not {_tuple_len(newargs)}"
            )
        _tuple_guard(newargs[0])
        _dict_guard(newargs[1])
        return newargs

    getnewargs = _object_type_getattr(self, "__getnewargs__")
    if getnewargs is not _Unbound:
        args = getnewargs()
        _tuple_guard(args)
        return args, None

    return None, None


def _object_reduce_getstate(self, required):
    getstate = _object_type_getattr(self, "__getstate__")
    if getstate is not _Unbound:
        return getstate()
    if _object_type_getattr(self, "__slots__") is not _Unbound:
        # TODO(T64462298): Remove this check. Instances with __slots__ should
        # not have a __dict__.
        state = None
    else:
        state = getattr(self, "__dict__", None)
        if not state:
            # None or empty instance_proxy; return None
            state = None
    slotnames = _object_type_getattr(self, "__slotnames__")
    if slotnames is _Unbound:
        import copyreg

        slotnames = copyreg._slotnames(_type(self))
    if required:
        # TODO(T64462484): If required, check the size of the object, compare
        # it to the base object size, and raise TypeError if unpickleable
        _unimplemented()
    if slotnames is None:
        return state
    num_slots = len(slotnames)
    if num_slots > 0:
        slots = {}
        for name in slotnames:
            try:
                value = getattr(self, name)
                slots[name] = value
            except AttributeError:
                pass
            if num_slots != len(slotnames):
                raise RuntimeError("__slotnames__ changed size during iteration")
        if slots:
            state = (state, slots)

    return state


def _object_reduce_newobj(self):
    args, kwargs = _object_reduce_getnewargs(self)
    newobj = None
    newargs = None
    if not kwargs:
        import copyreg

        newobj = copyreg.__newobj__
        newargs = (self.__class__, *args) if args else (self.__class__,)
    elif args is not None:
        import copyreg

        newobj = copyreg.__newobj_ex__
        newargs = (self.__class__, args, kwargs)
    else:
        raise ValueError("args is none and kwargs is not")

    required = args is not None and not _list_check(self) and not _dict_check(self)
    state = _object_reduce_getstate(self, required)

    listitems = iter(self) if _list_check(self) else None
    dictitems = iter(self.items()) if _dict_check(self) else None

    return newobj, newargs, state, listitems, dictitems


def _object_reduce(self, proto):
    if proto >= 2:
        return _object_reduce_newobj(self)
    from copyreg import _reduce_ex

    return _reduce_ex(self, proto)


class object(bootstrap=True):  # noqa: E999
    __class__ = _property(_type)

    @__class__.setter  # noqa: F811
    def __class__(self, value):
        _object_class_set(self, value)

    def __dir__(self):
        attrs = _object_keys(self)
        attrs.extend(type.__dir__(self.__class__))
        return attrs

    def __eq__(self, other):
        if self is other:
            return True
        return NotImplemented

    def __format__(self, format_spec: str) -> str:
        if format_spec != "":
            raise TypeError("format_spec must be empty string")
        return str(self)

    def __ge__(self, other):
        return NotImplemented

    # __getattribute__ is defined in C++ code.

    def __gt__(self, other):
        return NotImplemented

    def __hash__(self):
        _builtin()

    def __init__(self, *args, **kwargs):
        _builtin()

    @classmethod
    def __init_subclass__(cls):
        pass

    def __le__(self, other):
        return NotImplemented

    def __lt__(self, other):
        return NotImplemented

    def __ne__(self, other):
        res = _object_type_getattr(self, "__eq__")(other)
        if res is NotImplemented:
            return NotImplemented
        return not res

    def __new__(cls, *args, **kwargs):
        _builtin()

    def __reduce__(self):
        return _object_reduce(self, 0)

    def __reduce_ex__(self, proto):
        try:
            reduce = self.__reduce__
        except BaseException:
            return _object_reduce(self, proto)

        clsreduce = _type(self).__reduce__
        if clsreduce is not object.__reduce__:  # it's been overridden
            return reduce()

        return _object_reduce(self, proto)

    def __repr__(self):
        return f"<{_type(self).__name__} object at {_address(self):#x}>"

    def __setattr__(self, name, value):
        _builtin()

    def __sizeof__(self):
        _builtin()

    def __str__(self):
        dunder_repr = _object_type_getattr(self, "__repr__")
        if dunder_repr is not _Unbound:
            return dunder_repr()
        return object.__repr__(self)

    @classmethod
    def __subclasshook__(cls, *args):
        return NotImplemented


# End: Early definitions


class ArithmeticError(Exception, bootstrap=True):
    pass


class AssertionError(Exception, bootstrap=True):
    pass


class AttributeError(Exception, bootstrap=True):
    pass


class BaseException(bootstrap=True):
    # Some properties of BaseException can be Unbound to indicate that they're
    # nullptr from the C-API's point of view. We want to hide that distinction
    # from managed code, which is done with descriptors created here.
    #
    # This function is only needed during class creation, so it's deleted at
    # the end of the class body to avoid exposing it to user code.
    def _maybe_unbound_property(name, dunder_name, target_type):  # noqa: B902
        def get(instance):
            value = _instance_getattr(instance, dunder_name)
            return None if value is _Unbound else value

        def set(instance, value):
            if value is not None and not isinstance(value, target_type):
                raise TypeError(
                    f"exception {name} must be None or a {target_type.__name__}"
                )
            _instance_setattr(instance, dunder_name, value)

        return property(get, set)

    __cause__ = _maybe_unbound_property("cause", "__cause__", BaseException)

    __context__ = _maybe_unbound_property("context", "__context__", BaseException)

    def __init__(self, *args):
        _builtin()

    def __repr__(self):
        if not isinstance(self, BaseException):
            raise TypeError("not a BaseException object")
        return f"{self.__class__.__name__}{self.args!r}"

    def __reduce__(self):
        _unimplemented()

    def __str__(self):
        if not isinstance(self, BaseException):
            raise TypeError("not a BaseException object")
        if not self.args:
            return ""
        if len(self.args) == 1:
            return str(self.args[0])
        return str(self.args)

    __traceback__ = _maybe_unbound_property("traceback", "__traceback__", _traceback)

    del _maybe_unbound_property


class BlockingIOError(OSError, bootstrap=True):
    pass


class BrokenPipeError(ConnectionError, bootstrap=True):
    pass


class BufferError(Exception, bootstrap=True):
    pass


class BytesWarning(Warning, bootstrap=True):
    pass


class ChildProcessError(OSError, bootstrap=True):
    pass


class ConnectionAbortedError(ConnectionError, bootstrap=True):
    pass


class ConnectionError(OSError, bootstrap=True):
    pass


class ConnectionRefusedError(ConnectionError, bootstrap=True):
    pass


class ConnectionResetError(ConnectionError, bootstrap=True):
    pass


class DeprecationWarning(Warning, bootstrap=True):
    pass


class EOFError(Exception, bootstrap=True):
    pass


Ellipsis = ...


EnvironmentError = OSError


class Exception(BaseException, bootstrap=True):
    pass


class FileExistsError(OSError, bootstrap=True):
    pass


class FileNotFoundError(OSError, bootstrap=True):
    pass


class FloatingPointError(ArithmeticError, bootstrap=True):
    pass


class FutureWarning(Warning, bootstrap=True):
    pass


class GeneratorExit(BaseException, bootstrap=True):
    pass


IOError = OSError


class ImportError(Exception, bootstrap=True):
    def __init__(self, *args, name=None, path=None):
        # TODO(mpage): Call super once we have EX calling working for built-in methods
        self.args = args
        if len(args) == 1:
            self.msg = args[0]
        self.name = name
        self.path = path

    def __reduce__(self):
        _unimplemented()


class ImportWarning(Warning, bootstrap=True):
    pass


class IndentationError(SyntaxError, bootstrap=True):
    pass


class IndexError(LookupError, bootstrap=True):
    pass


class InterruptedError(OSError, bootstrap=True):
    pass


class IsADirectoryError(OSError, bootstrap=True):
    pass


class KeyError(LookupError, bootstrap=True):
    def __str__(self):
        if _tuple_check(self.args) and _tuple_len(self.args) == 1:
            return repr(self.args[0])
        return super(KeyError, self).__str__()


class KeyboardInterrupt(BaseException, bootstrap=True):
    pass


class LookupError(Exception, bootstrap=True):
    pass


class MemoryError(Exception, bootstrap=True):
    pass


class ModuleNotFoundError(ImportError, bootstrap=True):
    pass


class NameError(Exception, bootstrap=True):
    pass


class NoneType(bootstrap=True):
    __class__ = NoneType  # noqa: F821

    def __new__(cls):
        _builtin()

    def __repr__(self):
        _builtin()


class NotADirectoryError(OSError, bootstrap=True):
    pass


class NotImplementedType(bootstrap=True):
    def __repr__(self):
        return "NotImplemented"


class NotImplementedError(RuntimeError, bootstrap=True):
    pass


class OSError(Exception, bootstrap=True):
    def __new__(cls, *args):
        _type_subclass_guard(cls, OSError)
        if cls is OSError and _tuple_len(args) > 1:
            errno = _tuple_getitem(args, 0)
            if _int_check(errno):
                subclass = _os_error_subclass_from_errno(errno)
                if subclass is not OSError:
                    return subclass.__new__(subclass, *args)
        # TODO(T52743795): Use super().
        return Exception.__new__(cls, *args)

    def __init__(self, *args):
        BaseException.__init__(self, *args)
        self.errno = None
        self.strerror = None
        self.filename = None
        self.filename2 = None

        arg_len = _tuple_len(args)
        if arg_len > 2:
            self.errno = _tuple_getitem(args, 0)
            self.strerror = _tuple_getitem(args, 1)
            self.filename = _tuple_getitem(args, 2)

        if arg_len > 4:
            self.filename2 = _tuple_getitem(args, 4)


class OverflowError(ArithmeticError, bootstrap=True):
    pass


class PendingDeprecationWarning(Warning, bootstrap=True):
    pass


class PermissionError(OSError, bootstrap=True):
    pass


class ProcessLookupError(OSError, bootstrap=True):
    pass


class RecursionError(RuntimeError, bootstrap=True):
    pass


class ReferenceError(Exception, bootstrap=True):
    pass


class ResourceWarning(Warning, bootstrap=True):
    pass


class RuntimeError(Exception, bootstrap=True):
    pass


class RuntimeWarning(Warning, bootstrap=True):
    pass


class SimpleNamespace:
    def __init__(self, kwds=None):
        if kwds is not None:
            instance_proxy(self).update(kwds)

    def __repr__(self):
        if _repr_enter(self):
            return "{...}"
        proxy = instance_proxy(self)
        kwpairs = [f"{key}={value!r}" for key, value in proxy.items()]
        _repr_leave(self)
        return "namespace(" + ", ".join(kwpairs) + ")"


class StopAsyncIteration(Exception, bootstrap=True):
    pass


class StopIteration(Exception, bootstrap=True):
    def __init__(self, *args, **kwargs):
        _builtin()


class SyntaxError(Exception, bootstrap=True):
    def __init__(self, *args, **kwargs):
        # TODO(T52743795): Replace with super() when that is fixed
        super(SyntaxError, self).__init__(*args, **kwargs)
        # msg, filename, lineno, offset, text all default to None because of
        # Pyro's automatic None-initialization during allocation.
        args_len = _tuple_len(args)
        if args_len > 0:
            self.msg = args[0]
        if args_len == 2:
            info = tuple(args[1])
            if _tuple_len(info) != 4:
                raise IndexError("tuple index out of range")
            self.filename = info[0]
            self.lineno = info[1]
            self.offset = info[2]
            self.text = info[3]

    def __str__(self):
        have_filename = _str_check(self.filename)
        have_lineno = _int_check_exact(self.lineno)
        # TODO(T52652299): Take basename of filename. See implementation of
        # function my_basename in exceptions.c
        if not have_filename and not have_lineno:
            return str(self.msg)
        if have_filename and have_lineno:
            return f"{self.msg!s} ({self.filename}, line {self.lineno})"
        elif have_filename:
            return f"{self.msg!s} ({self.filename})"
        else:  # have_lineno
            return f"{self.msg!s} (line {self.lineno})"


class SyntaxWarning(Warning, bootstrap=True):
    pass


class SystemError(Exception, bootstrap=True):
    pass


class SystemExit(BaseException, bootstrap=True):
    def __init__(self, *args, **kwargs):
        _builtin()


class TabError(IndentationError, bootstrap=True):
    pass


class TimeoutError(OSError, bootstrap=True):
    pass


class TypeError(Exception, bootstrap=True):
    pass


class UnboundLocalError(NameError, bootstrap=True):
    pass


class UnicodeDecodeError(UnicodeError, bootstrap=True):
    def __init__(self, encoding, obj, start, end, reason):
        super(UnicodeDecodeError, self).__init__(encoding, obj, start, end, reason)
        if not _str_check(encoding):
            raise TypeError(f"argument 1 must be str, not {_type(encoding).__name__}")
        self.encoding = encoding
        self.start = _index(start)
        self.end = _index(end)
        if not _str_check(reason):
            raise TypeError(f"argument 5 must be str, not {_type(reason).__name__}")
        self.reason = reason
        _byteslike_guard(obj)
        self.object = obj


class UnicodeEncodeError(UnicodeError, bootstrap=True):
    def __init__(self, encoding, obj, start, end, reason):
        super(UnicodeEncodeError, self).__init__(encoding, obj, start, end, reason)
        if not _str_check(encoding):
            raise TypeError(f"argument 1 must be str, not {_type(encoding).__name__}")
        self.encoding = encoding
        if not _str_check(obj):
            raise TypeError(f"argument 2 must be str, not '{_type(obj).__name__}'")
        self.object = obj
        self.start = _index(start)
        self.end = _index(end)
        if not _str_check(reason):
            raise TypeError(f"argument 5 must be str, not {_type(reason).__name__}")
        self.reason = reason


class UnicodeError(ValueError, bootstrap=True):  # noqa: B903
    def __init__(self, *args):
        self.args = args


class UnicodeTranslateError(UnicodeError, bootstrap=True):
    def __init__(self, obj, start, end, reason):
        super(UnicodeTranslateError, self).__init__(obj, start, end, reason)
        if not _str_check(obj):
            raise TypeError(f"argument 1 must be str, not {_type(obj).__name__}")
        self.object = obj
        self.start = _index(start)
        self.end = _index(end)
        if not _str_check(reason):
            raise TypeError(f"argument 4 must be str, not {_type(reason).__name__}")
        self.reason = reason


class UnicodeWarning(Warning, bootstrap=True):
    pass


class UserWarning(Warning, bootstrap=True):
    pass


class ValueError(Exception, bootstrap=True):
    pass


class Warning(Exception, bootstrap=True):
    pass


class ZeroDivisionError(ArithmeticError, bootstrap=True):
    pass


def _bytes_new(source) -> bytes:
    # source should be a bytes-like object (fast case), tuple/list of ints (fast case),
    # or an iterable of int-like objects (slow case)
    # patch is not patched because that would cause a circularity problem.
    result = _bytes_from_ints(source)
    if result is not None:
        # source was a list or tuple of ints in range(0, 256)
        return result
    try:
        iterator = iter(source)
    except TypeError:
        raise TypeError(f"cannot convert '{_type(source).__name__}' object to bytes")
    return _bytes_from_ints([_index(x) for x in iterator])


class _descrclassmethod:
    def __init__(self, cls, fn):
        self.cls = cls
        self.fn = fn

    def __get__(self, instance, cls):
        _type_subclass_guard(cls, self.cls)
        return _bound_method(self.fn, cls)

    def __call__(self, *args, **kwargs):
        if not args:
            raise TypeError(f"Function {self.fn} needs at least 1 argument")
        cls = args[0]
        _type_subclass_guard(cls, self.cls)
        return self.fn(*args, **kwargs)


def _dunder_bases_tuple_check(obj, msg) -> None:
    try:
        if not _tuple_check(obj.__bases__):
            raise TypeError(msg)
    except AttributeError:
        raise TypeError(msg)


def _err_program_text(filename, lineno: int) -> str:
    # TODO(T42106571): protect against modified open()
    with open(filename, "rb") as file:
        line = ""
        while lineno > 0:
            line = file.readline()
            if not line:
                break
            lineno -= 1
        return line


def _float(value) -> float:
    # Equivalent to PyFloat_AsDouble() (if it would return a float object).
    if _float_check(value):
        return value
    dunder_float = _object_type_getattr(value, "__float__")
    if dunder_float is _Unbound:
        raise TypeError(f"must be real number, not {_type(value).__name__}")
    result = dunder_float()
    if _float_check(result):
        return result
    raise TypeError(
        f"{_type(value).__name__}.__float__ returned non-float "
        f"(type {_type(result).__name__})"
    )


def _exception_new(mod_name: str, exc_name: str, base, type_dict) -> type:
    _str_guard(mod_name)
    _str_guard(exc_name)
    _dict_guard(type_dict)
    if "__module__" not in type_dict:
        type_dict["__module__"] = mod_name
    if not _tuple_check(base):
        base = (base,)
    return type(exc_name, base, type_dict)


_formatter = None


def _int_format_hexadecimal(value):
    assert value >= 0
    if value == 0:
        return "0"
    result = ""
    while value > 0:
        result += "0123456789abcdef"[value & 15]
        value >>= 4
    return result[::-1]


def _int_format_hexadecimal_upcase(value):
    assert value >= 0
    if value == 0:
        return "0"
    result = ""
    while value > 0:
        result += "0123456789ABCDEF"[value & 15]
        value >>= 4
    return result[::-1]


def _int_format_octal(value):
    assert value >= 0
    if value == 0:
        return "0"
    result = ""
    while value > 0:
        result += "01234567"[value & 7]
        value >>= 3
    return result[::-1]


def _index(obj) -> int:
    # equivalent to PyNumber_Index
    if _int_check(obj):
        return obj
    dunder_index = _object_type_getattr(obj, "__index__")
    if dunder_index is _Unbound:
        raise TypeError(
            f"'{_type(obj).__name__}' object cannot be interpreted as an integer"
        )
    result = dunder_index()
    if _int_check(result):
        return result
    raise TypeError(f"__index__ returned non-int (type {_type(result).__name__})")


def _int(obj) -> int:
    # equivalent to _PyLong_FromNbInt
    if _int_check_exact(obj):
        return obj
    dunder_int = _object_type_getattr(obj, "__int__")
    if dunder_int is _Unbound:
        raise TypeError(f"an integer is required (got type {_type(obj).__name__})")
    result = dunder_int()
    if _int_check_exact(result):
        return result
    if _int_check(result):
        _warn(
            f"__int__ returned non-int (type {_type(result).__name__}).  "
            "The ability to return an instance of a strict subclass of int "
            "is deprecated, and may be removed in a future version of Python.",
            DeprecationWarning,
            stacklevel=1,
        )
        return result
    raise TypeError(f"__int__ returned non-int (type {_type(result).__name__})")


def _isinstance_type(obj, ty: type, cls: type) -> bool:
    if ty is cls or _type_issubclass(ty, cls):
        return True
    subcls = getattr(obj, "__class__", ty)
    return subcls is not ty and _type_check(subcls) and _type_issubclass(subcls, cls)


def _issubclass(subclass, superclass) -> bool:
    if _type_check(subclass) and _type_check(superclass):
        return _type_issubclass(subclass, superclass)
    _dunder_bases_tuple_check(subclass, "issubclass() arg 1 must be a class")
    _dunder_bases_tuple_check(
        superclass, "issubclass() arg 2 must be a class or tuple of classes"
    )
    return _issubclass_recursive(subclass, superclass)


def _issubclass_recursive(subclass, superclass) -> bool:
    if subclass is superclass:
        return True
    bases = subclass.__bases__
    if _tuple_check(bases):
        for base in bases:
            if _issubclass_recursive(base, superclass):
                return True
    return False


def _mapping_check(obj) -> bool:
    # equivalent PyMapping_Check()
    return _object_type_hasattr(obj, "__getitem__")


def _mapping_repr(left, mapping, right) -> str:
    if _repr_enter(mapping):
        return f"{left}...{right}"
    kwpairs = [f"{key!r}: {value!r}" for key, value in mapping.items()]
    _repr_leave(mapping)
    return f"{left}{', '.join(kwpairs)}{right}"


def _new_member_get_bool(offset):
    return lambda instance: bool(_get_member_int(_pyobject_offset(instance, offset)))


def _new_member_get_byte(offset):
    return lambda instance: _get_member_byte(_pyobject_offset(instance, offset))


def _new_member_get_char(offset):
    return lambda instance: _get_member_char(_pyobject_offset(instance, offset))


def _new_member_get_double(offset):
    return lambda instance: _get_member_double(_pyobject_offset(instance, offset))


def _new_member_get_float(offset):
    return lambda instance: _get_member_float(_pyobject_offset(instance, offset))


def _new_member_get_int(offset):
    return lambda instance: _get_member_int(_pyobject_offset(instance, offset))


def _new_member_get_long(offset):
    return lambda instance: _get_member_long(_pyobject_offset(instance, offset))


def _new_member_get_none(offset):
    return lambda instance: None


def _new_member_get_pyobject(offset, name=None):
    return lambda instance: _get_member_pyobject(
        _pyobject_offset(instance, offset), name
    )


def _new_member_get_short(offset):
    return lambda instance: _get_member_short(_pyobject_offset(instance, offset))


def _new_member_get_string(offset):
    return lambda instance: _get_member_string(_pyobject_offset(instance, offset))


def _new_member_get_ubyte(offset):
    return lambda instance: _get_member_ubyte(_pyobject_offset(instance, offset))


def _new_member_get_uint(offset):
    return lambda instance: _get_member_uint(_pyobject_offset(instance, offset))


def _new_member_get_ulong(offset):
    return lambda instance: _get_member_ulong(_pyobject_offset(instance, offset))


def _new_member_get_ushort(offset):
    return lambda instance: _get_member_ushort(_pyobject_offset(instance, offset))


def _new_member_set_bool(offset):
    def setter(instance, value):
        if not _bool_check(value):
            raise TypeError("attribute value type must be bool")
        _set_member_integral(_pyobject_offset(instance, offset), int(value), 4)

    return setter


def _new_member_set_char(offset):
    def setter(instance, value):
        if not _str_check(value):
            raise TypeError("attribute value type must be str")
        if len(value) != 1:
            raise TypeError("attribute str length must be 1")
        _set_member_integral(_pyobject_offset(instance, offset), ord(value), 1)

    return setter


def _new_member_set_double(offset):
    def setter(instance, value):
        if not _float_check(value):
            raise TypeError("attribute value type must be float")
        _set_member_double(_pyobject_offset(instance, offset), value)

    return setter


def _new_member_set_float(offset):
    def setter(instance, value):
        if not _float_check(value):
            raise TypeError("attribute value type must be float")
        _set_member_float(_pyobject_offset(instance, offset), value)

    return setter


def _new_member_set_integral(offset, num_bytes, min_value, max_value, primitive_type):
    def setter(instance, value):
        if not _int_check(value):
            raise TypeError("attribute value type must be int")
        _set_member_integral(_pyobject_offset(instance, offset), value, num_bytes)
        if value < min_value or value > max_value:
            _warn(f"Truncation of value to {primitive_type}")

    return setter


def _new_member_set_pyobject(offset):
    return lambda instance, value: _set_member_pyobject(
        _pyobject_offset(instance, offset), value
    )


def _new_member_set_readonly(name):
    def setter(instance, value):
        raise AttributeError("{name} is a readonly attribute")

    return setter


def _new_member_set_readonly_strings(name):
    def setter(instance, value):
        raise TypeError("{name} is a readonly attribute")

    return setter


def _number_check(obj) -> bool:
    # equivalent to PyNumber_Check()
    return _object_type_hasattr(obj, "__int__") or _object_type_hasattr(
        obj, "__float__"
    )


def _range_getitem(self: range, idx: int) -> int:
    length = _range_len(self)
    if idx < 0:
        idx = length + idx
    if idx < 0 or idx >= length:
        raise IndexError("range object index out of range")
    return self.start + idx * self.step


def _range_getslice(self: range, start: int, stop: int, step: int) -> range:
    new_step = self.step * step
    new_start = self.start + start * self.step
    new_stop = self.start + stop * self.step
    return range(new_start, new_stop, new_step)


def _sequence_repr(left, seq, right) -> str:
    if _repr_enter(seq):
        return f"{left}...{right}"
    result = f"{left}{', '.join(repr(x) for x in seq)}{right}"
    _repr_leave(seq)
    return result


def _slice_index(num) -> int:
    if num is None or _int_check(num):
        return num
    if _object_type_hasattr(num, "__index__"):
        return _index(num)
    raise TypeError(
        "slice indices must be integers or None or have an __index__ method"
    )


def _slice_index_not_none(num) -> int:
    if _int_check(num):
        return num
    if _object_type_hasattr(num, "__index__"):
        return _index(num)
    raise TypeError("slice indices must be integers or have an __index__ method")


class _strarray(bootstrap=True):  # noqa: F821
    def __init__(self, source=_Unbound) -> None:
        _builtin()

    __hash__ = None

    def __new__(cls, source=_Unbound) -> _strarray:  # noqa: F821
        _builtin()

    def __repr__(self) -> str:
        if _type(self) is not _strarray:
            raise TypeError("'__repr__' requires a '_strarray' object")
        return f"_strarray('{self.__str__()}')"

    def __str__(self) -> str:  # noqa: T484
        _builtin()


class _structseq_field:
    def __get__(self, instance, owner):
        if _type(instance) is not self.type:
            if instance is None:
                return self
            raise TypeError(
                f"descriptor for '{self.type.__name__}' doesn't apply to "
                f"'{_type(instance).__name__}' object"
            )
        return _structseq_getitem(instance, self.index)

    def __init__(self, type, index):
        self.type = type
        self.index = index

    def __set__(self, instance, value):
        raise TypeError("readonly attribute")


def _structseq_new(cls, sequence, dict={}):  # noqa B006
    seq_tuple = (*sequence,)
    seq_len = _tuple_len(seq_tuple)
    max_len = cls.n_fields
    min_len = cls.n_sequence_fields
    if seq_len < min_len:
        raise TypeError(
            f"{cls.__name__} needs at least a {min_len}-sequence "
            f"({seq_len}-sequence given)"
        )
    if seq_len > max_len:
        raise TypeError(
            f"{cls.__name__} needs at most a {max_len}-sequence "
            f"({seq_len}-sequence given)"
        )

    # `cls` is known to be a subclass of tuple; use tuple constructor.
    structseq = _tuple_new(cls, seq_tuple[:min_len])

    # Fill the rest of the hidden fields
    for i in range(min_len, seq_len):
        _structseq_setitem(structseq, i, _tuple_getitem(seq_tuple, i))

    # Fill the remaining from the dict or set to None
    for i in range(seq_len, max_len):
        key = cls._structseq_field_names[i]
        _structseq_setitem(structseq, i, dict.get(key))

    return structseq


def _structseq_repr(self):
    _tuple_guard(self)
    tp = _type(self)
    fullname = f"{tp.__module__}.{tp.__name__}"
    if _repr_enter(self):
        return f"{fullname}(...)"
    field_names = tp._structseq_field_names
    name_index = 0
    value_strings = []
    for i in range(tp.n_sequence_fields):
        # Replicate cpython bug and simply take the next name in case of an
        # unnamed field...
        while True:
            name = field_names[name_index]
            name_index += 1
            if name is not None:
                break
        value = _tuple_getitem(self, i)
        value_strings.append(f"{name}={value!r}")
    _repr_leave(self)
    return f"{fullname}({', '.join(value_strings)})"


def _type_name(cls):
    return f"{cls.__module__}.{cls.__qualname__}"


def abs(x):
    "Same as a.__abs__()."
    dunder_abs = _object_type_getattr(x, "__abs__")
    if dunder_abs is _Unbound:
        raise TypeError(f"bad operand type for abs(): '{type(x).__name__}'")
    return dunder_abs()


def all(iterable):
    for element in iterable:
        if not element:
            return False
    return True


def any(iterable):
    for element in iterable:
        if element:
            return True
    return False


def ascii(obj):
    return _str_escape_non_ascii(repr(obj))


class async_generator(bootstrap=True):
    pass


def bin(number) -> str:
    _builtin()


class bool(int, bootstrap=True):
    def __and__(self, other):
        _bool_guard(self)
        if _bool_check(other):
            return self and other
        if _int_check(other):
            return int.__and__(other, self)
        return NotImplemented

    def __new__(cls, val=False):
        _builtin()

    def __or__(self, other):
        _builtin()

    def __rand__(self, other):
        return bool.__and__(self, other)

    def __repr__(self):
        return "True" if self else "False"

    def __ror__(self, other):
        _builtin()

    def __rxor__(self, other):
        _unimplemented()

    def __str__(self) -> str:  # noqa: T484
        return bool.__repr__(self)

    def __xor__(self, other):
        _unimplemented()


class bytearray(bootstrap=True):
    def __add__(self, other) -> bytearray:
        _builtin()

    def __alloc__(self):
        _unimplemented()

    def __contains__(self, key):
        result = _bytearray_contains(self, key)
        if result is not _Unbound:
            return result
        try:
            return _bytearray_contains(self, _index(key))
        except BaseException:
            pass
        _byteslike_guard(key)
        # TODO(T59013969): Add support for bytearray.__contains__(byteslike)
        _unimplemented()

    def __delitem__(self, key):
        _bytearray_guard(self)
        if _int_check(key):
            return _bytearray_delitem(self, key)
        if _slice_check(key):
            step = _slice_step(_slice_index(key.step))
            length = _bytearray_len(self)
            start = _slice_start(_slice_index(key.start), step, length)
            stop = _slice_stop(_slice_index(key.stop), step, length)
            return _bytearray_delslice(self, start, stop, step)
        if _object_type_hasattr(key, "__index__"):
            return _bytearray_delitem(self, _index(key))
        raise TypeError("bytearray indices must be integers or slices")

    def __eq__(self, value):
        _builtin()

    def __ge__(self, value):
        _builtin()

    def __getitem__(self, key):
        result = _bytearray_getitem(self, key)
        if result is not _Unbound:
            return result
        if _slice_check(key):
            step = _slice_step(_slice_index(key.step))
            length = _bytearray_len(self)
            start = _slice_start(_slice_index(key.start), step, length)
            stop = _slice_stop(_slice_index(key.stop), step, length)
            return _bytearray_getslice(self, start, stop, step)
        if _object_type_hasattr(key, "__index__"):
            return _bytearray_getitem(self, _index(key))
        raise TypeError(
            f"bytearray indices must be integers or slices, not {_type(key).__name__}"
        )

    __getattribute__ = object.__getattribute__

    def __gt__(self, value):
        _builtin()

    __hash__ = None

    def __iadd__(self, other) -> bytearray:
        _builtin()

    def __imul__(self, n: int) -> bytearray:
        _builtin()

    def __init__(self, source=_Unbound, encoding=_Unbound, errors=_Unbound):
        _bytearray_guard(self)
        _bytearray_clear(self)
        if source is _Unbound:
            if encoding is not _Unbound or errors is not _Unbound:
                raise TypeError("encoding or errors without sequence argument")
        elif _str_check(source):
            if encoding is _Unbound:
                raise TypeError("string argument without an encoding")
            encoded = str.encode(source, encoding, errors)
            _bytearray_setslice(self, 0, 0, 1, encoded)
        elif encoding is not _Unbound or errors is not _Unbound:
            raise TypeError("encoding or errors without a string argument")
        elif _byteslike_check(source):
            _bytearray_setslice(self, 0, 0, 1, source)
        elif _int_check(source) or _object_type_hasattr(source, "__index__"):
            _bytearray_setslice(self, 0, 0, 1, _bytes_repeat(b"\x00", _index(source)))
        else:
            _bytearray_setslice(self, 0, 0, 1, _bytes_new(source))

    def __iter__(self):
        _builtin()

    def __le__(self, value):
        _builtin()

    def __len__(self) -> int:
        _builtin()

    def __lt__(self, value):
        _builtin()

    def __mod__(self, value):
        _unimplemented()

    def __mul__(self, n: int) -> bytearray:
        _builtin()

    def __ne__(self, value):
        _builtin()

    def __new__(cls, source=_Unbound, encoding=_Unbound, errors=_Unbound):
        _builtin()

    def __reduce__(self):
        _unimplemented()

    def __reduce_ex__(self):
        _unimplemented()

    def __repr__(self):
        _builtin()

    def __rmod__(self, value):
        _unimplemented()

    def __rmul__(self, n: int) -> bytearray:
        _bytearray_guard(self)
        return bytearray.__mul__(self, n)

    def __setitem__(self, key, value):
        _bytearray_guard(self)
        if _int_check(key):
            _int_guard(value)
            return _bytearray_setitem(self, key, value)
        if _slice_check(key):
            step = _slice_step(_slice_index(key.step))
            length = _bytearray_len(self)
            start = _slice_start(_slice_index(key.start), step, length)
            stop = _slice_stop(_slice_index(key.stop), step, length)
            if not _byteslike_check(value):
                try:
                    it = (*value,)
                except TypeError:
                    raise TypeError(
                        "can assign only bytes, buffers, or iterables of ints "
                        "in range(0, 256)"
                    )
                value = _bytes_new(it)
            return _bytearray_setslice(self, start, stop, step, value)
        if _object_type_hasattr(key, "__index__"):
            _int_guard(value)
            return _bytearray_setitem(self, _index(key), value)
        raise TypeError("bytearray indices must be integers or slices")

    def append(self, item):
        result = _bytearray_append(self, item)
        if result is not _Unbound:
            return
        _bytearray_append(self, _index(item))

    def capitalize(self):
        _unimplemented()

    def center(self):
        _unimplemented()

    def clear(self):
        _bytearray_guard(self)
        _bytearray_clear(self)

    def copy(self):
        return _bytearray_copy(self)

    def count(self, sub, start=None, end=None):
        _bytearray_guard(self)
        start = 0 if start is None else _index(start)
        end = _bytearray_len(self) if end is None else _index(end)
        if _byteslike_check(sub):
            return _byteslike_count(self, sub, start, end)
        if _int_check(sub):
            return _byteslike_count(self, sub, start, end)
        if not _number_check(sub):
            raise TypeError(
                f"a bytes-like object is required, not '{_type(sub).__name__}'"
            )
        try:
            sub = _index(sub)
        except OverflowError:
            raise ValueError("byte must be in range(0, 256)")
        except Exception:
            raise TypeError(
                f"a bytes-like object is required, not '{_type(sub).__name__}'"
            )
        return _byteslike_count(self, sub, start, end)

    def decode(self, encoding="utf-8", errors=_Unbound):
        return _codecs.decode(self, encoding, errors)

    def endswith(self, suffix, start=_Unbound, end=_Unbound):
        _bytearray_guard(self)
        if not _tuple_check(suffix):
            return _byteslike_endswith(self, suffix, start, end)
        for item in suffix:
            if _byteslike_endswith(self, item, start, end):
                return True
        return False

    def expandtabs(self, tabsize=8):
        _unimplemented()

    def extend(self, iterable_of_ints) -> None:
        _bytearray_guard(self)
        length = _bytearray_len(self)
        if _byteslike_check(iterable_of_ints):
            value = iterable_of_ints
        elif _str_check(iterable_of_ints):
            # CPython iterates, so it does not fail for the empty string
            if iterable_of_ints:
                raise TypeError("cannot extend bytearray with str")
            value = b""
        else:
            value = _bytes_new(iterable_of_ints)
        _bytearray_setslice(self, length, length, 1, value)

    def find(self, sub, start=None, end=None) -> int:
        _bytearray_guard(self)
        start = 0 if start is None else _index(start)
        end = _bytearray_len(self) if end is None else _index(end)
        if _byteslike_check(sub):
            return _byteslike_find_byteslike(self, sub, start, end)
        if _int_check(sub):
            return _byteslike_find_int(self, sub, start, end)
        if not _number_check(sub):
            raise TypeError(
                f"a bytes-like object is required, not '{_type(sub).__name__}'"
            )
        try:
            sub = _index(sub)
        except OverflowError:
            raise ValueError("byte must be in range(0, 256)")
        except Exception:
            raise TypeError(
                f"a bytes-like object is required, not '{_type(sub).__name__}'"
            )
        return _byteslike_find_int(self, sub, start, end)

    @classmethod
    def fromhex(cls, string):
        _unimplemented()

    def hex(self) -> str:
        _builtin()

    def index(self, sub, start=None, end=None) -> int:
        _bytearray_guard(self)
        result = bytearray.find(self, sub, start, end)
        if result is -1:
            raise ValueError("subsection not found")
        return result

    def insert(self, index, item):
        _unimplemented()

    def isalnum(self):
        _unimplemented()

    def isalpha(self):
        _unimplemented()

    def isdigit(self):
        _unimplemented()

    def islower(self):
        _unimplemented()

    def isspace(self):
        _unimplemented()

    def istitle(self):
        _unimplemented()

    def isupper(self):
        _unimplemented()

    def join(self, iterable) -> bytearray:
        result = _bytearray_join(self, iterable)
        if result is not _Unbound:
            return result
        items = [x for x in iterable]
        return _bytearray_join(self, items)

    def ljust(self, width, fillchar=_Unbound):
        _unimplemented()

    def lower(self):
        _builtin()

    def lstrip(self, bytes=None):
        _builtin()

    @staticmethod
    def maketrans(frm, to) -> bytes:
        return bytes.maketrans(frm, to)

    def partition(self, sep):
        _unimplemented()

    def pop(self, index=-1):
        _unimplemented()

    def remove(self, value):
        _unimplemented()

    def replace(self, old, new, count=-1):
        _unimplemented()

    def reverse(self):
        _unimplemented()

    def rfind(self, sub, start=None, end=None) -> int:
        _bytearray_guard(self)
        start = 0 if start is None else _index(start)
        end = _bytearray_len(self) if end is None else _index(end)
        if _byteslike_check(sub):
            return _byteslike_rfind_byteslike(self, sub, start, end)
        if _int_check(sub):
            return _byteslike_rfind_int(self, sub, start, end)
        if not _number_check(sub):
            raise TypeError(
                f"a bytes-like object is required, not '{_type(sub).__name__}'"
            )
        try:
            sub = _index(sub)
        except OverflowError:
            raise ValueError("byte must be in range(0, 256)")
        except Exception:
            raise TypeError(
                f"a bytes-like object is required, not '{_type(sub).__name__}'"
            )
        return _byteslike_rfind_int(self, sub, start, end)

    def rindex(self, sub, start=None, end=None) -> int:
        _bytearray_guard(self)
        result = bytearray.rfind(self, sub, start, end)
        if result is -1:
            raise ValueError("subsection not found")
        return result

    def rjust(self, width, fillchar=_Unbound):
        _unimplemented()

    def rpartition(self, sep):
        _unimplemented()

    def rsplit(self, sep=None, maxsplit=-1):
        _unimplemented()

    def rstrip(self, bytes=None):
        _builtin()

    def split(self, sep=None, maxsplit=-1):
        _bytearray_guard(self)
        self_bytes = bytes(self)
        result = self_bytes.split(sep, maxsplit)
        return [bytearray(x) for x in result]

    def splitlines(self, keepends=False):
        _unimplemented()

    def startswith(self, prefix, start=None, end=None):
        _bytearray_guard(self)
        start = 0 if start is None else _slice_index(start)
        end = _bytearray_len(self) if end is None else _slice_index(end)
        if not _tuple_check(prefix):
            return _byteslike_startswith(self, prefix, start, end)
        for item in prefix:
            if _byteslike_startswith(self, item, start, end):
                return True
        return False

    def strip(self, bytes=None):
        _builtin()

    def swapcase(self):
        _unimplemented()

    def title(self):
        _unimplemented()

    def translate(self, table, delete=b""):
        _builtin()

    def upper(self):
        _builtin()

    def zfill(self):
        _unimplemented()


class bytearray_iterator(bootstrap=True):
    def __iter__(self):
        _builtin()

    def __length_hint__(self):
        _builtin()

    def __next__(self):
        _builtin()


class bytes(bootstrap=True):
    def __add__(self, other: bytes) -> bytes:
        _builtin()

    def __contains__(self, key):
        result = _bytes_contains(self, key)
        if result is not _Unbound:
            return result
        try:
            return _bytes_contains(self, _index(key))
        except BaseException:
            pass
        _byteslike_guard(key)
        return _byteslike_find_byteslike(self, key, 0, _bytes_len(self)) != -1

    def __eq__(self, other):
        _builtin()

    def __ge__(self, other):
        _builtin()

    __getattribute__ = object.__getattribute__

    def __getitem__(self, key):
        result = _bytes_getitem(self, key)
        if result is not _Unbound:
            return result
        if _slice_check(key):
            step = _slice_step(_slice_index(key.step))
            length = _bytes_len(self)
            start = _slice_start(_slice_index(key.start), step, length)
            stop = _slice_stop(_slice_index(key.stop), step, length)
            return _bytes_getslice(self, start, stop, step)
        if _object_type_hasattr(key, "__index__"):
            return _bytes_getitem(self, _index(key))
        raise TypeError(
            f"byte indices must be integers or slice, not {_type(key).__name__}"
        )

    def __getnewargs__(self, other):
        _unimplemented()

    def __gt__(self, other):
        _builtin()

    def __hash__(self):
        _builtin()

    def __iter__(self):
        _builtin()

    def __le__(self, other):
        _builtin()

    def __len__(self) -> int:
        _builtin()

    def __lt__(self, other):
        _builtin()

    def __mod__(self, n):
        _unimplemented()

    def __mul__(self, n: int) -> bytes:
        _builtin()

    def __ne__(self, other):
        _builtin()

    def __new__(cls, source=_Unbound, encoding=_Unbound, errors=_Unbound):  # noqa: C901
        _type_subclass_guard(cls, bytes)
        if source is _Unbound:
            if encoding is _Unbound and errors is _Unbound:
                return _bytes_from_bytes(cls, b"")
            raise TypeError("encoding or errors without sequence argument")
        if encoding is not _Unbound:
            if not _str_check(source):
                raise TypeError("encoding without a string argument")
            return _bytes_from_bytes(cls, str.encode(source, encoding, errors))
        if errors is not _Unbound:
            if _str_check(source):
                raise TypeError("string argument without an encoding")
            raise TypeError("errors without a string argument")
        dunder_bytes = _object_type_getattr(source, "__bytes__")
        if dunder_bytes is not _Unbound:
            result = dunder_bytes()
            if not _bytes_check(result):
                raise TypeError(
                    f"__bytes__ returned non-bytes (type {_type(result).__name__})"
                )
            return _bytes_from_bytes(cls, result)
        if _str_check(source):
            raise TypeError("string argument without an encoding")
        if _int_check(source):
            return _bytes_from_bytes(cls, _bytes_repeat(b"\x00", source))
        if _object_type_hasattr(source, "__index__"):
            return _bytes_from_bytes(cls, _bytes_repeat(b"\x00", _index(source)))
        return _bytes_from_bytes(cls, _bytes_new(source))

    def __repr__(self) -> str:  # noqa: T484
        _builtin()

    def __rmod__(self, n):
        _unimplemented()

    def __rmul__(self, n: int) -> bytes:
        _bytes_guard(self)
        return bytes.__mul__(self, n)

    __str__ = __repr__

    def capitalize(self):
        _unimplemented()

    def center(self, width, fillbyte=b" "):
        _bytes_guard(self)
        width = _index(width)
        if _bytes_check(fillbyte) and _bytes_len(fillbyte) == 1:
            pass
        elif _bytearray_check(fillbyte) and _bytearray_len(fillbyte) == 1:
            # convert into bytes
            fillbyte = _bytes_from_ints(fillbyte)
        else:
            raise TypeError(
                "center() argument 2 must be a byte string of length 1, not "
                f"{_type(fillbyte).__name__}"
            )

        pad = width - _bytes_len(self)
        if pad <= 0:
            return self
        left_pad = (pad >> 1) + (pad & width & 1)
        return fillbyte * left_pad + self + fillbyte * (pad - left_pad)

    def count(self, sub, start=None, end=None):
        _bytes_guard(self)
        start = 0 if start is None else _index(start)
        end = _bytes_len(self) if end is None else _index(end)
        if _byteslike_check(sub):
            return _byteslike_count(self, sub, start, end)
        if _int_check(sub):
            return _byteslike_count(self, sub, start, end)
        if not _number_check(sub):
            raise TypeError(
                f"a bytes-like object is required, not '{_type(sub).__name__}'"
            )
        try:
            sub = _index(sub)
        except OverflowError:
            raise ValueError("byte must be in range(0, 256)")
        except Exception:
            raise TypeError(
                f"a bytes-like object is required, not '{_type(sub).__name__}'"
            )
        return _byteslike_count(self, sub, start, end)

    def decode(self, encoding="utf-8", errors="strict") -> str:
        result = _bytes_decode(self, encoding)
        if result is not _Unbound:
            return result
        return _codecs.decode(self, encoding, errors)

    def endswith(self, suffix, start=_Unbound, end=_Unbound):
        _bytes_guard(self)
        if not _tuple_check(suffix):
            return _byteslike_endswith(self, suffix, start, end)
        for item in suffix:
            if _byteslike_endswith(self, item, start, end):
                return True
        return False

    def expandtabs(self, tabsize=8):
        _unimplemented()

    def find(self, sub, start=None, end=None) -> int:
        _bytes_guard(self)
        start = 0 if start is None else _index(start)
        end = _bytes_len(self) if end is None else _index(end)
        if _byteslike_check(sub):
            return _byteslike_find_byteslike(self, sub, start, end)
        if _int_check(sub):
            return _byteslike_find_int(self, sub, start, end)
        if not _number_check(sub):
            raise TypeError(
                f"a bytes-like object is required, not '{_type(sub).__name__}'"
            )
        try:
            sub = _index(sub)
        except OverflowError:
            raise ValueError("byte must be in range(0, 256)")
        except Exception:
            raise TypeError(
                f"a bytes-like object is required, not '{_type(sub).__name__}'"
            )
        return _byteslike_find_int(self, sub, start, end)

    @classmethod
    def fromhex(cls, string):
        _str_guard(string)
        fromhex_error = "non-hexadecimal number found in fromhex() arg at position "
        result = bytearray()
        length = _str_len(string)
        i = 0
        while i < length:
            top_cp = string[i]
            if top_cp == " ":
                i += 1
                continue
            try:
                top_digit = _int_new_from_str(int, top_cp, 16)
            except ValueError:
                raise ValueError(fromhex_error + str(i))
            i += 1
            if i == length:
                raise ValueError(fromhex_error + str(i))
            bottom_cp = string[i]
            try:
                bottom_digit = _int_new_from_str(int, bottom_cp, 16)
            except ValueError:
                raise ValueError(fromhex_error + str(i))
            _bytearray_append(result, top_digit << 4 | bottom_digit)
            i += 1
        return cls(result)

    def hex(self) -> str:
        _builtin()

    def index(self, sub, start=None, end=None) -> int:
        _bytes_guard(self)
        result = bytes.find(self, sub, start, end)
        if result is -1:
            raise ValueError("subsection not found")
        return result

    def isalnum(self):
        _unimplemented()

    def isalpha(self):
        _unimplemented()

    def isdigit(self):
        _unimplemented()

    def islower(self):
        _unimplemented()

    def isspace(self):
        _unimplemented()

    def istitle(self):
        _unimplemented()

    def isupper(self):
        _unimplemented()

    def join(self, iterable) -> bytes:
        result = _bytes_join(self, iterable)
        if result is not _Unbound:
            return result
        items = [x for x in iterable]
        return _bytes_join(self, items)

    def ljust(self, width, fillbyte=b" "):
        _bytes_guard(self)
        width = _index(width)
        if _bytes_check(fillbyte) and _bytes_len(fillbyte) == 1:
            pass
        elif _bytearray_check(fillbyte) and _bytearray_len(fillbyte) == 1:
            # convert into bytes
            fillbyte = _bytes_from_ints(fillbyte)
        else:
            raise TypeError(
                "ljust() argument 2 must be a byte string of length 1, not "
                f"{_type(fillbyte).__name__}"
            )

        padding = width - _bytes_len(self)
        return self + fillbyte * padding if padding > 0 else self

    def lower(self):
        _builtin()

    def lstrip(self, bytes=None):
        _builtin()

    @staticmethod
    def maketrans(frm, to) -> bytes:
        _byteslike_guard(frm)
        _byteslike_guard(to)
        if len(frm) != len(to):
            raise ValueError("maketrans arguments must have same length")
        return _bytes_maketrans(frm, to)

    def partition(self, sep):
        _unimplemented()

    def replace(self, old, new, count=-1):
        result = _bytes_replace(self, old, new, count)
        if result is not _Unbound:
            return result
        return _bytes_replace(self, old, new, _index(count))

    def rfind(self, sub, start=None, end=None) -> int:
        _bytes_guard(self)
        start = 0 if start is None else _index(start)
        end = bytes.__len__(self) if end is None else _index(end)
        if _byteslike_check(sub):
            return _byteslike_rfind_byteslike(self, sub, start, end)
        if _int_check(sub):
            return _byteslike_rfind_int(self, sub, start, end)
        if not _number_check(sub):
            raise TypeError(
                f"a bytes-like object is required, not '{_type(sub).__name__}'"
            )
        try:
            sub = _index(sub)
        except OverflowError:
            raise ValueError("byte must be in range(0, 256)")
        except Exception:
            raise TypeError(
                f"a bytes-like object is required, not '{_type(sub).__name__}'"
            )
        return _byteslike_rfind_int(self, sub, start, end)

    def rindex(self, sub, start=None, end=None) -> int:
        _bytes_guard(self)
        result = bytes.rfind(self, sub, start, end)
        if result is -1:
            raise ValueError("subsection not found")
        return result

    def rjust(self, width, fillbyte=b" "):
        _bytes_guard(self)
        width = _index(width)
        if _bytes_check(fillbyte) and _bytes_len(fillbyte) == 1:
            pass
        elif _bytearray_check(fillbyte) and _bytearray_len(fillbyte) == 1:
            # convert into bytes
            fillbyte = _bytes_from_ints(fillbyte)
        else:
            raise TypeError(
                "rjust() argument 2 must be a byte string of length 1, not "
                f"{_type(fillbyte).__name__}"
            )

        padding = width - _bytes_len(self)
        return fillbyte * padding + self if padding > 0 else self

    def rpartition(self, sep):
        _unimplemented()

    def rsplit(self, sep=None, maxsplit=-1):
        _unimplemented()

    def rstrip(self, bytes=None):
        _builtin()

    def split(self, sep=None, maxsplit=-1):
        _bytes_guard(self)
        if not _int_check(maxsplit):
            maxsplit = _index(maxsplit)
        if sep is None:
            return _bytes_split_whitespace(self, maxsplit)
        _byteslike_guard(sep)
        return _bytes_split(self, sep, maxsplit)

    def splitlines(self, keepends=False):
        _unimplemented()

    def startswith(self, prefix, start=None, end=None):
        _bytes_guard(self)
        start = 0 if start is None else _slice_index(start)
        end = _bytes_len(self) if end is None else _slice_index(end)
        if not _tuple_check(prefix):
            return _byteslike_startswith(self, prefix, start, end)
        for item in prefix:
            if _byteslike_startswith(self, item, start, end):
                return True
        return False

    def strip(self, bytes=None):
        _builtin()

    def swapcase(self):
        _unimplemented()

    def title(self):
        _unimplemented()

    def translate(self, table, delete=b""):
        _builtin()

    def upper(self):
        _builtin()

    def zfill(self):
        _unimplemented()


class bytes_iterator(bootstrap=True):
    def __iter__(self):
        _builtin()

    def __length_hint__(self):
        _builtin()

    def __next__(self):
        _builtin()


def callable(f):
    _builtin()


def chr(c):
    _builtin()


class code(bootstrap=True):
    def __new__(
        cls,
        argcount,
        kwonlyargcount,
        nlocals,
        stacksize,
        flags,
        code,
        consts,
        names,
        varnames,
        filename,
        name,
        firstlineno,
        lnotab,
        freevars=(),
        cellvars=(),
    ):
        argcount = _int(argcount)
        posonlyargcount = 0
        kwonlyargcount = _int(kwonlyargcount)
        nlocals = _int(nlocals)
        stacksize = _int(stacksize)
        flags = _int(flags)
        _bytes_guard(code)
        _tuple_guard(consts)
        _tuple_guard(names)
        _tuple_guard(varnames)
        _str_guard(filename)
        _str_guard(name)
        firstlineno = _int(firstlineno)
        _bytes_guard(lnotab)
        _tuple_guard(freevars)
        _tuple_guard(cellvars)
        return _code_new(
            cls,
            argcount,
            posonlyargcount,
            kwonlyargcount,
            nlocals,
            stacksize,
            flags,
            code,
            consts,
            names,
            varnames,
            filename,
            name,
            firstlineno,
            lnotab,
            freevars,
            cellvars,
        )

    def __hash__(self):
        _code_guard(self)
        result = (
            hash(self.co_name)
            ^ hash(self.co_code)
            ^ hash(self.co_consts)
            ^ hash(self.co_names)
            ^ hash(self.co_varnames)
            ^ hash(self.co_freevars)
            ^ hash(self.co_cellvars)
            ^ self.co_argcount
            ^ self.co_kwonlyargcount
            ^ self.co_nlocals
            ^ self.co_flags
        )
        return result if result != -1 else -2


def compile(source, filename, mode, flags=0, dont_inherit=False, optimize=-1):
    from _compile import compile

    if not dont_inherit:
        try:
            code = _caller_function().__code__
            flags |= code.co_flags & _compile_flags_mask
        except ValueError:
            pass  # May have been called on a fresh stackframe.

    return compile(source, filename, mode, flags, optimize)


def _complex_str_parts(s):  # noqa: C901
    # This function is based on PyPy's _split_complex function in
    # complexobject.py
    slen = len(s)
    if slen == 0:
        raise ValueError
    realstart = 0
    realstop = 0
    imagstart = 0
    imagstop = 0
    imagsign = " "
    i = 0
    # ignore whitespace at beginning and end
    while i < slen and s[i] == " ":
        i += 1
    while slen > 0 and s[slen - 1] == " ":
        slen -= 1

    # if it's all whitespace, raise
    if i >= slen:
        raise ValueError

    if s[i] == "(" and s[slen - 1] == ")":
        i += 1
        slen -= 1
        # ignore whitespace after bracket
        while i < slen and s[i] == " ":
            i += 1
        while slen > 0 and s[slen - 1] == " ":
            slen -= 1

    # extract first number
    realstart = i
    pc = s[i]
    while i < slen and s[i] != " ":
        if s[i] in ("+", "-") and pc not in ("e", "E") and i != realstart:
            break
        pc = s[i]
        i += 1

    realstop = i

    # return appropriate strings is only one number is there
    if i >= slen:
        newstop = realstop - 1
        if newstop < 0:
            raise ValueError
        if s[newstop] in ("j", "J"):
            if realstart == newstop:
                imagpart = "1.0"
            elif realstart == newstop - 1 and s[realstart] == "+":
                imagpart = "1.0"
            elif realstart == newstop - 1 and s[realstart] == "-":
                imagpart = "-1.0"
            else:
                imagpart = s[realstart:newstop]
            return "0.0", imagpart
        else:
            return s[realstart:realstop], "0.0"

    # find sign for imaginary part
    if s[i] == "-" or s[i] == "+":
        imagsign = s[i]
    else:
        raise ValueError

    i += 1
    if i >= slen:
        raise ValueError

    imagstart = i
    pc = s[i]
    while i < slen and s[i] != " ":
        if s[i] in ("+", "-") and pc not in ("e", "E"):
            break
        pc = s[i]
        i += 1

    imagstop = i - 1
    if imagstop < 0:
        raise ValueError
    if s[imagstop] not in ("j", "J"):
        raise ValueError
    if imagstop < imagstart:
        raise ValueError

    if i < slen:
        raise ValueError

    realpart = s[realstart:realstop]
    if imagstart == imagstop:
        imagpart = "1.0"
    else:
        imagpart = s[imagstart:imagstop]
    if imagsign == "-":
        imagpart = imagsign + imagpart

    return realpart, imagpart


class complex(bootstrap=True):
    def __abs__(self):
        _unimplemented()

    def __add__(self, other):
        _builtin()

    def __bool__(self):
        return True if (_complex_imag(self) or _complex_real(self)) else False

    def __divmod__(self, other):
        _unimplemented()

    def __eq__(self, other) -> bool:
        imag = _complex_imag(self)
        real = _complex_real(self)
        if _float_check(other) or _int_check(other):
            if imag != 0.0:
                return False
            return real == other
        if _complex_check(other):
            return imag == _complex_imag(other) and real == _complex_real(other)
        return NotImplemented

    def __float__(self, other):
        _unimplemented()

    def __floordiv__(self, other):
        _unimplemented()

    def __ge__(self, other):
        _unimplemented()

    __getattribute__ = object.__getattribute__

    def __getnewargs__(self):
        _unimplemented()

    def __gt__(self, other):
        _unimplemented()

    def __hash__(self) -> int:
        _builtin()

    def __int__(self):
        _unimplemented()

    def __le__(self, other):
        _unimplemented()

    def __lt__(self, other):
        _unimplemented()

    def __mod__(self, other):
        _unimplemented()

    def __mul__(self, other):
        _unimplemented()

    def __ne__(self, other):
        _unimplemented()

    def __neg__(self):
        _builtin()

    def __new__(cls, real=0.0, imag=_Unbound):  # noqa: C901
        _type_subclass_guard(cls, complex)
        result = _complex_new(cls, real, imag)
        if result is not _Unbound:
            return result
        if _str_check(real):
            if imag is _Unbound:
                real_str, imag_str = _complex_str_parts(real)
                return _complex_new(cls, float(real_str), float(imag_str))
            raise TypeError("complex() can't take second arg if first is a string")
        if _str_check(imag):
            raise TypeError("complex() second arg can't be a string")
        dunder_complex = _object_type_getattr(real, "__complex__")
        if dunder_complex is not _Unbound:
            real = dunder_complex()
            if not _complex_check(real):
                raise TypeError("__complex__ should return a complex object")
            elif not _complex_checkexact(real):
                # NOTE: type(real) could a subclass of complex, so coerce it here
                real = _complex_new(complex, real.real, real.imag)
        if not _object_type_hasattr(real, "__float__"):
            raise TypeError(
                "complex() first argument must be a string or a number, "
                f"not '{_type(real).__name__}'"
            )
        if imag is not _Unbound and not _object_type_hasattr(imag, "__float__"):
            raise TypeError(
                "complex() second argument must be a number, "
                f"not '{_type(imag).__name__}'"
            )
        if not _complex_check(real) and not _float_check_exact(real):
            real = float(real)
        if (
            imag is not _Unbound
            and not _complex_check(imag)
            and not _float_check_exact(imag)
        ):
            imag = float(imag)

        return _complex_new(cls, real, imag)

    def __pos__(self):
        _builtin()

    def __pow__(self, other):
        _unimplemented()

    __radd__ = __add__

    def __rdivmod__(self, other):
        _unimplemented()

    def __repr__(self):
        return f"({self.real}+{self.imag}j)"

    def __rfloordiv__(self, other):
        _unimplemented()

    def __rmod__(self, other):
        _unimplemented()

    def __rmul__(self, other):
        _unimplemented()

    def __rpow__(self, other):
        _unimplemented()

    def __rsub__(self, other):
        _builtin()

    def __rtruediv__(self, other):
        _unimplemented()

    def __str__(self, other):
        _unimplemented()

    def __sub__(self, other):
        _builtin()

    def __truediv__(self, other):
        _unimplemented()

    def conjugate(self):
        _unimplemented()

    imag = _property(_complex_imag)

    real = _property(_complex_real)


copyright = ""


class coroutine(bootstrap=True):
    def close(self):
        # TODO(T42623564): implement this.
        pass

    def send(self, value):
        _builtin()

    def throw(self, exc, value=_Unbound, tb=_Unbound):
        pass

    def __repr__(self):
        return f"<coroutine object {self.__qualname__} at {_address(self):#x}>"


credits = ""


def delattr(obj, name):
    _builtin()


class dict(bootstrap=True):
    def __contains__(self, key) -> bool:
        _dict_guard(self)
        return _dict_get(self, key, _Unbound) is not _Unbound  # noqa: T484

    def __delitem__(self, key):
        _builtin()

    def __eq__(self, other):
        _builtin()

    def __ge__(self, other):
        _unimplemented()

    __getattribute__ = object.__getattribute__

    def __getitem__(self, key):
        _dict_guard(self)
        result = _dict_get(self, key, _Unbound)
        if result is _Unbound:
            if not _dict_check_exact(self):
                dunder_missing = _object_type_getattr(self, "__missing__")
                if dunder_missing is not _Unbound:
                    # Subclass defined __missing__
                    return dunder_missing(key)
            raise KeyError(key)
        return result

    def __gt__(self, other):
        _unimplemented()

    __hash__ = None

    @_positional_only(2)
    def __init__(self, other=_Unbound, **kwargs):
        dict.update(self, other, **kwargs)

    def __iter__(self):
        _builtin()

    def __le__(self, other):
        _unimplemented()

    def __len__(self):
        _builtin()

    def __lt__(self, other):
        _unimplemented()

    def __ne__(self, other):
        eq_result = dict.__eq__(self, other)
        if eq_result is NotImplemented:
            return NotImplemented
        return not eq_result

    def __new__(cls, *args, **kwargs):
        _builtin()

    def __repr__(self):
        return _mapping_repr("{", self, "}")

    __setitem__ = _dict_setitem

    def clear(self):
        _builtin()

    def copy(self):
        _dict_guard(self)
        result = {}
        result.update(self)
        return result

    @_classmethod
    def fromkeys(cls, iterable, value=None):  # noqa: B902
        _type_subclass_guard(cls, dict)
        result = cls()
        for key in iterable:
            result[key] = value
        return result

    get = _dict_get

    def items(self):
        _builtin()

    def keys(self):
        _builtin()

    def pop(self, key, default=_Unbound):
        _dict_guard(self)
        value = _dict_get(self, key, default)
        if value is _Unbound:
            raise KeyError(key)
        if key in self:
            dict.__delitem__(self, key)
        return value

    def popitem(self):
        _builtin()

    def setdefault(self, key, default=None):
        _dict_guard(self)
        value = _dict_get(self, key, _Unbound)
        if value is _Unbound:
            _dict_setitem(self, key, default)
            return default
        return value

    @_positional_only(2)
    def update(self, iterable=_Unbound, **kwargs):
        if _dict_update(self, iterable, kwargs) is not _Unbound:
            return

        # iterable should be an iterable of pairs
        num_items = 0
        for x in iter(iterable):
            item = tuple(x)
            if _tuple_len(item) != 2:
                raise ValueError(
                    f"dictionary update sequence element #{num_items} has length "
                    f"{_tuple_len(item)}; 2 is required"
                )
            _dict_setitem(self, *item)
            num_items += 1

        for key, value in kwargs.items():
            _dict_setitem(self, key, value)

    def values(self):
        _builtin()


def _dictview_and(self, other):
    result = set(self)
    result.intersection_update(other)
    return result


def _dictview_eq(self, other):
    other_type = _type(other)
    if (
        (not _anyset_check(other))
        and other_type is not dict_items
        and other_type is not dict_keys
    ):
        return NotImplemented
    if len(self) != len(other):
        return False
    for item in self:
        if item not in other:
            return False
    return True


def _dictview_or(self, other):
    return {*self, *other}


def _dictview_sub(self, other):
    result = set(self)
    result.difference_update(other)
    return result


def _dictview_xor(self, other):
    result = set(self)
    result.symmetric_difference_update(other)
    return result


class dict_itemiterator(bootstrap=True):
    def __iter__(self):
        _builtin()

    def __length_hint__(self):
        _builtin()

    def __next__(self):
        _builtin()


class dict_items(bootstrap=True):
    def __and__(self, other):
        _dict_items_guard(self)
        return _dictview_and(self, other)

    def __eq__(self, other):
        _dict_items_guard(self)
        return _dictview_eq(self, other)

    def __iter__(self):
        _builtin()

    def __len__(self):
        _builtin()

    def __or__(self, other):
        _dict_items_guard(self)
        return _dictview_or(self, other)

    def __repr__(self):
        _dict_items_guard(self)
        if _repr_enter(self):
            return "..."
        result = _sequence_repr("dict_items([", list(self), "])")
        _repr_leave(self)
        return result

    def __ror__(self, other):
        _dict_items_guard(self)
        return _dictview_or(self, other)

    def __sub__(self, other):
        _dict_items_guard(self)
        return _dictview_sub(self, other)

    def __xor__(self, other):
        _dict_items_guard(self)
        return _dictview_xor(self, other)


class dict_keyiterator(bootstrap=True):
    def __iter__(self):
        _builtin()

    def __length_hint__(self):
        _builtin()

    def __next__(self):
        _builtin()


class dict_keys(bootstrap=True):
    def __and__(self, other):
        _dict_keys_guard(self)
        return _dictview_and(self, other)

    def __eq__(self, other):
        _dict_keys_guard(self)
        return _dictview_eq(self, other)

    def __iter__(self):
        _builtin()

    def __len__(self):
        _builtin()

    def __or__(self, other):
        _dict_keys_guard(self)
        return _dictview_or(self, other)

    def __repr__(self):
        _dict_keys_guard(self)
        if _repr_enter(self):
            return "..."
        result = _sequence_repr("dict_keys([", list(self), "])")
        _repr_leave(self)
        return result

    def __ror__(self, other):
        _dict_keys_guard(self)
        return _dictview_or(self, other)

    def __sub__(self, other):
        _dict_keys_guard(self)
        return _dictview_sub(self, other)

    def __xor__(self, other):
        _dict_keys_guard(self)
        return _dictview_xor(self, other)


class dict_valueiterator(bootstrap=True):
    def __iter__(self):
        _builtin()

    def __length_hint__(self):
        _builtin()

    def __next__(self):
        _builtin()


class dict_values(bootstrap=True):
    def __repr__(self):
        if _repr_enter(self):
            return "..."
        result = _sequence_repr("dict_values([", list(self), "])")
        _repr_leave(self)
        return result

    def __iter__(self):
        _builtin()

    def __len__(self):
        _builtin()


def dir(obj=_Unbound):
    if obj is _Unbound:
        names = _caller_locals().keys()
    else:
        names = _type(obj).__dir__(obj)
    return sorted(names)


divmod = _divmod


class ellipsis(bootstrap=True):
    def __repr__(self):
        return "Ellipsis"


class enumerate:
    __getattribute__ = object.__getattribute__

    def __init__(self, iterable, start=0):
        self.iterator = iter(iterable)
        self.index = start

    def __iter__(self):
        return self

    def __reduce__(self):
        _unimplemented()

    def __next__(self):
        result = (self.index, next(self.iterator))
        self.index += 1
        return result


def eval(source, globals=None, locals=None):
    if globals is None:
        caller = _caller_function()
        mod = caller.__module_object__
        if locals is None:
            locals = _caller_locals()
    elif _module_proxy_check(globals):
        mod = globals.__module_object__
        globals = None
    elif _dict_check(globals):
        mod = module("")
        mod.__dict__.update(globals)
    else:
        raise TypeError("'eval' arg 2 requires a dict or None")

    if locals is None:
        pass  # this is specifically allowed
    elif locals is globals:
        locals = None
    elif _mapping_check(locals):
        pass
    else:
        raise TypeError("'eval' arg 3 requires a mapping or None")

    if _code_check(source):
        code = source
    else:
        try:
            caller = _caller_function()
            flags = caller.__code__.co_flags & _compile_flags_mask
        except ValueError:
            flags = 0  # May have been called on a fresh stackframe.
        from _compile import compile

        if _str_check(source) or _byteslike_check(source):
            source = source.lstrip()
        code = compile(source, "<string>", "eval", flags, -1)
    if code.co_freevars:
        raise TypeError("'eval' code may not contain free variables")
    res = _exec(code, mod, locals)
    if globals is not None:
        globals.update(mod.__dict__)
    return res


def exec(source, globals=None, locals=None):
    if globals is None:
        caller = _caller_function()
        mod = caller.__module_object__
        if locals is None:
            locals = _caller_locals()
    elif _module_proxy_check(globals):
        mod = globals.__module_object__
        globals = None
    elif _dict_check(globals):
        mod = module("")
        mod.__dict__.update(globals)
    else:
        raise TypeError("'exec' arg 2 requires a dict or None")

    if locals is None:
        pass  # this is specifically allowed
    elif locals is globals:
        locals = None
    elif _mapping_check(locals):
        pass
    else:
        raise TypeError("'exec' arg 3 requires a mapping or None")

    if _code_check(source):
        code = source
    else:
        try:
            caller = _caller_function()
            flags = caller.__code__.co_flags & _compile_flags_mask
        except ValueError:
            flags = 0  # May have been called on a fresh stackframe.
        from _compile import compile

        code = compile(source, "<string>", "exec", flags, -1)
    if code.co_freevars:
        raise TypeError("'exec' code may not contain free variables")
    _exec(code, mod, locals)
    if globals is not None:
        globals.update(mod.__dict__)


class filter:
    """filter(function or None, iterable) --> filter object

    Return an iterator yielding those items of iterable for which function(item)
    is true. If function is None, return the items that are true.
    """

    __getattribute__ = object.__getattribute__

    def __iter__(self):
        return self

    def __new__(cls, function, iterable, **kwargs):
        obj = super(filter, cls).__new__(cls)
        obj.func = (lambda e: e) if function is None else function
        obj.iter = iter(iterable)
        return obj

    def __next__(self):
        func = self.func
        while True:
            item = next(self.iter)
            if func(item):
                return item

    def __reduce__(self):
        _unimplemented()


class float(bootstrap=True):
    def __abs__(self) -> float:
        _builtin()

    def __add__(self, n: float) -> float:
        _builtin()

    def __bool__(self) -> bool:
        _builtin()

    def __eq__(self, n: float) -> bool:  # noqa: T484
        _builtin()

    def __divmod__(self, n) -> float:
        _float_guard(self)
        if _float_check(n):
            return _float_divmod(self, n)
        if _int_check(n):
            return _float_divmod(self, int.__float__(n))
        return NotImplemented

    def __float__(self) -> float:
        _builtin()

    def __floordiv__(self, n) -> float:
        _float_guard(self)
        if _float_check(n):
            return _float_divmod(self, n)[0]
        if _int_check(n):
            return _float_divmod(self, int.__float__(n))[0]
        return NotImplemented

    def __ge__(self, n: float) -> bool:
        _builtin()

    __getattribute__ = object.__getattribute__

    @classmethod
    def __getformat__(cls: type, typearg: str) -> str:
        _type_subclass_guard(cls, float)
        _str_guard(typearg)
        typearg = str(typearg)
        if typearg != "double" and typearg != "float":
            raise ValueError("__getformat__() argument 1 must be 'double' or 'float'")
        return f"IEEE, {_sys.byteorder}-endian"

    def __getnewargs__(self):
        _unimplemented()

    def __gt__(self, n: float) -> bool:
        _builtin()

    def __hash__(self) -> int:
        _builtin()

    def __int__(self) -> int:
        _builtin()

    def __le__(self, n: float) -> bool:
        _builtin()

    def __lt__(self, n: float) -> bool:
        _builtin()

    def __mod__(self, n) -> float:
        _float_guard(self)
        if _float_check(n):
            return _float_divmod(self, n)[1]
        if _int_check(n):
            return _float_divmod(self, int.__float__(n))[1]
        return NotImplemented

    def __mul__(self, n: float) -> float:
        _builtin()

    def __ne__(self, n: float) -> float:  # noqa: T484
        _float_guard(self)
        if not _float_check(n) and not _int_check(n):
            return NotImplemented
        return not float.__eq__(self, n)  # noqa: T484

    def __neg__(self) -> float:
        _builtin()

    def __new__(cls, arg=0.0) -> float:
        _type_subclass_guard(cls, float)
        if _str_check_exact(arg):
            return _float_new_from_str(cls, arg)
        if _float_check_exact(arg):
            return _float_new_from_float(cls, arg)
        dunder_float = _object_type_getattr(arg, "__float__")
        if dunder_float is not _Unbound:
            result = dunder_float()
            if _float_check_exact(result):
                return _float_new_from_float(cls, result)
            if not _float_check(result):
                raise TypeError(
                    f"{_type(arg).__name__}.__float__ returned non-float "
                    f"(type {_type(result).__name__})"
                )
            _warn(
                f"{_type(arg).__name__} returned non-float (type "
                f"{_type(result).__name__}).  The ability to return an "
                "instance of a strict subclass of float is deprecated, and may "
                "be removed in a future version of Python.",
                DeprecationWarning,
            )
        if _str_check(arg):
            return _float_new_from_str(cls, arg)
        if _byteslike_check(arg):
            return _float_new_from_byteslike(cls, arg)
        raise TypeError(
            "float() argument must be a string or a number, "
            f"not '{_type(arg).__name__}'"
        )

    def __pos__(self, other):
        _unimplemented()

    def __pow__(self, other, mod=None) -> float:
        _builtin()

    def __radd__(self, n: float) -> float:
        # The addition for floating point numbers is commutative:
        # https://en.wikipedia.org/wiki/Floating-point_arithmetic#Accuracy_problems.
        # Note: Handling NaN payloads may vary depending on the hardware, but nobody
        # seems to depend on it due to such variance.
        return float.__add__(self, n)

    def __rdivmod__(self, n) -> float:
        _float_guard(self)
        if _float_check(n):
            return _float_divmod(n, self)
        if _int_check(n):
            return _float_divmod(int.__float__(n), self)
        return NotImplemented

    def __rfloordiv__(self, n) -> float:
        _float_guard(self)
        if _float_check(n):
            return _float_divmod(n, self)[0]
        if _int_check(n):
            return _float_divmod(int.__float__(n), self)[0]
        return NotImplemented

    def __repr__(self) -> str:
        if not _float_check(self):
            raise TypeError(
                f"'__repr__' requires a 'float' object "
                f"but received a '{_type(self).__name__}'"
            )
        return _float_format(self, "r", 0, False, True, False)

    def __rmul__(self, n: float) -> float:
        # The multiplication for floating point numbers is commutative:
        # https://en.wikipedia.org/wiki/Floating-point_arithmetic#Accuracy_problems
        return float.__mul__(self, n)

    def __rmod__(self, n) -> float:
        _float_guard(self)
        if _float_check(n):
            return _float_divmod(n, self)[1]
        if _int_check(n):
            return _float_divmod(int.__float__(n), self)[1]
        return NotImplemented

    def __round__(self, ndigits=None):
        _builtin()

    def __rpow__(self, other, mod=None):
        _float_guard(self)
        if _float_check(other):
            return float.__pow__(other, self, mod)
        if _int_check(other):
            return float.__pow__(int.__float__(other), self, mod)
        return NotImplemented

    def __rsub__(self, n: float) -> float:
        # n - self == -self + n.
        return float.__neg__(self).__add__(n)

    def __rtruediv__(self, n: float) -> float:
        _builtin()

    @classmethod
    def __setformat__(cls, fmt):
        _unimplemented()

    def __str__(self) -> str:
        if not _float_check(self):
            raise TypeError(
                f"'__str__' requires a 'float' object "
                f"but received a '{_type(self).__name__}'"
            )
        return _float_format(self, "r", 0, False, True, False)

    def __sub__(self, n: float) -> float:
        _builtin()

    def __truediv__(self, n: float) -> float:
        _builtin()

    def __trunc__(self) -> int:
        _builtin()

    def as_integer_ratio(self):
        _unimplemented()

    def conjugate(self):
        _unimplemented()

    @classmethod
    def fromhex(cls, string):
        _unimplemented()

    def hex(self):
        _unimplemented()

    def imag(self):
        _unimplemented()

    def is_integer(self):
        _unimplemented()

    def real(self):
        _unimplemented()


def format(obj, fmt_spec):
    if not _str_check(fmt_spec):
        raise TypeError(
            f"fmt_spec must be str instance, not '{_type(fmt_spec).__name__}'"
        )
    result = obj.__format__(fmt_spec)
    if not _str_check(result):
        raise TypeError(
            f"__format__ must return str instance, not '{_type(result).__name__}'"
        )
    return result


class frozenset(bootstrap=True):
    def __and__(self, other):
        _builtin()

    def __contains__(self, value):
        _builtin()

    def __eq__(self, other):
        _builtin()

    def __ge__(self, other):
        _builtin()

    __getattribute__ = object.__getattribute__

    def __gt__(self, other):
        _builtin()

    def __hash__(self) -> int:
        _builtin()

    def __iter__(self):
        _builtin()

    def __le__(self, other):
        _builtin()

    def __len__(self):
        _builtin()

    def __lt__(self, other):
        _builtin()

    def __ne__(self, other):
        _builtin()

    def __new__(cls, iterable=_Unbound):
        _builtin()

    def __or__(self, other):
        _builtin()

    def __rand__(self, other):
        _unimplemented()

    def __reduce__(self):
        _unimplemented()

    def __repr__(self):
        _frozenset_guard(self)
        name = _type(self).__name__
        if _repr_enter(self):
            return f"{name}(...)"
        if len(self) == 0:
            _repr_leave(self)
            return f"{name}()"
        result = f"{name}({{{', '.join(repr(x) for x in self)}}})"
        _repr_leave(self)
        return result

    def __ror__(self, other):
        _unimplemented()

    def __rsub__(self, other):
        _unimplemented()

    def __rxor__(self, other):
        _unimplemented()

    def __sub__(self, other):
        _unimplemented()

    def __xor__(self, other):
        _unimplemented()

    def copy(self):
        _builtin()

    def difference(self, *others):
        _frozenset_guard(self)
        result = list(self)
        for other in others:
            diff = []
            for item in result:
                if item not in other:
                    diff.append(item)
            result = diff
        return frozenset(result)

    def intersection(self, other):
        _builtin()

    def isdisjoint(self, other):
        _builtin()

    def issubset(self, other):
        _unimplemented()

    def issuperset(self, other):
        _frozenset_guard(self)
        if not _anyset_check(other):
            other = set(other)
        return set.__ge__(self, other)

    def symmetric_difference(self, other):
        _unimplemented()

    def union(self, other):
        _unimplemented()


class generator(bootstrap=True):
    def __iter__(self):
        _builtin()

    def __next__(self):
        _builtin()

    def send(self, value):
        _builtin()

    def throw(self, exc, value=_Unbound, tb=_Unbound):
        _builtin()

    def __repr__(self):
        return f"<generator object {self.__qualname__} at {_address(self):#x}>"


def getattr(obj, key, default=_Unbound):
    _builtin()


def globals():
    return _caller_function().__module_object__.__dict__


def hasattr(obj, name):
    _builtin()


def hash(obj) -> int:
    _builtin()


# TODO(T63894279): Hide private names.
class frame(bootstrap=True):
    @_property
    def f_builtins(self):
        return self._function.__module_object__.__builtins__.__dict__

    # TODO(T63894279): Once self._function.__code__ becomes a mutable attribute,
    # this should be populated in the native object directly.
    @_property
    def f_code(self):
        return self._function.__code__

    @_property
    def f_globals(self):
        return self._function.__globals__

    @_property
    def f_lineno(self):
        if self.f_lasti is not None:
            return _function_lineno(self._function, self.f_lasti)
        return -1

    # The trace function.
    @_property
    def f_trace(self):
        _unimplemented()

    # The lines of the functions in the trace.
    @_property
    def f_trace_lines(self):
        _unimplemented()

    # The last instructions executed in the trace.
    @_property
    def f_trace_opcodes(self):
        _unimplemented()

    def __eq__(self, other):
        _unimplemented()


def help(obj=_Unbound):
    _unimplemented()


def hex(number) -> str:
    _builtin()


def id(obj):
    _builtin()


def input(prompt=None):
    _unimplemented()


class instance_proxy(bootstrap=True):
    def __contains__(self, key):
        instance = self._instance
        _instance_guard(instance)
        return _instance_getattr(instance, key) is not _Unbound

    def __delitem__(self, key):
        instance = self._instance
        _instance_guard(instance)
        _instance_delattr(instance, key)

    def __eq__(self, other):
        if self is other:
            return True
        if not _dict_check(other) and _type(other) is not instance_proxy:
            return NotImplemented
        if len(self) != len(other):
            return False
        for key, value in self.items():
            other_value = other.get(key, _Unbound)
            if other_value is _Unbound or (
                value is not other_value and value != other_value
            ):
                return False
        return True

    def __getitem__(self, key):
        instance = self._instance
        _instance_guard(instance)
        result = _instance_getattr(instance, key)
        if result is _Unbound:
            raise KeyError(key)
        return result

    __hash__ = None

    def __init__(self, instance):
        _instance_guard(instance)
        self._instance = instance

    def __iter__(self):
        return self.keys().__iter__()

    def __len__(self):
        return len(self.keys())

    def __repr__(self):
        return _mapping_repr("instance_proxy({", self, "})")

    def __setitem__(self, key, value):
        instance = self._instance
        _instance_guard(instance)
        _instance_setattr(instance, key, value)

    def clear(self):
        instance = self._instance
        _instance_guard(instance)
        for key in _object_keys(instance):
            _instance_delattr(instance, key)

    def copy(self):
        instance = self._instance
        _instance_guard(instance)
        return {key: _instance_getattr(instance, key) for key in _object_keys(instance)}

    def update(self, d):
        instance = self._instance
        _instance_guard(instance)
        for key, value in d.items():
            _instance_setattr(instance, key, value)

    def get(self, key, default=None):
        instance = self._instance
        _instance_guard(instance)
        result = _instance_getattr(instance, key)
        if result is _Unbound:
            return default
        return result

    def items(self):
        # TODO(emacs): Return an iterator.
        instance = self._instance
        _instance_guard(instance)
        result = []
        for key in self.keys():
            value = _instance_getattr(instance, key)
            assert value is not _Unbound
            result.append((key, value))
        return result

    def keys(self):
        # TODO(emacs): Return an iterator.
        instance = self._instance
        _instance_guard(instance)
        return _object_keys(instance)

    def pop(self, key, default=_Unbound):
        instance = self._instance
        _instance_guard(instance)
        value = _instance_getattr(instance, key)
        if value is _Unbound:
            if default is _Unbound:
                raise KeyError(key)
            return default
        _instance_delattr(instance, key)
        return value

    def popitem(self):
        instance = self._instance
        _instance_guard(instance)
        keys = self.keys()
        if not keys:
            raise KeyError("dictionary is empty")
        key = keys[-1]
        value = _instance_getattr(instance, key)
        _instance_delattr(instance, key)
        return key, value

    def setdefault(self, key, default=None):
        instance = self._instance
        _instance_guard(instance)
        value = _instance_getattr(instance, key)
        if value is not _Unbound:
            return value
        _instance_setattr(instance, key, default)
        return default


class int(bootstrap=True):
    def __abs__(self) -> int:
        _builtin()

    def __add__(self, n: int) -> int:
        _builtin()

    def __and__(self, n: int) -> int:
        _builtin()

    def __bool__(self) -> bool:
        _builtin()

    def __ceil__(self) -> int:
        _builtin()

    def __divmod__(self, n: int):
        _builtin()

    def __eq__(self, n: int) -> bool:  # noqa: T484
        _builtin()

    def __float__(self) -> float:
        _builtin()

    def __floor__(self) -> int:
        _builtin()

    def __floordiv__(self, n: int) -> int:
        _builtin()

    def __format__(self, format_spec: str) -> str:
        _builtin()

    def __ge__(self, n: int) -> bool:
        _builtin()

    __getattribute__ = object.__getattribute__

    def __getnewargs__(self):
        _unimplemented()

    def __gt__(self, n: int) -> bool:
        _builtin()

    def __hash__(self) -> int:
        _builtin()

    def __index__(self) -> int:
        _builtin()

    def __int__(self) -> int:
        _builtin()

    def __invert__(self) -> int:
        _builtin()

    def __le__(self, n: int) -> bool:
        _builtin()

    def __lshift__(self, n: int) -> int:
        _builtin()

    def __lt__(self, n: int) -> bool:
        _builtin()

    def __mod__(self, n: int) -> int:
        _builtin()

    def __mul__(self, n: int) -> int:
        _builtin()

    def __ne__(self, n: int) -> int:  # noqa: T484
        _builtin()

    def __neg__(self) -> int:
        _builtin()

    def __new__(cls, x=_Unbound, base=_Unbound) -> int:  # noqa: C901
        if cls is bool:
            raise TypeError("int.__new__(bool) is not safe. Use bool.__new__()")
        _type_subclass_guard(cls, int)
        if x is _Unbound:
            if base is _Unbound:
                return _int_new_from_int(cls, 0)
            raise TypeError("int() missing string argument")
        if base is _Unbound:
            if _int_check_exact(x):
                return _int_new_from_int(cls, x)
            if _object_type_hasattr(x, "__int__"):
                return _int_new_from_int(cls, _int(x))
            dunder_trunc = _object_type_getattr(x, "__trunc__")
            if dunder_trunc is not _Unbound:
                result = dunder_trunc()
                if _int_check_exact(result) and cls is int:
                    return result
                if _int_check(result):
                    return _int_new_from_int(cls, result)
                if _object_type_hasattr(result, "__int__"):
                    return _int_new_from_int(cls, _int(result))
                raise TypeError(
                    f"__trunc__ returned non-Integral (type {_type(result).__name__})"
                )
            if _str_check(x):
                return _int_new_from_str(cls, x.strip(), 10)
            if _bytes_check(x):
                return _int_new_from_bytes(cls, x.strip(), 10)
            if _bytearray_check(x):
                return _int_new_from_bytearray(cls, x.strip(), 10)
            raise TypeError(
                f"int() argument must be a string, a bytes-like object "
                f"or a number, not {_type(x).__name__}"
            )
        base = _index(base)
        if base > 36 or (base < 2 and base != 0):
            raise ValueError("int() base must be >= 2 and <= 36")
        if _str_check(x):
            return _int_new_from_str(cls, x.strip(), base)
        if _bytes_check(x):
            return _int_new_from_bytes(cls, x.strip(), base)
        if _bytearray_check(x):
            return _int_new_from_bytearray(cls, x.strip(), base)
        raise TypeError("int() can't convert non-string with explicit base")

    def __or__(self, n: int) -> int:
        _builtin()

    def __pos__(self) -> int:
        _builtin()

    def __pow__(self, other, modulo=None) -> int:
        # TODO(T42359066): Re-write this in C++ if we need a speed boost.
        _int_guard(self)
        if not _int_check(other):
            return NotImplemented
        if modulo is not None and not _int_check(modulo):
            return NotImplemented
        if other < 0:
            if modulo is not None:
                raise ValueError(
                    "pow() 2nd argument cannot be negative when 3rd argument specified"
                )
            else:
                return float.__pow__(float(self), other)
        if other == 0:
            return 1
        if modulo == 1:
            return 0
        if modulo is None:
            result = self
            while int.__gt__(other, 1):
                result = int.__mul__(result, self)
                other = int.__sub__(other, 1)
        else:
            result = 1
            self = int.__mod__(self, modulo)
            while int.__gt__(other, 0):
                if int.__mod__(other, 2) == 1:
                    result = int.__mul__(result, self)
                    result = int.__mod__(result, modulo)
                other = int.__rshift__(other, 1)
                self = int.__mul__(self, self)
                self = int.__mod__(self, modulo)
        return result

    def __radd__(self, n: int) -> int:
        _int_guard(self)
        if not _int_check(n):
            return NotImplemented
        return int.__add__(n, self)

    def __rand__(self, n: int) -> int:
        _int_guard(self)
        if not _int_check(n):
            return NotImplemented
        return int.__and__(n, self)

    def __rdivmod__(self, n: int) -> int:
        _int_guard(self)
        if not _int_check(n):
            return NotImplemented
        return int.__divmod__(n, self)  # noqa: T484

    def __repr__(self) -> str:  # noqa: T484
        _builtin()

    def __rfloordiv__(self, n: int) -> int:
        _int_guard(self)
        if not _int_check(n):
            return NotImplemented
        return int.__floordiv__(n, self)  # noqa: T484

    def __rlshift__(self, n: int) -> int:
        _int_guard(self)
        if not _int_check(n):
            return NotImplemented
        return int.__lshift__(n, self)

    def __rmod__(self, n: int) -> int:
        _int_guard(self)
        if not _int_check(n):
            return NotImplemented
        return int.__mod__(n, self)  # noqa: T484

    def __rmul__(self, n: int) -> int:
        _int_guard(self)
        if not _int_check(n):
            return NotImplemented
        return int.__mul__(n, self)

    def __ror__(self, n: int) -> int:
        _int_guard(self)
        if not _int_check(n):
            return NotImplemented
        return int.__or__(n, self)

    def __round__(self) -> int:
        _builtin()

    def __rpow__(self, n: int, *, mod=None):
        _int_guard(self)
        if not _int_check(n):
            return NotImplemented
        return int.__pow__(n, self, modulo=mod)  # noqa: T484

    def __rrshift__(self, n: int) -> int:
        _int_guard(self)
        if not _int_check(n):
            return NotImplemented
        return int.__rshift__(n, self)

    def __rshift__(self, n: int) -> int:
        _builtin()

    def __rsub__(self, n: int) -> int:
        _int_guard(self)
        if not _int_check(n):
            return NotImplemented
        return int.__sub__(n, self)

    def __rtruediv__(self, n: int) -> int:
        _int_guard(self)
        if not _int_check(n):
            return NotImplemented
        return int.__truediv__(n, self)  # noqa: T484

    def __rxor__(self, n: int) -> int:
        _int_guard(self)
        if not _int_check(n):
            return NotImplemented
        return int.__xor__(n, self)

    def __str__(self) -> str:  # noqa: T484
        _builtin()

    def __sub__(self, n: int) -> int:
        _builtin()

    def __truediv__(self, other):
        _builtin()

    def __trunc__(self) -> int:
        _builtin()

    def __xor__(self, n: int) -> int:
        _builtin()

    def bit_length(self) -> int:
        _builtin()

    def conjugate(self) -> int:
        _builtin()

    @_property
    def denominator(self) -> int:
        return 1  # noqa: T484

    @classmethod
    def from_bytes(
        cls: type, bytes: bytes, byteorder: str, *, signed: bool = False
    ) -> int:
        _type_subclass_guard(cls, int)
        _str_guard(byteorder)
        if str.__eq__(byteorder, "little"):
            byteorder_big = False
        elif str.__eq__(byteorder, "big"):
            byteorder_big = True
        else:
            raise ValueError("byteorder must be either 'little' or 'big'")
        signed_bool = True if signed else False
        if _bytes_check(bytes):
            return _int_from_bytes(cls, bytes, byteorder_big, signed_bool)

        dunder_bytes = _object_type_getattr(bytes, "__bytes__")
        if dunder_bytes is _Unbound:
            bytes = _bytes_new(bytes)
            return _int_from_bytes(cls, bytes, byteorder_big, signed_bool)

        bytes = dunder_bytes()
        if not _bytes_check(bytes):
            raise TypeError(
                f"__bytes__ returned non-bytes (type {type(bytes).__name__})"
            )
        return _int_from_bytes(cls, bytes, byteorder_big, signed_bool)

    @_property
    def imag(self) -> int:
        return 0  # noqa: T484

    numerator = property(__int__)

    real = property(__int__)

    def to_bytes(self, length, byteorder, signed=False):
        _builtin()


def isinstance(obj, type_or_tuple) -> bool:
    ty = _type(obj)
    if ty is type_or_tuple:
        return True
    if _type_check_exact(type_or_tuple):
        return _isinstance_type(obj, ty, type_or_tuple)
    if _tuple_check(type_or_tuple):
        for item in type_or_tuple:
            if isinstance(obj, item):
                return True
        return False
    dunder_instancecheck = _object_type_getattr(type_or_tuple, "__instancecheck__")
    if dunder_instancecheck is not _Unbound:
        return bool(dunder_instancecheck(obj))
    # according to CPython, this is probably never reached anymore
    if _type_check(type_or_tuple):
        return _isinstance_type(obj, ty, type_or_tuple)
    _dunder_bases_tuple_check(
        type_or_tuple, "isinstance() arg 2 must be a type or tuple of types"
    )
    subclass = getattr(obj, "__class__", None)
    return subclass and _issubclass_recursive(subclass, type_or_tuple)


def issubclass(cls, type_or_tuple) -> bool:
    if _type_check_exact(type_or_tuple):
        if _type_check(cls):
            return _type_issubclass(cls, type_or_tuple)
        return _issubclass(cls, type_or_tuple)
    if _tuple_check(type_or_tuple):
        for item in type_or_tuple:
            if issubclass(cls, item):
                return True
        return False
    dunder_subclasscheck = _object_type_getattr(type_or_tuple, "__subclasscheck__")
    if dunder_subclasscheck is not _Unbound:
        return bool(dunder_subclasscheck(cls))
    # according to CPython, this is probably never reached anymore
    return _issubclass(cls, type_or_tuple)


class callable_iterator:
    def __init__(self, callable, sentinel):
        self.__callable = callable
        self.__sentinel = sentinel

    def __iter__(self):
        return self

    def __next__(self):
        value = self.__callable()
        if self.__sentinel == value:
            raise StopIteration()
        return value

    def __reduce__(self):
        return (iter, (self.__callable, self.__sentinel))


def iter(obj, sentinel=None):
    if sentinel is None:
        return _iter(obj)
    return callable_iterator(obj, sentinel)


class iterator(bootstrap=True):
    def __init__(self, iterable):
        _seq_set_iterable(self, iterable)
        _seq_set_index(self, 0)

    def __iter__(self):
        return self

    def __next__(self):
        try:
            index = _seq_index(self)
            val = _seq_iterable(self)[index]
            _seq_set_index(self, index + 1)
            return val
        except IndexError:
            raise StopIteration()


def len(seq):
    dunder_len = _object_type_getattr(seq, "__len__")
    if dunder_len is _Unbound:
        raise TypeError(f"object of type '{_type(seq).__name__}' has no len()")
    return _index(dunder_len())


license = ""


class list(bootstrap=True):
    def __add__(self, other):
        _builtin()

    def __contains__(self, value):
        _builtin()

    def __delitem__(self, key) -> None:
        _list_guard(self)
        if _int_check(key):
            return _list_delitem(self, key)
        if _slice_check(key):
            step = _slice_step(_slice_index(key.step))
            length = _list_len(self)
            start = _slice_start(_slice_index(key.start), step, length)
            stop = _slice_stop(_slice_index(key.stop), step, length)
            return _list_delslice(self, start, stop, step)
        if _object_type_hasattr(key, "__index__"):
            return _list_delitem(self, _index(key))
        raise TypeError("list indices must be integers or slices")

    def __eq__(self, other):
        _list_guard(self)
        if not _list_check(other):
            return NotImplemented

        if self is other:
            return True
        length = _list_len(self)
        if length != _list_len(other):
            return False
        i = 0
        while i < length:
            lhs = _list_getitem(self, i)
            rhs = _list_getitem(other, i)
            if lhs is not rhs and not (lhs == rhs):
                return False
            i += 1
        return True

    def __ge__(self, other):
        _list_guard(self)
        if not _list_check(other):
            return NotImplemented
        i = 0
        len_self = _list_len(self)
        len_other = _list_len(other)
        min_len = len_self if len_self < len_other else len_other
        while i < min_len:
            self_item = _list_getitem(self, i)
            other_item = _list_getitem(other, i)
            if self_item is not other_item and not self_item == other_item:
                return self_item >= other_item
            i += 1
        # If the items are all up equal up to min_len, compare lengths
        return len_self >= len_other

    __getattribute__ = object.__getattribute__

    def __getitem__(self, key):
        result = _list_getitem(self, key)
        if result is not _Unbound:
            return result
        if _slice_check(key):
            step = _slice_step(_slice_index(key.step))
            length = _list_len(self)
            start = _slice_start(_slice_index(key.start), step, length)
            stop = _slice_stop(_slice_index(key.stop), step, length)
            return _list_getslice(self, start, stop, step)
        if _object_type_hasattr(key, "__index__"):
            return _list_getitem(self, _index(key))
        raise TypeError(
            f"list indices must be integers or slices, not {_type(key).__name__}"
        )

    def __gt__(self, other):
        _list_guard(self)
        if not _list_check(other):
            return NotImplemented
        i = 0
        len_self = _list_len(self)
        len_other = _list_len(other)
        min_len = len_self if len_self < len_other else len_other
        while i < min_len:
            self_item = _list_getitem(self, i)
            other_item = _list_getitem(other, i)
            if self_item is not other_item and not self_item == other_item:
                return self_item > other_item
            i += 1
        # If the items are all up equal up to min_len, compare lengths
        return len_self > len_other

    __hash__ = None

    def __iadd__(self, other):
        result = _list_extend(self, other)
        if result is not _Unbound:
            return self
        _list_extend(self, (*other,))
        return self

    def __imul__(self, other):
        _builtin()

    def __init__(self, iterable=()):
        self.extend(iterable)

    def __iter__(self):
        _builtin()

    def __le__(self, other):
        _list_guard(self)
        if not _list_check(other):
            return NotImplemented
        i = 0
        len_self = _list_len(self)
        len_other = _list_len(other)
        min_len = len_self if len_self < len_other else len_other
        while i < min_len:
            self_item = _list_getitem(self, i)
            other_item = _list_getitem(other, i)
            if self_item is not other_item and not self_item == other_item:
                return self_item <= other_item
            i += 1
        # If the items are all up equal up to min_len, compare lengths
        return len_self <= len_other

    def __len__(self):
        _builtin()

    def __lt__(self, other):
        _list_guard(self)
        if not _list_check(other):
            return NotImplemented
        i = 0
        len_self = _list_len(self)
        len_other = _list_len(other)
        min_len = len_self if len_self < len_other else len_other
        while i < min_len:
            self_item = _list_getitem(self, i)
            other_item = _list_getitem(other, i)
            if self_item is not other_item and not self_item == other_item:
                return self_item < other_item
            i += 1
        # If the items are all up equal up to min_len, compare lengths
        return len_self < len_other

    def __mul__(self, other):
        _builtin()

    def __ne__(self, other):
        eq_result = list.__eq__(self, other)
        if eq_result is NotImplemented:
            return NotImplemented
        return not eq_result

    def __new__(cls, iterable=()):
        _builtin()

    def __repr__(self):
        return _sequence_repr("[", self, "]")

    def __reversed__(self):
        return iter(self[::-1])

    def __rmul__(self, other):
        _unimplemented()

    def __setitem__(self, key, value):
        result = _list_setitem(self, key, value)
        if result is not _Unbound:
            return result
        if _slice_check(key):
            step = _slice_step(_slice_index(key.step))
            length = _list_len(self)
            start = _slice_start(_slice_index(key.start), step, length)
            stop = _slice_stop(_slice_index(key.stop), step, length)
            result = _list_setslice(self, start, stop, step, value)
            if result is not _Unbound:
                return result
            try:
                value = (*value,)
            except TypeError:
                raise TypeError("can only assign an iterable")
            return _list_setslice(self, start, stop, step, value)
        if _object_type_hasattr(key, "__index__"):
            return _list_setitem(self, _index(key), value)
        raise TypeError(
            f"list indices must be integers or slices, not {_type(key).__name__}"
        )

    def append(self, other):
        _builtin()

    def clear(self):
        _builtin()

    def copy(self):
        _list_guard(self)
        return list(self)

    def count(self, value):
        _list_guard(self)
        i = 0
        length = _list_len(self)
        result = 0
        while i < length:
            item = _list_getitem(self, i)
            if item is value or item == value:
                result += 1
            i += 1
        return result

    def extend(self, other):
        result = _list_extend(self, other)
        if result is not _Unbound:
            return
        # NOTE: this loop prevents recursion in (*other,)
        for item in other:
            list.append(self, item)

    def index(self, obj, start=0, end=_Unbound):
        _list_guard(self)
        length = _list_len(self)
        # enforce type int to avoid custom comparators
        i = (
            start
            if _int_check_exact(start)
            else _int_new_from_int(int, _slice_index_not_none(start))
        )
        if end is _Unbound:
            end = length
        elif not _int_check_exact(end):
            end = _int_new_from_int(int, _slice_index_not_none(end))
        if i < 0:
            i += length
            if i < 0:
                i = 0
        if end < 0:
            end += length
            if end < 0:
                end = 0
        while i < end:
            item = _list_getitem(self, i)
            if item is obj or item == obj:
                return i
            i += 1
        raise ValueError(f"{repr(obj)} is not in list")

    def insert(self, index, value):
        _builtin()

    def pop(self, index=_Unbound):
        _builtin()

    def remove(self, value):
        _builtin()

    def reverse(self):
        _list_guard(self)
        length = _list_len(self)
        if length < 2:
            return
        left = 0
        right = length - 1
        while left < right:
            _list_swap(self, left, right)
            left += 1
            right -= 1

    def sort(self, key=None, reverse=False):
        _list_guard(self)
        if reverse:
            list.reverse(self)
        if key:
            i = 0
            length = _list_len(self)
            while i < length:
                item = _list_getitem(self, i)
                list.__setitem__(self, i, (key(item), item))
                i += 1
        _list_sort(self)
        if key:
            i = 0
            while i < length:
                item = _list_getitem(self, i)
                list.__setitem__(self, i, item[1])
                i += 1
        if reverse:
            list.reverse(self)


class list_iterator(bootstrap=True):
    def __iter__(self):
        _builtin()

    def __length_hint__(self):
        _builtin()

    def __next__(self):
        _builtin()


def locals():
    return _caller_locals()


class longrange_iterator(bootstrap=True):
    def __iter__(self):
        _builtin()

    def __length_hint__(self):
        _builtin()

    def __next__(self):
        _builtin()


class map:
    __getattribute__ = object.__getattribute__

    def __init__(self, func, *iterables):
        if len(iterables) < 1:
            raise TypeError("map() must have at least two arguments.")
        self.func = func
        self.iters = tuple(iter(iterable) for iterable in iterables)
        self.len_iters = len(self.iters)

    def __iter__(self):
        return self

    def __next__(self):
        func = self.func
        if self.len_iters == 1:
            return func(next(self.iters[0]))
        return func(*[next(iter) for iter in self.iters])

    def __reduce__(self):
        _unimplemented()


class mappingproxy(bootstrap=True):
    def __contains__(self, key):
        _mappingproxy_guard(self)
        return key in _mappingproxy_mapping(self)

    def __eq__(self, other):
        _mappingproxy_guard(self)
        return _mappingproxy_mapping(self) == other

    def __getitem__(self, key):
        _mappingproxy_guard(self)
        return _mappingproxy_mapping(self)[key]

    def __init__(self, mapping):
        _mappingproxy_guard(self)
        if _dict_check(mapping) or _type_proxy_check(mapping):
            _mappingproxy_set_mapping(self, mapping)
            return

        from collections.abc import Mapping

        if not isinstance(mapping, Mapping):
            raise TypeError(
                f"mappingproxy() argument must be a mapping, "
                f"not {_type(mapping).__name__}"
            )
        _mappingproxy_set_mapping(self, mapping)

    def __iter__(self):
        _mappingproxy_guard(self)
        return iter(_mappingproxy_mapping(self))

    def __len__(self):
        _mappingproxy_guard(self)
        return len(_mappingproxy_mapping(self))

    def __repr__(self):
        _mappingproxy_guard(self)
        enclosed_result = repr(_mappingproxy_mapping(self))  # noqa: F841
        return f"mappingproxy({enclosed_result})"

    def copy(self):
        _mappingproxy_guard(self)
        return _mappingproxy_mapping(self).copy()

    def get(self, key, default=None):
        _mappingproxy_guard(self)
        return _mappingproxy_mapping(self).get(key, default)

    def items(self):
        _mappingproxy_guard(self)
        return _mappingproxy_mapping(self).items()

    def keys(self):
        _mappingproxy_guard(self)
        return _mappingproxy_mapping(self).keys()

    def values(self):
        _mappingproxy_guard(self)
        return _mappingproxy_mapping(self).values()


def max(arg1, arg2=_Unbound, *args, key=_Unbound, default=_Unbound):  # noqa: C901
    if arg2 is not _Unbound:
        if default is not _Unbound:
            raise TypeError(
                "Cannot specify a default for max() with multiple positional arguments"
            )
        # compare positional arguments and varargs
        if key is _Unbound:
            # using argument values
            result = arg2 if arg2 > arg1 else arg1
            if args:
                for item in args:
                    result = item if item > result else result
        else:
            # using ordering function values
            key1 = key(arg1)
            key2 = key(arg2)
            key_max = key2 if key2 > key1 else key1
            result = arg1 if key_max is key1 else arg2
            if args:
                for item in args:
                    key_item = key(item)
                    if key_item > key_max:
                        key_max, result = key_item, item
    else:
        # compare iterable items
        result = default
        if key is _Unbound:
            # using iterable item values
            for item in arg1:
                if result is _Unbound or item > result:
                    result = item
        else:
            # using ordering function values
            key_max = _Unbound
            for item in arg1:
                key_item = key(item)
                if key_max is _Unbound or key_item > key_max:
                    key_max = key_item
                    result = item
        if result is _Unbound:
            raise ValueError("max() arg is an empty sequence")
    return result


class method(bootstrap=True):
    @_property
    def __doc__(self):
        return self.__func__.__doc__

    def __call__(self, *args, **kwargs):
        return self.__func__(self.__self__, *args, **kwargs)

    def __getattr__(self, name):
        return getattr(self.__func__, name)

    def __new__(cls, func, self):
        if not callable(func):
            raise TypeError("first argument must be callable")
        if self is None:
            raise TypeError("self must not be None")
        return _bound_method(func, self)


class memoryview(bootstrap=True):
    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, exc_tb):
        memoryview.release(self)

    def __getitem__(self, key):
        result = _memoryview_getitem(self, key)
        if result is not _Unbound:
            return result
        if _slice_check(key):
            step = _slice_step(_slice_index(key.step))
            length = memoryview.__len__(self)
            start = _slice_start(_slice_index(key.start), step, length)
            stop = _slice_stop(_slice_index(key.stop), step, length)
            return _memoryview_getslice(self, start, stop, step)
        if _object_type_hasattr(key, "__index__"):
            return _memoryview_getitem(self, _index(key))
        raise TypeError(
            f"memoryview indices must be integers or slices, "
            f"not {_type(key).__name__}"
        )

    def __len__(self) -> int:
        _builtin()

    def __new__(cls, object):
        _builtin()

    def __setitem__(self, key, value):
        result = _memoryview_setitem(self, key, value)
        if result is not _Unbound:
            return result
        if _int_check(key) or _object_type_hasattr(key, "__index__"):
            key = _index(key)
            fmt = self.format
            if fmt == "f" or fmt == "d":
                try:
                    val_float = float(value)
                except TypeError:
                    raise TypeError(f"memoryview: invalid type for format '{fmt}'")
                return _memoryview_setitem(self, key, val_float)
            elif fmt == "?":
                return _memoryview_setitem(self, key, bool(value))
            else:
                try:
                    val_int = _index(value)
                except TypeError:
                    raise TypeError(f"memoryview: invalid type for format '{fmt}'")
                return _memoryview_setitem(self, key, val_int)
        if _slice_check(key):
            _byteslike_guard(value)
            step = _slice_step(_slice_index(key.step))
            length = memoryview.__len__(self)
            start = _slice_start(_slice_index(key.start), step, length)
            stop = _slice_stop(_slice_index(key.stop), step, length)
            return _memoryview_setslice(self, start, stop, step, value)
        raise TypeError("memoryview: invalid slice key")

    def cast(self, format: str) -> memoryview:
        _builtin()

    itemsize = _property(_memoryview_itemsize)

    nbytes = _property(_memoryview_nbytes)

    def release(self):
        # Do nothing.
        pass

    def tolist(self):
        _memoryview_guard(self)
        return [*self]


def min(arg1, arg2=_Unbound, *args, key=_Unbound, default=_Unbound):  # noqa: C901
    # NOTE: Sync this with max() since min() is copied from max().
    if arg2 is not _Unbound:
        if default is not _Unbound:
            raise TypeError(
                "Cannot specify a default for min() with multiple positional arguments"
            )
        # compare positional arguments and varargs
        if key is _Unbound:
            # using argument values
            result = arg2 if arg2 < arg1 else arg1
            if args:
                for item in args:
                    result = item if item < result else result
        else:
            # using ordering function values
            key1 = key(arg1)
            key2 = key(arg2)
            key_max = key2 if key2 < key1 else key1
            result = arg1 if key_max is key1 else arg2
            if args:
                for item in args:
                    key_item = key(item)
                    if key_item < key_max:
                        key_max, result = key_item, item
    else:
        # compare iterable items
        result = default
        if key is _Unbound:
            # using iterable item values
            for item in arg1:
                if result is _Unbound or item < result:
                    result = item
        else:
            # using ordering function values
            key_max = _Unbound
            for item in arg1:
                key_item = key(item)
                if key_max is _Unbound or key_item < key_max:
                    key_max = key_item
                    result = item
        if result is _Unbound:
            raise ValueError("min() arg is an empty sequence")
    return result


# This is just a placeholder implementation to be patched by
# importlib/_bootstrap.py for its _module_repr implementation.
def _module_repr(module):
    _unimplemented()


class module(bootstrap=True):
    __dict__ = _property(_module_proxy)

    def __dir__(self):
        if not isinstance(self, module):
            raise TypeError(
                f"'__dir__' requires a 'module' object "
                "but received a '{_type(self).__name__}'"
            )
        return _module_dir(self)

    def __getattribute__(self, name):
        _builtin()

    def __init__(self, name):
        _builtin()

    def __new__(cls, *args, **kwargs):
        _builtin()

    def __repr__(self):
        return _module_repr(self)

    def __setattr__(self, name, value):
        _builtin()


class module_proxy(bootstrap=True):
    def __contains__(self, key):
        _builtin()

    def __delitem__(self, key):
        _builtin()

    def __eq__(self, other):
        _unimplemented()

    def __getitem__(self, key):
        _builtin()

    __hash__ = None

    # module_proxy is not designed to be instantiated by the managed code.
    __init__ = None

    def __iter__(self):
        # TODO(T50379702): Return an iterable to avoid materializing a list of keys.
        return iter(_module_proxy_keys(self))

    def __len__(self):
        _builtin()

    # module_proxy is not designed to be subclassed.
    __new__ = None

    def __repr__(self):
        _module_proxy_guard(self)
        return _mapping_repr("{", self, "}")

    def __setitem__(self, key, value):
        _module_proxy_setitem(self, key, value)

    def clear(self):
        _unimplemented()

    def copy(self):
        # TODO(T50379702): Return an iterable to avoid materializing the list of items.
        keys = _module_proxy_keys(self)
        values = _module_proxy_values(self)
        return {keys[i]: values[i] for i in range(len(keys))}

    def get(self, key, default=None):
        _builtin()

    def items(self):
        # TODO(T50379702): Return an iterable to avoid materializing the list of items.
        keys = _module_proxy_keys(self)
        values = _module_proxy_values(self)
        return [(keys[i], values[i]) for i in range(len(keys))]

    def keys(self):
        # TODO(T50379702): Return an iterable to avoid materializing the list of keys.
        return _module_proxy_keys(self)

    def pop(self, key, default=_Unbound):
        _builtin()

    def setdefault(self, key, default=None):
        _builtin()

    @_positional_only(2)
    def update(self, other=_Unbound, **kwargs):
        _module_proxy_guard(self)
        if _dict_check_exact(other):
            for key, value in other.items():
                _module_proxy_setitem(self, key, value)
        elif hasattr(other, "keys"):
            for key in other.keys():
                _module_proxy_setitem(self, key, other[key])
        elif other is not _Unbound:
            num_items = 0
            for x in other:
                item = tuple(x)
                if _tuple_len(item) != 2:
                    raise ValueError(
                        f"dictionary update sequence element #{num_items} has "
                        f"length {_tuple_len(item)}; 2 is required"
                    )
                _module_proxy_setitem(self, *item)
                num_items += 1
        for key, value in kwargs.items():
            _module_proxy_setitem(self, key, value)

    def values(self):
        # TODO(T50379702): Return an iterable to avoid materializing the list of values.
        return _module_proxy_values(self)


def next(iterator, default=_Unbound):
    dunder_next = _object_type_getattr(iterator, "__next__")
    if dunder_next is _Unbound:
        raise TypeError(f"'{_type(iterator).__name__}' object is not iterable")
    try:
        return dunder_next()
    except StopIteration:
        if default is _Unbound:
            raise
        return default


def oct(number) -> str:
    _builtin()


def ord(c):
    _builtin()


def pow(x, y, z=None):
    if z is None:
        return x ** y
    dunder_pow = _object_type_getattr(x, "__pow__")
    if dunder_pow is not _Unbound:
        result = dunder_pow(y, z)
        if result is not NotImplemented:
            return result
    raise TypeError(
        f"unsupported operand type(s) for pow(): '{_type(x).__name__}', "
        f"'{_type(y).__name__}', '{_type(z).__name__}'"
    )


def print(*args, sep=" ", end="\n", file=None, flush=False):
    if file is None:
        try:
            file = _sys.stdout
        except AttributeError:
            raise RuntimeError("lost _sys.stdout")
        if file is None:
            return
    if args:
        file.write(str(args[0]))
        length = len(args)
        i = 1
        while i < length:
            file.write(sep)
            file.write(str(args[i]))
            i += 1
    file.write(end)
    if flush:
        file.flush()


def quit():
    _unimplemented()


class range(bootstrap=True):
    def __bool__(self):
        _range_guard(self)
        return bool(_range_len(self))

    __getattribute__ = object.__getattribute__

    def __contains__(self, num):
        _range_guard(self)
        if _int_check_exact(num) or _bool_check(num):
            start = self.start
            stop = self.stop
            step = self.step
            if step is 1:  # noqa: F632
                # Fast path; only two comparisons required
                return start <= num < stop
            if step > 0:
                if num < start or num >= stop:
                    return False
            elif num > start or num <= stop:
                return False
            return (num - start) % step == 0

        return num in self.__iter__()

    def __eq__(self, other):
        _range_guard(self)
        if not _range_check(other):
            return NotImplemented
        if self is other:
            return True
        self_len = _range_len(self)
        if not self_len == _range_len(other):
            return False
        if not self_len:
            return True
        if self.start != other.start:
            return False
        if self_len == 1:
            return True
        return self.step == other.step

    def __ge__(self, other):
        _range_guard(self)
        return NotImplemented

    def __getitem__(self, key):
        if not _range_check(self):
            raise TypeError(
                "'__getitem__' requires a 'range' object but received a "
                f"'{_type(self).__name__}'"
            )
        if _int_check(key):
            return _range_getitem(self, key)
        if _slice_check(key):
            step = _slice_step_long(_slice_index(key.step))
            length = _range_len(self)
            start = _slice_start_long(_slice_index(key.start), step, length)
            stop = _slice_stop_long(_slice_index(key.stop), step, length)
            return _range_getslice(self, start, stop, step)
        if _object_type_hasattr(key, "__index__"):
            return _range_getitem(self, _index(key))
        raise TypeError(
            f"range indices must be integers or slices, not {_type(key).__name__}"
        )

    def __gt__(self, other):
        _range_guard(self)
        return NotImplemented

    def __hash__(self):
        _range_guard(self)
        range_len = _range_len(self)
        if not range_len:
            return tuple.__hash__((range_len, None, None))
        if range_len == 1:
            return tuple.__hash__((range_len, self.start, None))
        return tuple.__hash__((range_len, self.start, self.step))

    def __iter__(self):
        _builtin()

    def __le__(self, other):
        _range_guard(self)
        return NotImplemented

    def __len__(self):
        _builtin()

    def __lt__(self, other):
        _range_guard(self)
        return NotImplemented

    def __ne__(self, other):
        _range_guard(self)
        if not _range_check(other):
            return NotImplemented
        if self is other:
            return False
        self_len = _range_len(self)
        if self_len != _range_len(other):
            return True
        if not self_len:
            return False
        if self.start != other.start:
            return True
        if self_len == 1:
            return False
        return not self.step == other.step

    def __new__(cls, start_or_stop, stop=_Unbound, step=_Unbound):
        _builtin()

    def __reduce__(self):
        return (range, (self.start, self.stop, self.step))

    def __repr__(self):
        if self.step == 1:
            return f"range({self.start!r}, {self.stop!r})"
        return f"range({self.start!r}, {self.stop!r}, {self.step!r})"

    def __reversed__(self):
        _range_guard(self)
        self_len = _range_len(self)
        new_stop = self.start - self.step
        new_start = new_stop + (self_len * self.step)
        return range.__iter__(range(new_start, new_stop, -self.step))

    def __setstate__(self):
        _unimplemented()

    def count(self, value):
        _range_guard(self)
        if _int_check_exact(value) or _bool_check(value):
            return 1 if range.__contains__(self, value) else 0
        seen = 0
        for i in self:
            if i == value:
                seen += 1
        return seen

    def index(self, value):
        _range_guard(self)
        if _int_check_exact(value) or _bool_check(value):
            if range.__contains__(self, value):
                return int.__floordiv__(value - self.start, self.step)
        else:
            i = 0
            for element in self:
                if element == value:
                    return i
                i += 1
        raise ValueError(f"{value} is not in range")


class range_iterator(bootstrap=True):
    def __iter__(self):
        _builtin()

    def __length_hint__(self):
        _builtin()

    def __next__(self):
        _builtin()


def repr(obj):
    result = _object_type_getattr(obj, "__repr__")()
    if not _str_check(result):
        raise TypeError("__repr__ returned non-string")
    return result


class reversed:
    __getattribute__ = object.__getattribute__

    def __iter__(self):
        return self

    def __new__(cls, seq, **kwargs):
        dunder_reversed = _object_type_getattr(seq, "__reversed__")
        if dunder_reversed is None:
            raise TypeError(f"'{_type(seq).__name__}' object is not reversible")
        if dunder_reversed is not _Unbound:
            return dunder_reversed()
        if _object_type_getattr(seq, "__getitem__") is _Unbound:
            raise TypeError(f"'{_type(seq).__name__}' object is not reversible")
        return object.__new__(cls)

    def __init__(self, seq):
        self.remaining = len(seq)
        self.seq = seq

    def __next__(self):
        i = self.remaining - 1
        if i >= 0:
            try:
                item = self.seq[i]
                self.remaining = i
                return item
            except (IndexError, StopIteration):
                pass
        self.remaining = -1
        raise StopIteration

    def __length_hint__(self):
        return self.remaining

    def __reduce__(self):
        _unimplemented()

    def __setstate__(self):
        _unimplemented()


def round(number, ndigits=None):
    dunder_round = _object_type_getattr(number, "__round__")
    if dunder_round is _Unbound:
        raise TypeError(
            f"type {_type(number).__name__} doesn't define __round__ method"
        )
    if ndigits is None:
        return dunder_round()
    return dunder_round(ndigits)


class set(bootstrap=True):
    def __and__(self, other):
        _builtin()

    def __contains__(self, value):
        _builtin()

    def __eq__(self, other):
        _builtin()

    def __ge__(self, other):
        _builtin()

    __getattribute__ = object.__getattribute__

    def __gt__(self, other):
        _builtin()

    __hash__ = None

    def __iand__(self, other):
        _builtin()

    def __init__(self, iterable=()):
        _builtin()

    def __ior__(self, other):
        _set_guard(self)
        if not _anyset_check(other):
            return NotImplemented
        if self is other:
            return self
        set.update(self, other)
        return self

    def __isub__(self, other):
        _set_guard(self)
        if not _anyset_check(other):
            return NotImplemented
        set.difference_update(self, other)
        return self

    def __iter__(self):
        _builtin()

    def __ixor__(self, other):
        _set_guard(self)
        if not _anyset_check(other):
            return NotImplemented
        set.symmetric_difference_update(self, other)
        return self

    def __le__(self, other):
        _builtin()

    def __len__(self):
        _builtin()

    def __lt__(self, other):
        _builtin()

    def __ne__(self, other):
        _builtin()

    def __new__(cls, iterable=()):
        _builtin()

    def __or__(self, other):
        _set_guard(self)
        if not _anyset_check(other):
            return NotImplemented
        result = set.copy(self)
        if self is other:
            return result
        set.update(result, other)
        return result

    def __rand__(self, other):
        _unimplemented()

    def __reduce__(self):
        _unimplemented()

    def __repr__(self):
        _set_guard(self)
        if _repr_enter(self):
            return f"{_type(self).__name__}(...)"
        if _set_len(self) == 0:
            _repr_leave(self)
            return f"{_type(self).__name__}()"
        result = f"{{{', '.join([repr(item) for item in self])}}}"
        _repr_leave(self)
        return result

    def __ror__(self, other):
        _unimplemented()

    def __rsub__(self, other):
        _unimplemented()

    def __rxor__(self, other):
        _set_guard(self)
        if not _anyset_check(other):
            return NotImplemented
        return set.symmetric_difference(self, other)

    def __sub__(self, other):
        _set_guard(self)
        if not _anyset_check(other):
            return NotImplemented
        return set.difference(self, other)

    def __xor__(self, other):
        _set_guard(self)
        if not _anyset_check(other):
            return NotImplemented
        return set.symmetric_difference(self, other)

    def add(self, value):
        _builtin()

    def clear(self):
        _builtin()

    def copy(self):
        _builtin()

    def difference(self, *others):
        _set_guard(self)
        result = list(self)
        for other in others:
            diff = []
            for item in result:
                if item not in other:
                    diff.append(item)
            result = diff
        return set(result)

    def difference_update(self, *others):
        _set_guard(self)
        for other in others:
            for item in other:
                set.discard(self, item)

    def discard(self, elem):
        _builtin()

    def intersection(self, other):
        _builtin()

    def intersection_update(self, *others):
        _set_guard(self)
        for other in others:
            set.__iand__(self, set(other))

    def isdisjoint(self, other):
        _builtin()

    def issubset(self, other):
        _unimplemented()

    def issuperset(self, other):
        _set_guard(self)
        if not _anyset_check(other):
            other = set(other)
        return set.__ge__(self, other)

    def pop(self):
        _builtin()

    def remove(self, elt):
        _builtin()

    def symmetric_difference(self, other):
        _set_guard(self)
        result = set(other)
        for item in self:
            if set.__contains__(result, item):
                set.remove(result, item)
            else:
                set.add(result, item)
        return result

    def symmetric_difference_update(self, other):
        _set_guard(self)
        if self is other:
            set.clear(self)
            return
        other = set(other)
        for item in other:
            if set.__contains__(self, item):
                set.remove(self, item)
            else:
                set.add(self, item)

    def union(self, *others):
        if not _set_check(self):
            raise TypeError(
                "'union' requires a 'set' object but received a "
                f"'{_type(self).__name__}'"
            )
        result = set.copy(self)
        for item in others:
            if item is self:
                continue
            set.update(result, item)
        return result

    def update(self, *args):
        _builtin()


class set_iterator(bootstrap=True):
    def __iter__(self):
        _builtin()

    def __length_hint__(self):
        _builtin()

    def __next__(self):
        _builtin()


def setattr(obj, name, value):
    _builtin()


class slice(bootstrap=True):
    def __eq__(self, other):
        _unimplemented()

    def __ge__(self, other):
        _unimplemented()

    __getattribute__ = object.__getattribute__

    def __gt__(self, other):
        _unimplemented()

    __hash__ = None

    def __le__(self, other):
        _unimplemented()

    def __lt__(self, other):
        _unimplemented()

    def __ne__(self, other):
        _unimplemented()

    def __new__(cls, start_or_stop, stop=_Unbound, step=None):
        _builtin()

    def __reduce__(self):
        _unimplemented()

    def __repr__(self):
        return f"slice({self.start!r}, {self.stop!r}, {self.step!r})"

    def indices(self, length) -> tuple:  # noqa: C901
        _slice_guard(self)
        length = _index(length)
        if length < 0:
            raise ValueError("length should not be negative")
        step = _slice_step_long(_slice_index(self.step))
        start = _slice_start_long(_slice_index(self.start), step, length)
        stop = _slice_stop_long(_slice_index(self.stop), step, length)
        return start, stop, step


def sorted(iterable, *, key=None, reverse=False):
    result = list(iterable)
    result.sort(key=key, reverse=reverse)
    return result


class str(bootstrap=True):
    def __add__(self, other):
        _builtin()

    def __bool__(self):
        _builtin()

    def __contains__(self, other):
        _builtin()

    def __eq__(self, other):
        _builtin()

    def __format__(self, format_spec: str) -> str:
        _builtin()

    def __ge__(self, other):
        _builtin()

    __getattribute__ = object.__getattribute__

    def __getitem__(self, key):
        result = _str_getitem(self, key)
        if result is not _Unbound:
            return result
        if _slice_check(key):
            step = _slice_step(_slice_index(key.step))
            length = _str_len(self)
            start = _slice_start(_slice_index(key.start), step, length)
            stop = _slice_stop(_slice_index(key.stop), step, length)
            return _str_getslice(self, start, stop, step)
        if _object_type_hasattr(key, "__index__"):
            return _str_getitem(self, _index(key))
        raise TypeError(
            f"string indices must be integers or slices, not {_type(key).__name__}"
        )

    def __getnewargs__(self):
        _unimplemented()

    def __gt__(self, other):
        _builtin()

    def __hash__(self):
        _builtin()

    def __iter__(self):
        _builtin()

    def __le__(self, other):
        _builtin()

    def __len__(self) -> int:
        _builtin()

    def __lt__(self, other):
        _builtin()

    def __mod__(self, other):
        result = _str_mod_fast_path(self, other)
        if result is not _Unbound:
            return result

        if not _str_check(self):
            raise TypeError(
                f"'__mod__' requires a 'str' object "
                f"but received a '{_type(self).__name__}'"
            )
        return _str_mod_format(self, other)

    def __mul__(self, n: int) -> str:
        _builtin()

    def __ne__(self, other):
        _builtin()

    def __new__(cls, obj=_Unbound, encoding=_Unbound, errors=_Unbound):  # noqa: C901
        _type_subclass_guard(cls, str)
        if obj is _Unbound:
            return _str_from_str(cls, "")
        if encoding is _Unbound and errors is _Unbound:
            if _str_check_exact(obj):
                return _str_from_str(cls, obj)
            dunder_str = _object_type_getattr(obj, "__str__")
            if dunder_str is _Unbound:
                return _str_from_str(cls, _type(obj).__repr__(obj))
            result = dunder_str()
            if not _str_check(result):
                raise TypeError(f"__str__ returned non-string '{_type(obj).__name__}'")
            return _str_from_str(cls, result)
        if _str_check(obj):
            raise TypeError("decoding str is not supported")
        if not _byteslike_check(obj):
            raise TypeError(
                "decoding to str: need a bytes-like object, "
                f"'{_type(obj).__name__}' found"
            )

        if encoding is _Unbound:
            result = _bytes_decode_utf_8(obj)
        else:
            result = _bytes_decode(obj, encoding)
        if result is not _Unbound:
            if cls is str:
                return result
            return _str_from_str(cls, result)
        if errors is _Unbound:
            return _str_from_str(cls, _codecs.decode(obj, encoding))
        if encoding is _Unbound:
            return _str_from_str(cls, _codecs.decode(obj, errors=errors))
        return _str_from_str(cls, _codecs.decode(obj, encoding, errors))

    def __repr__(self):
        _builtin()

    def __rmod__(self, n):
        _unimplemented()

    def __rmul__(self, n: int) -> str:
        _str_guard(self)
        return str.__mul__(self, n)

    def __str__(self):
        return self

    def _mod_check_single_arg(self, value):
        """Helper function used by the compiler when transforming some
        printf-style formatting to f-strings."""
        if _tuple_check(value):
            len = _tuple_len(value)
            if len == 0:
                raise TypeError("not enough arguments for format string")
            if len > 1:
                raise TypeError("not all arguments converted during string formatting")
            return value
        return (value,)

    def _mod_convert_number(self, value):
        """Helper function used by the compiler when transforming some
        printf-style formatting to f-strings."""
        if not _number_check(value):
            raise TypeError(f"format requires a number, not {_type(value).__name__}")
        return _int(value)

    def capitalize(self):
        _str_guard(self)
        if not self:
            return self
        first_letter = str.upper(_str_getitem(self, 0).upper())
        lowercase_str = str.lower(_str_getslice(self, 1, _str_len(self), 1))
        return first_letter + lowercase_str

    def casefold(self):
        _unimplemented()

    def center(self, width: int, fillchar: str = " ") -> str:
        _str_guard(self)
        width = _index(width)
        if not _str_check(fillchar):
            raise TypeError(
                f"center() argument 2 must be str, not {_type(fillchar).__name__}"
            )
        if _str_len(fillchar) != 1:
            raise TypeError("The fill character must be exactly one character long")

        pad = width - _str_len(self)
        if pad <= 0:
            return self
        left_pad = (pad >> 1) + (pad & width & 1)
        return fillchar * left_pad + self + fillchar * (pad - left_pad)

    def count(self, sub, start=None, end=None):
        _str_guard(self)
        if not _str_check(sub):
            raise TypeError(f"must be str, not {_type(sub).__name__}")
        if start is not None:
            start = _index(start)
        if end is not None:
            end = _index(end)
        return _str_count(self, sub, start, end)

    def encode(self, encoding="utf-8", errors=_Unbound) -> bytes:
        result = _str_encode(self, encoding)
        if result is not _Unbound:
            return result
        return _codecs.encode(self, encoding, errors)

    def endswith(self, suffix, start=None, end=None):
        _str_guard(self)
        start = _slice_index(start)
        end = _slice_index(end)
        if _str_check(suffix):
            return _str_endswith(self, suffix, start, end)
        if _tuple_check(suffix):
            for item in suffix:
                if not _str_check(item):
                    raise TypeError(
                        "tuple for endswith must only contain str, "
                        f"not {_type(item).__name__}"
                    )
                if _str_endswith(self, item, start, end):
                    return True
            return False
        raise TypeError(
            "endswith first arg must be str or a tuple of str, "
            f"not {_type(suffix).__name__}"
        )

    def expandtabs(self, tabsize=8):
        _str_guard(self)
        _int_guard(tabsize)
        if tabsize == 0:
            return str.replace(self, "\t", "")
        chars_seen = 0
        col_pos = 0
        substr_start = 0
        result = _strarray()
        for char in self:
            if char == "\n" or char == "\r":
                chars_seen += 1
                col_pos = 0
                continue
            if char == "\t":
                _strarray_iadd(result, self[substr_start : substr_start + chars_seen])
                _strarray_iadd(result, " " * (tabsize - (col_pos % tabsize)))
                substr_start += chars_seen + 1
                chars_seen = 0
                col_pos = 0
                continue
            chars_seen += 1
            col_pos += 1
        if chars_seen != 0:
            _strarray_iadd(result, self[substr_start : substr_start + chars_seen + 1])
        return str(result)

    def find(self, sub, start=None, end=None):
        _str_guard(self)
        if not _str_check(sub):
            raise TypeError(f"must be str, not {_type(sub).__name__}")
        if start is not None:
            start = _index(start)
        if end is not None:
            end = _index(end)
        return _str_find(self, sub, start, end)

    def format(self, *args, **kwargs):
        _str_guard(self)

        global _formatter
        if not _formatter:
            import string

            _formatter = string.Formatter()
        return _formatter.format(self, *args, **kwargs)

    def format_map(self, mapping):
        _unimplemented()

    def index(self, sub, start=None, end=None):
        res = self.find(sub, start, end)
        if res < 0:
            raise ValueError(f"substring {sub} not found")
        return res

    def isalnum(self):
        _builtin()

    def isalpha(self):
        _builtin()

    def isascii(self):
        _builtin()

    def isdecimal(self):
        _builtin()

    def isdigit(self):
        _builtin()

    def isidentifier(self):
        _builtin()

    def islower(self):
        _builtin()

    def isnumeric(self):
        _builtin()

    def isprintable(self):
        _builtin()

    def isspace(self):
        _builtin()

    def istitle(self):
        _builtin()

    def isupper(self):
        _builtin()

    def join(self, items) -> str:
        result = _str_join(self, items)
        if result is not _Unbound:
            return result
        try:
            it = iter(items)
        except Exception:
            raise TypeError("can only join an iterable")
        return _str_join(self, [*it])

    def ljust(self, width: int, fillchar: str = " ") -> str:
        _str_guard(self)
        width = _index(width)
        if not _str_check(fillchar):
            raise TypeError(
                f"rjust() argument 2 must be str, not {_type(fillchar).__name__}"
            )
        if _str_len(fillchar) != 1:
            raise TypeError("The fill character must be exactly one character long")
        padding = width - _str_len(self)
        return self + fillchar * padding if padding > 0 else self

    def lower(self):
        _builtin()

    def lstrip(self, other=None):
        _builtin()

    @staticmethod
    def maketrans(frm, to=_Unbound, to_none=_Unbound):
        result = {}
        if to is _Unbound:
            _dict_guard(frm)
            for key, value in frm.items():
                if _str_check(key) and _str_ischr(key):
                    key = ord(key)
                elif not _int_check(key):
                    raise TypeError("keys in translate table must be str or int")
                _dict_setitem(result, key, value)
            return result

        _str_guard(frm)
        _str_guard(to)
        frm_iter = iter(frm)
        to_iter = iter(to)
        while True:
            key = next(frm_iter, None)
            value = next(to_iter, None)
            if key is None and value is None:
                break
            elif key is None or value is None:
                raise ValueError(
                    "The first two maketrans arguments must have equal length"
                )
            _dict_setitem(result, ord(key), ord(value))

        if to_none is not _Unbound:
            for ch in to_none:
                _dict_setitem(result, ord(ch), None)
        return result

    def partition(self, sep):
        _str_guard(self)
        _str_guard(sep)
        return _str_partition(self, sep)

    def replace(self, old, new, count=None):
        _str_guard(self)
        if not _str_check(old):
            raise TypeError(
                f"replace() argument 1 must be str, not {_type(old).__name__}"
            )
        if not _str_check(new):
            raise TypeError(
                f"replace() argument 2 must be str, not {_type(new).__name__}"
            )
        if _float_check(count):
            raise TypeError("integer argument expected, got float")

        if count is not None:
            count = _index(count)
            if count == 0:
                return _str_from_str(str, self)
        else:
            count = -1
        result = _str_replace(self, old, new, count)
        return _str_from_str(str, result) if self is result else result

    def rfind(self, sub, start=None, end=None):
        _str_guard(self)
        if not _str_check(sub):
            raise TypeError(f"must be str, not {_type(sub).__name__}")
        if start is not None:
            start = _index(start)
        if end is not None:
            end = _index(end)
        return _str_rfind(self, sub, start, end)

    def rindex(self, sub, start=_Unbound, end=_Unbound):
        _unimplemented()

    def rjust(self, width: int, fillchar: str = " ") -> str:
        _str_guard(self)
        width = _index(width)
        if not _str_check(fillchar):
            raise TypeError(
                f"rjust() argument 2 must be str, not {_type(fillchar).__name__}"
            )
        if _str_len(fillchar) != 1:
            raise TypeError("The fill character must be exactly one character long")
        padding = width - _str_len(self)
        return fillchar * padding + self if padding > 0 else self

    def rpartition(self, sep):
        _str_guard(self)
        _str_guard(sep)
        return _str_rpartition(self, sep)

    def rsplit(self, sep=None, maxsplit=-1):
        # TODO(T37437993): Write in C++
        return [s[::-1] for s in self[::-1].split(sep[::-1], maxsplit)[::-1]]

    def rstrip(self, other=None):
        _builtin()

    def split(self, sep=None, maxsplit=-1):
        _str_guard(self)
        if _int_check(maxsplit) and (sep is None or _str_check(sep)):
            return _str_split(self, sep, maxsplit)
        if _float_check(maxsplit):
            raise TypeError("integer argument expected, got float")
        maxsplit = _index(maxsplit)
        if sep is None or _str_check(sep):
            return _str_split(self, sep, maxsplit)
        raise TypeError(f"must be str or None, not {_type(sep).__name__}")

    def splitlines(self, keepends=False):
        _str_guard(self)
        if _float_check(keepends):
            raise TypeError("integer argument expected, got float")
        if not _int_check(keepends):
            keepends = int(keepends)
        return _str_splitlines(self, keepends)

    def startswith(self, prefix, start=None, end=None):
        _str_guard(self)
        start = _slice_index(start)
        end = _slice_index(end)
        if _str_check(prefix):
            return _str_startswith(self, prefix, start, end)
        if _tuple_check(prefix):
            for item in prefix:
                if not _str_check(item):
                    raise TypeError(
                        "tuple for startswith must only contain str, "
                        f"not {_type(item).__name__}"
                    )
                if _str_startswith(self, item, start, end):
                    return True
            return False
        raise TypeError(
            "startswith first arg must be str or a tuple of str, "
            f"not {_type(prefix).__name__}"
        )

    def strip(self, other=None):
        _builtin()

    def swapcase(self):
        _unimplemented()

    def title(self):
        _builtin()

    def translate(self, table):
        result = _str_translate(self, table)
        if result is not _Unbound:
            return result
        result = _strarray()
        if _dict_check(table):
            for ch in self:
                value = _dict_get(table, ord(ch), ch)
                if value is None:
                    continue
                if _int_check(value):
                    value = chr(value)
                elif not _str_check(value):
                    raise TypeError("character mapping must return int, None or str")
                _strarray_iadd(result, value)
            return str(result)

        dunder_getitem = _object_type_getattr(table, "__getitem__")
        if dunder_getitem is _Unbound:
            raise TypeError(f"'{_type(table).__name__}' object is not subscriptable")
        for ch in self:
            try:
                table_entry = dunder_getitem(ord(ch))
                if _int_check(table_entry):
                    _strarray_iadd(result, chr(table_entry))
                elif _str_check(table_entry):
                    _strarray_iadd(result, table_entry)
                elif table_entry is not None:
                    raise TypeError(
                        "character mapping must return integer, None, or str"
                    )
            except LookupError:
                _strarray_iadd(result, ch)
        return str(result)

    def upper(self):
        _builtin()

    def zfill(self, width):
        _str_guard(self)
        if not _int_check(width):
            if _float_check(width):
                raise TypeError("integer argument expected, got float")
            width = _index(width)
        str_len = _str_len(self)
        if width <= str_len:
            return self

        result = _strarray()
        has_prefix = str.startswith(self, ("+", "-"))
        if has_prefix:
            _strarray_iadd(result, _str_getitem(self, 0))
        _strarray_iadd(result, "0" * (width - str_len))
        _strarray_iadd(result, _str_getslice(self, int(has_prefix), str_len, 1))
        return str(result)


class str_iterator(bootstrap=True):
    def __iter__(self):
        _builtin()

    def __length_hint__(self):
        _builtin()

    def __next__(self):
        _builtin()


def sum(iterable, start=0):
    if _str_check(start):
        raise TypeError("sum() can't sum strings [use ''.join(seq) instead]")
    if _bytes_check(start):
        raise TypeError("sum() can't sum bytes [use b''.join(seq) instead]")
    if _bytearray_check(start):
        raise TypeError("sum() can't sum bytearray [use b''.join(seq) instead]")

    result = start
    for item in iterable:
        result = result + item
    return result


class super(bootstrap=True):
    def __init__(self, type=_Unbound, type_or_obj=_Unbound):
        _builtin()

    def __getattribute__(self, name):
        _builtin()

    def __new__(cls, type=_Unbound, type_or_obj=_Unbound):
        _builtin()


class tuple(bootstrap=True):
    def __add__(self, other):
        _builtin()

    def __contains__(self, key):
        _builtin()

    def __eq__(self, other):
        _tuple_guard(self)
        if not _tuple_check(other):
            return NotImplemented
        len_self = _tuple_len(self)
        len_other = _tuple_len(other)
        min_len = len_self if len_self < len_other else len_other
        # Find the first non-equal item in the tuples
        i = 0
        while i < min_len:
            self_item = _tuple_getitem(self, i)
            other_item = _tuple_getitem(other, i)
            if self_item is not other_item and not self_item == other_item:
                return False
            i += 1
        # If the items are all up equal up to min_len, compare lengths
        return len_self == len_other

    def __ge__(self, other):
        _tuple_guard(self)
        if not _tuple_check(other):
            return NotImplemented
        len_self = _tuple_len(self)
        len_other = _tuple_len(other)
        min_len = len_self if len_self < len_other else len_other
        # Find the first non-equal item in the tuples
        i = 0
        while i < min_len:
            self_item = _tuple_getitem(self, i)
            other_item = _tuple_getitem(other, i)
            if self_item is not other_item and not self_item == other_item:
                return self_item >= other_item
            i += 1
        # If the items are all up equal up to min_len, compare lengths
        return len_self >= len_other

    __getattribute__ = object.__getattribute__

    def __getitem__(self, index):
        result = _tuple_getitem(self, index)
        if result is not _Unbound:
            return result
        if _slice_check(index):
            step = _slice_step(_slice_index(index.step))
            length = _tuple_len(self)
            start = _slice_start(_slice_index(index.start), step, length)
            stop = _slice_stop(_slice_index(index.stop), step, length)
            return _tuple_getslice(self, start, stop, step)
        if _object_type_hasattr(index, "__index__"):
            return _tuple_getitem(self, _index(index))
        raise TypeError(
            f"tuple indices must be integers or slices, not {_type(index).__name__}"
        )

    def __getnewargs__(self):
        _unimplemented()

    def __gt__(self, other):
        _tuple_guard(self)
        if not _tuple_check(other):
            return NotImplemented
        len_self = _tuple_len(self)
        len_other = _tuple_len(other)
        min_len = len_self if len_self < len_other else len_other
        # Find the first non-equal item in the tuples
        i = 0
        while i < min_len:
            self_item = _tuple_getitem(self, i)
            other_item = _tuple_getitem(other, i)
            if self_item is not other_item and not self_item == other_item:
                return self_item > other_item
            i += 1
        # If the items are all up equal up to min_len, compare lengths
        return len_self > len_other

    def __hash__(self):
        _builtin()

    def __iter__(self):
        _builtin()

    def __le__(self, other):
        _tuple_guard(self)
        if not _tuple_check(other):
            return NotImplemented
        len_self = _tuple_len(self)
        len_other = _tuple_len(other)
        min_len = len_self if len_self < len_other else len_other
        # Find the first non-equal item in the tuples
        i = 0
        while i < min_len:
            self_item = _tuple_getitem(self, i)
            other_item = _tuple_getitem(other, i)
            if self_item is not other_item and not self_item == other_item:
                return self_item <= other_item
            i += 1
        # If the items are all up equal up to min_len, compare lengths
        return len_self <= len_other

    def __len__(self):
        _builtin()

    def __lt__(self, other):
        _tuple_guard(self)
        if not _tuple_check(other):
            return NotImplemented
        len_self = _tuple_len(self)
        len_other = _tuple_len(other)
        min_len = len_self if len_self < len_other else len_other
        # Find the first non-equal item in the tuples
        i = 0
        while i < min_len:
            self_item = _tuple_getitem(self, i)
            other_item = _tuple_getitem(other, i)
            if self_item is not other_item and not self_item == other_item:
                return self_item < other_item
            i += 1
        # If the items are all up equal up to min_len, compare lengths
        return len_self < len_other

    def __mul__(self, other):
        _builtin()

    def __ne__(self, other):
        _tuple_guard(self)
        if not _tuple_check(other):
            return NotImplemented
        len_self = _tuple_len(self)
        len_other = _tuple_len(other)
        min_len = len_self if len_self < len_other else len_other
        # Find the first non-equal item in the tuples
        i = 0
        while i < min_len:
            self_item = _tuple_getitem(self, i)
            other_item = _tuple_getitem(other, i)
            if self_item is not other_item and not self_item == other_item:
                return True
            i += 1
        # If the items are all up equal up to min_len, compare lengths
        return len_self != len_other

    def __new__(cls, iterable=()):
        _type_subclass_guard(cls, tuple)
        if cls is tuple:
            if _tuple_check_exact(iterable):
                return iterable
            return (*iterable,)
        if _tuple_check_exact(iterable):
            return _tuple_new(cls, iterable)
        return _tuple_new(cls, (*iterable,))

    def __repr__(self):
        _tuple_guard(self)
        if _repr_enter(self):
            return "(...)"
        if _tuple_len(self) == 1:
            elt = repr(self[0])
            _repr_leave(self)
            return f"({elt},)"
        _repr_leave(self)
        return _sequence_repr("(", self, ")")

    __rmul__ = __mul__

    def count(self, other):
        _unimplemented()

    def index(self, other, start=_Unbound, stop=_Unbound):
        _tuple_guard(self)
        length = _tuple_len(self)
        if start is _Unbound:
            start = 0
        else:
            start = _slice_start(_slice_index_not_none(start), 1, length)
        if stop is _Unbound:
            stop = length
        else:
            stop = _slice_stop(_slice_index_not_none(stop), 1, length)
        i = start
        while i < stop:
            item = _tuple_getitem(self, i)
            if item is other or item == other:
                return i
            i += 1
        raise ValueError("tuple.index(x): x not in tuple")


class tuple_iterator(bootstrap=True):
    def __iter__(self):
        _builtin()

    def __length_hint__(self):
        _builtin()

    def __next__(self):
        _builtin()


class type_proxy(bootstrap=True):
    def __contains__(self, key) -> bool:
        _type_proxy_guard(self)
        return _type_proxy_get(self, key, _Unbound) is not _Unbound  # noqa: T484

    def __eq__(self, other):
        _unimplemented()

    def __getitem__(self, key):
        _type_proxy_guard(self)
        result = _type_proxy_get(self, key, _Unbound)
        if result is _Unbound:
            raise KeyError(key)
        return result

    __hash__ = None

    # type_proxy is not designed to be instantiated.
    def __init__(self, *args, **kwargs):
        _unimplemented()

    def __iter__(self):
        _type_proxy_guard(self)
        # TODO(T53302128): Return an iterable to avoid materializing a list of keys.
        return iter(_type_proxy_keys(self))

    def __len__(self):
        _type_proxy_guard(self)
        return _type_proxy_len(self)

    # type_proxy is not designed to be subclassed.
    def __new__(cls, *args, **kwargs):
        _unimplemented()

    def __repr__(self):
        _type_proxy_guard(self)
        if _repr_enter(self):
            return "{...}"
        kwpairs = [f"{key!r}: {self[key]!r}" for key in _type_proxy_keys(self)]
        _repr_leave(self)
        return "{" + ", ".join(kwpairs) + "}"

    def copy(self):
        _type_proxy_guard(self)
        # TODO(T53302128): Return an iterable to avoid materializing the list of items.
        keys = _type_proxy_keys(self)
        values = _type_proxy_values(self)
        return {keys[i]: values[i] for i in range(len(keys))}

    def get(self, key, default=None):
        _type_proxy_guard(self)
        return _type_proxy_get(self, key, default)

    def items(self):
        _type_proxy_guard(self)
        # TODO(T53302128): Return an iterable to avoid materializing the list of items.
        keys = _type_proxy_keys(self)
        values = _type_proxy_values(self)
        return [(keys[i], values[i]) for i in range(len(keys))]

    def keys(self):
        _type_proxy_guard(self)
        # TODO(T53302128): Return an iterable to avoid materializing the list of keys.
        return _type_proxy_keys(self)

    def values(self):
        _type_proxy_guard(self)
        # TODO(T53302128): Return an iterable to avoid materializing the list of values.
        return _type_proxy_values(self)


def vars(obj=_Unbound):
    if obj is _Unbound:
        return _caller_locals()
    try:
        return obj.__dict__
    except Exception:
        raise TypeError("vars() argument must have __dict__ attribute")


class zip:
    def __init__(self, *iterables):
        if not iterables:
            iterators = [iter(())]
        else:
            iterators = [iter(it) for it in iterables]
        self._iterators = iterators

    def __iter__(self):
        return self

    def __next__(self):
        iterators = self._iterators
        length = _list_len(iterators)
        result = _list_new(length)
        i = 0
        while i < length:
            result[i] = next(iterators[i])
            i += 1
        return (*result,)

    def __reduce__(self):
        _unimplemented()
