#!/usr/bin/env python3
"""Built-in classes, functions, and constants."""


# These values, which live in the _builtins module, are injected by our boot
# process. The runtime imports them manually because __import__ has not been
# defined yet in the execution of this module. flake8 has no knowledge about
# their definitions and will complain without these circular assignments.
_address = _address  # noqa: F821
_bool_check = _bool_check  # noqa: F821
_bound_method = _bound_method  # noqa: F821
_bytearray_check = _bytearray_check  # noqa: F821
_bytearray_clear = _bytearray_clear  # noqa: F821
_bytearray_delitem = _bytearray_delitem  # noqa: F821
_bytearray_delslice = _bytearray_delslice  # noqa: F821
_bytearray_getitem = _bytearray_getitem  # noqa: F821
_bytearray_getslice = _bytearray_getslice  # noqa: F821
_bytearray_guard = _bytearray_guard  # noqa: F821
_bytearray_join = _bytearray_join  # noqa: F821
_bytearray_len = _bytearray_len  # noqa: F821
_bytearray_setitem = _bytearray_setitem  # noqa: F821
_bytearray_setslice = _bytearray_setslice  # noqa: F821
_bytes_check = _bytes_check  # noqa: F821
_bytes_decode = _bytes_decode  # noqa: F821
_bytes_decode_utf_8 = _bytes_decode_utf_8  # noqa: F821
_bytes_from_bytes = _bytes_from_bytes  # noqa: F821
_bytes_from_ints = _bytes_from_ints  # noqa: F821
_bytes_getitem = _bytes_getitem  # noqa: F821
_bytes_getslice = _bytes_getslice  # noqa: F821
_bytes_guard = _bytes_guard  # noqa: F821
_bytes_join = _bytes_join  # noqa: F821
_bytes_len = _bytes_len  # noqa: F821
_bytes_maketrans = _bytes_maketrans  # noqa: F821
_bytes_repeat = _bytes_repeat  # noqa: F821
_bytes_split = _bytes_split  # noqa: F821
_bytes_split_whitespace = _bytes_split_whitespace  # noqa: F821
_byteslike_check = _byteslike_check  # noqa: F821
_byteslike_count = _byteslike_count  # noqa: F821
_byteslike_endswith = _byteslike_endswith  # noqa: F821
_byteslike_find_byteslike = _byteslike_find_byteslike  # noqa: F821
_byteslike_find_int = _byteslike_find_int  # noqa: F821
_byteslike_guard = _byteslike_guard  # noqa: F821
_byteslike_rfind_byteslike = _byteslike_rfind_byteslike  # noqa: F821
_byteslike_rfind_int = _byteslike_rfind_int  # noqa: F821
_byteslike_startswith = _byteslike_startswith  # noqa: F821
_classmethod = _classmethod  # noqa: F821
_classmethod_isabstract = _classmethod_isabstract  # noqa: F821
_code_check = _code_check  # noqa: F821
_code_guard = _code_guard  # noqa: F821
_compile_flags_mask = _compile_flags_mask  # noqa: F821
_complex_check = _complex_check  # noqa: F821
_complex_imag = _complex_imag  # noqa: F821
_complex_real = _complex_real  # noqa: F821
_dict_bucket_insert = _dict_bucket_insert  # noqa: F821
_dict_bucket_key = _dict_bucket_key  # noqa: F821
_dict_bucket_set_value = _dict_bucket_set_value  # noqa: F821
_dict_bucket_value = _dict_bucket_value  # noqa: F821
_dict_check = _dict_check  # noqa: F821
_dict_checkexact = _dict_checkexact  # noqa: F821
_dict_get = _dict_get  # noqa: F821
_dict_guard = _dict_guard  # noqa: F821
_dict_lookup = _dict_lookup  # noqa: F821
_dict_lookup_next = _dict_lookup_next  # noqa: F821
_dict_popitem = _dict_popitem  # noqa: F821
_dict_setitem = _dict_setitem  # noqa: F821
_dict_update = _dict_update  # noqa: F821
_divmod = _divmod  # noqa: F821
_exec = _exec  # noqa: F821
_float_check = _float_check  # noqa: F821
_float_checkexact = _float_checkexact  # noqa: F821
_float_divmod = _float_divmod  # noqa: F821
_float_format = _float_format  # noqa: F821
_float_guard = _float_guard  # noqa: F821
_float_new_from_byteslike = _float_new_from_byteslike  # noqa: F821
_float_new_from_float = _float_new_from_float  # noqa: F821
_float_new_from_str = _float_new_from_str  # noqa: F821
_frozenset_check = _frozenset_check  # noqa: F821
_frozenset_guard = _frozenset_guard  # noqa: F821
_function_globals = _function_globals  # noqa: F821
_function_guard = _function_guard  # noqa: F821
_getframe_function = _getframe_function  # noqa: F821
_getframe_locals = _getframe_locals  # noqa: F821
_get_member_byte = _get_member_byte  # noqa: F821
_get_member_char = _get_member_char  # noqa: F821
_get_member_double = _get_member_double  # noqa: F821
_get_member_float = _get_member_float  # noqa: F821
_get_member_int = _get_member_int  # noqa: F821
_get_member_long = _get_member_long  # noqa: F821
_get_member_pyobject = _get_member_pyobject  # noqa: F821
_get_member_short = _get_member_short  # noqa: F821
_get_member_string = _get_member_string  # noqa: F821
_get_member_ubyte = _get_member_ubyte  # noqa: F821
_get_member_uint = _get_member_uint  # noqa: F821
_get_member_ulong = _get_member_ulong  # noqa: F821
_get_member_ushort = _get_member_ushort  # noqa: F821
_instance_delattr = _instance_delattr  # noqa: F821
_instance_getattr = _instance_getattr  # noqa: F821
_instance_guard = _instance_guard  # noqa: F821
_instance_overflow_dict = _instance_overflow_dict  # noqa: F821
_instance_setattr = _instance_setattr  # noqa: F821
_int_check = _int_check  # noqa: F821
_int_checkexact = _int_checkexact  # noqa: F821
_int_from_bytes = _int_from_bytes  # noqa: F821
_int_guard = _int_guard  # noqa: F821
_int_new_from_bytearray = _int_new_from_bytearray  # noqa: F821
_int_new_from_bytes = _int_new_from_bytes  # noqa: F821
_int_new_from_int = _int_new_from_int  # noqa: F821
_int_new_from_str = _int_new_from_str  # noqa: F821
_list_check = _list_check  # noqa: F821
_list_checkexact = _list_checkexact  # noqa: F821
_list_delitem = _list_delitem  # noqa: F821
_list_delslice = _list_delslice  # noqa: F821
_list_extend = _list_extend  # noqa: F821
_list_getitem = _list_getitem  # noqa: F821
_list_getslice = _list_getslice  # noqa: F821
_list_guard = _list_guard  # noqa: F821
_list_len = _list_len  # noqa: F821
_list_sort = _list_sort  # noqa: F821
_list_swap = _list_swap  # noqa: F821
_mappingproxy_guard = _mappingproxy_guard  # noqa: F821
_mappingproxy_mapping = _mappingproxy_mapping  # noqa: F821
_mappingproxy_set_mapping = _mappingproxy_set_mapping  # noqa: F821
_memoryview_guard = _memoryview_guard  # noqa: F821
_memoryview_itemsize = _memoryview_itemsize  # noqa: F821
_memoryview_nbytes = _memoryview_nbytes  # noqa: F821
_memoryview_setitem = _memoryview_setitem  # noqa: F821
_memoryview_setslice = _memoryview_setslice  # noqa: F821
_module_dir = _module_dir  # noqa: F821
_module_proxy = _module_proxy  # noqa: F821
_module_proxy_check = _module_proxy_check  # noqa: F821
_module_proxy_delitem = _module_proxy_delitem  # noqa: F821
_module_proxy_get = _module_proxy_get  # noqa: F821
_module_proxy_guard = _module_proxy_guard  # noqa: F821
_module_proxy_keys = _module_proxy_keys  # noqa: F821
_module_proxy_len = _module_proxy_len  # noqa: F821
_module_proxy_setitem = _module_proxy_setitem  # noqa: F821
_module_proxy_values = _module_proxy_values  # noqa: F821
_iter = _iter  # noqa: F821
_object_keys = _object_keys  # noqa: F821
_object_type_getattr = _object_type_getattr  # noqa: F821
_object_type_hasattr = _object_type_hasattr  # noqa: F821
_patch = _patch  # noqa: F821
_property = _property  # noqa: F821
_property_isabstract = _property_isabstract  # noqa: F821
_pyobject_offset = _pyobject_offset  # noqa: F821
_range_check = _range_check  # noqa: F821
_range_guard = _range_guard  # noqa: F821
_range_len = _range_len  # noqa: F821
_repr_enter = _repr_enter  # noqa: F821
_repr_leave = _repr_leave  # noqa: F821
_seq_index = _seq_index  # noqa: F821
_seq_iterable = _seq_iterable  # noqa: F821
_seq_set_index = _seq_set_index  # noqa: F821
_seq_set_iterable = _seq_set_iterable  # noqa: F821
_set_check = _set_check  # noqa: F821
_set_guard = _set_guard  # noqa: F821
_set_len = _set_len  # noqa: F821
_set_member_double = _set_member_double  # noqa: F821
_set_member_float = _set_member_float  # noqa: F821
_set_member_integral = _set_member_integral  # noqa: F821
_set_member_pyobject = _set_member_pyobject  # noqa: F821
_slice_check = _slice_check  # noqa: F821
_slice_guard = _slice_guard  # noqa: F821
_slice_start = _slice_start  # noqa: F821
_slice_start_long = _slice_start_long  # noqa: F821
_slice_step = _slice_step  # noqa: F821
_slice_step_long = _slice_step_long  # noqa: F821
_slice_stop = _slice_stop  # noqa: F821
_slice_stop_long = _slice_stop_long  # noqa: F821
_staticmethod_isabstract = _staticmethod_isabstract  # noqa: F821
_str_check = _str_check  # noqa: F821
_str_checkexact = _str_checkexact  # noqa: F821
_str_count = _str_count  # noqa: F821
_str_encode = _str_encode  # noqa: F821
_str_endswith = _str_endswith  # noqa: F821
_str_escape_non_ascii = _str_escape_non_ascii  # noqa: F821
_str_find = _str_find  # noqa: F821
_str_from_str = _str_from_str  # noqa: F821
_str_getitem = _str_getitem  # noqa: F821
_str_getslice = _str_getslice  # noqa: F821
_str_guard = _str_guard  # noqa: F821
_str_ischr = _str_ischr  # noqa: F821
_str_join = _str_join  # noqa: F821
_str_len = _str_len  # noqa: F821
_str_partition = _str_partition  # noqa: F821
_str_replace = _str_replace  # noqa: F821
_str_rfind = _str_rfind  # noqa: F821
_str_rpartition = _str_rpartition  # noqa: F821
_str_split = _str_split  # noqa: F821
_str_splitlines = _str_splitlines  # noqa: F821
_str_startswith = _str_startswith  # noqa: F821
_strarray_iadd = _strarray_iadd  # noqa: F821
_tuple_check = _tuple_check  # noqa: F821
_tuple_checkexact = _tuple_checkexact  # noqa: F821
_tuple_getitem = _tuple_getitem  # noqa: F821
_tuple_getslice = _tuple_getslice  # noqa: F821
_tuple_guard = _tuple_guard  # noqa: F821
_tuple_len = _tuple_len  # noqa: F821
_tuple_new = _tuple_new  # noqa: F821
_type = _type  # noqa: F821
_type_abstractmethods_del = _type_abstractmethods_del  # noqa: F821
_type_abstractmethods_get = _type_abstractmethods_get  # noqa: F821
_type_abstractmethods_set = _type_abstractmethods_set  # noqa: F821
_type_bases_del = _type_bases_del  # noqa: F821
_type_bases_get = _type_bases_get  # noqa: F821
_type_bases_set = _type_bases_set  # noqa: F821
_type_check = _type_check  # noqa: F821
_type_check_exact = _type_check_exact  # noqa: F821
_type_dunder_call = _type_dunder_call  # noqa: F821
_type_guard = _type_guard  # noqa: F821
_type_init = _type_init  # noqa: F821
_type_issubclass = _type_issubclass  # noqa: F821
_type_new = _type_new  # noqa: F821
_type_proxy = _type_proxy  # noqa: F821
_type_proxy_check = _type_proxy_check  # noqa: F821
_type_proxy_get = _type_proxy_get  # noqa: F821
_type_proxy_guard = _type_proxy_guard  # noqa: F821
_type_proxy_keys = _type_proxy_keys  # noqa: F821
_type_proxy_len = _type_proxy_len  # noqa: F821
_type_proxy_values = _type_proxy_values  # noqa: F821
_Unbound = _Unbound  # noqa: F821
_unimplemented = _unimplemented  # noqa: F821
_warn = _warn  # noqa: F821


# Begin: Early definitions that are necessary to process the rest of the file:

# This function is called after all builtins modules are initialized to enable
# __import__ to work correctly.
def _init():
    global _codecs
    import _codecs

    global _str_mod
    import _str_mod

    global _sys
    import sys as _sys


@_patch
def __build_class__(func, name, *bases, metaclass=_Unbound, bootstrap=False, **kwargs):
    pass


class function(bootstrap=True):
    def __call__(self, *args, **kwargs):
        _function_guard(self)
        return self(*args, **kwargs)

    __globals__ = _property(_function_globals)

    @_property
    def __dict__(self):
        # TODO(T56646836) we should not need to define a custom __dict__.
        _function_guard(self)
        return _instance_overflow_dict(self)

    def __get__(self, instance, owner):
        pass

    def __repr__(self):
        _function_guard(self)
        return f"<function {self.__name__} at {_address(self):#x}>"


class classmethod(bootstrap=True):
    def __get__(self, instance, owner):
        pass

    def __init__(self, fn):
        pass

    __isabstractmethod__ = _property(_classmethod_isabstract)

    def __new__(cls, fn):
        pass


class property(bootstrap=True):
    def __delete__(self, instance):
        pass

    def __get__(self, instance, owner=None):
        pass

    def __init__(self, fget=None, fset=None, fdel=None, doc=None):
        pass

    __isabstractmethod__ = _property(_property_isabstract)

    def __new__(cls, fget=None, fset=None, fdel=None, doc=None):
        pass

    def __set__(self, instance, value):
        pass

    def deleter(self, fn):
        pass

    def getter(self, fn):
        pass

    def setter(self, fn):
        pass


class staticmethod(bootstrap=True):
    def __get__(self, instance, owner):
        pass

    def __init__(self, fn):
        pass

    __isabstractmethod__ = _property(_staticmethod_isabstract)

    def __new__(cls, fn):
        pass


class _traceback(bootstrap=True):
    pass


class type(bootstrap=True):
    __abstractmethods__ = _property(
        _type_abstractmethods_get, _type_abstractmethods_set, _type_abstractmethods_del
    )

    __bases__ = _property(_type_bases_get, _type_bases_set, _type_bases_del)

    @_property
    def __dict__(self):
        return mappingproxy(_type_proxy(self))

    __call__ = _type_dunder_call

    def __dir__(self):
        _type_guard(self)
        result = set()
        type._merge_class_dict_keys(self, result)
        return list(result)

    def __getattribute__(self, name):
        pass

    def __init__(self, name_or_object, bases=_Unbound, dict=_Unbound):
        # Not a patch; just empty
        pass

    def __instancecheck__(self, obj) -> bool:
        return _isinstance_type(obj, _type(obj), self)

    def __new__(cls, name_or_object, bases=_Unbound, type_dict=_Unbound):
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
        return _type_init(instance, name_or_object, type_dict, mro)

    @_classmethod
    def __prepare__(self, *args, **kwargs):
        return {}

    def __repr__(self):
        return f"<class '{self.__name__}'>"

    def __setattr__(self, name, value):
        pass

    def __subclasscheck__(self, subclass) -> bool:
        return _issubclass(subclass, self)

    def __subclasses__(self):
        pass

    def _merge_class_dict_keys(self, result):
        result.update(self.__dict__.keys())
        for base in self.__bases__:
            type._merge_class_dict_keys(base, result)

    def mro(self):
        pass


class object(bootstrap=True):  # noqa: E999
    __class__ = _property(_type)

    @__class__.setter  # noqa: F811
    def __class__(self, value):
        _unimplemented()

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
        pass

    def __init__(self, *args, **kwargs):
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
        pass

    def __repr__(self):
        return f"<{_type(self).__name__} object at {_address(self):#x}>"

    def __setattr__(self, name, value):
        pass

    def __sizeof__(self):
        pass

    def __str__(self):
        return _type(self).__repr__(self)

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
        pass

    def __repr__(self):
        if not isinstance(self, BaseException):
            raise TypeError("not a BaseException object")
        return f"{self.__class__.__name__}{self.args!r}"

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
        pass

    def __repr__(self):
        pass


class NotADirectoryError(OSError, bootstrap=True):
    pass


class NotImplementedType(bootstrap=True):
    def __repr__(self):
        return "NotImplemented"


class NotImplementedError(RuntimeError, bootstrap=True):
    pass


class OSError(Exception, bootstrap=True):
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
        pass


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
        have_lineno = _int_checkexact(self.lineno)
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
        pass


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


@_patch
def __import__(name, globals=None, locals=None, fromlist=(), level=0):
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
        if not _type_issubclass(cls, self.cls):
            raise TypeError(f"Expected instance of type {self.cls} not {cls}")
        return _bound_method(self.fn, cls)

    def __call__(self, *args, **kwargs):
        if not args:
            raise TypeError(f"Function {self.fn} needs at least 1 argument")
        cls = args[0]
        if not issubclass(cls, self.cls):
            raise TypeError(f"Expected type {self.cls} not {self}")
        return self.fn(*args, **kwargs)


def _dict_getitem(self, key):
    # Fast path. From the probing strategy, most dictionary lookups will
    # successfully match the first non-empty bucket it finds or completely
    # fail to find a single match.
    key_hash = hash(key)
    index = _dict_lookup(self, key, key_hash)
    if index < 0:  # No match in the entire dict
        return _Unbound
    other_key = _dict_bucket_key(self, index)
    if other_key is key or other_key == key:
        return _dict_bucket_value(self, index)

    # Slow path. The first non-empty bucket did not contain a match. Restart
    # the probe at the point where it found the last non-empty bucket until
    # a match is found or all buckets have been probed
    index, perturb = _dict_lookup_next(self, index, key, key_hash, _Unbound)
    while index >= 0:
        other_key = _dict_bucket_key(self, index)
        if other_key is key or other_key == key:
            return _dict_bucket_value(self, index)
        index, perturb = _dict_lookup_next(self, index, key, key_hash, perturb)
    return _Unbound


# TODO(T56367459): Use _dict_setitem instead.
def _capi_dict_setitem(self, key, value):
    # Fast path. From the probing strategy, most dictionary lookups will
    # successfully match the first non-empty bucket it finds or completely
    # fail to find a single match.
    key_hash = hash(key)
    index = _dict_lookup(self, key, key_hash)
    if index < 0:  # No match in the entire dict
        _dict_bucket_insert(self, index, key, key_hash, value)
        return
    other_key = _dict_bucket_key(self, index)
    if other_key is key or other_key == key:
        _dict_bucket_set_value(self, index, value)
        return

    # Slow path. The first non-empty bucket did not contain a match. Restart
    # the probe at the point where it found the last non-empty bucket until
    # a match is found or all buckets have been probed
    index, perturb = _dict_lookup_next(self, index, key, key_hash, _Unbound)
    while index >= 0:
        other_key = _dict_bucket_key(self, index)
        if other_key is key or other_key == key:
            _dict_bucket_set_value(self, index, value)
            return
        index, perturb = _dict_lookup_next(self, index, key, key_hash, perturb)
    _dict_bucket_insert(self, index, key, key_hash, value)


def _dunder_bases_tuple_check(obj, msg) -> None:
    try:
        if not _tuple_check(obj.__bases__):
            raise TypeError(msg)
    except AttributeError:
        raise TypeError(msg)


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
    if _int_checkexact(obj):
        return obj
    dunder_int = _object_type_getattr(obj, "__int__")
    if dunder_int is _Unbound:
        raise TypeError(f"an integer is required (got type {_type(obj).__name__})")
    result = dunder_int()
    if _int_checkexact(result):
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
        pass

    __hash__ = None

    def __new__(cls, source=_Unbound) -> _strarray:  # noqa: F821
        pass

    def __repr__(self) -> str:
        if _type(self) is not _strarray:
            raise TypeError("'__repr__' requires a '_strarray' object")
        return f"_strarray('{self.__str__()}')"

    def __str__(self) -> str:  # noqa: T484
        pass


class _structseq_field:
    def __get__(self, instance, owner):
        if instance is None:
            return self
        if self.index is not None:
            return instance[self.index]
        return _instance_getattr(instance, self.name)

    def __init__(self, name, index):
        self.name = name
        self.index = index

    def __set__(self, instance, value):
        raise TypeError("readonly attribute")


def _structseq_getitem(self, pos):
    if pos < 0 or pos >= _type(self).n_fields:
        raise IndexError("index out of bounds")
    if pos < len(self):
        return self[pos]
    else:
        name = self._structseq_field_names[pos]
        return _instance_getattr(self, name)


def _structseq_new(cls, sequence, dict={}):  # noqa B006
    seq_tuple = tuple(sequence)
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

    # Create the tuple of size min_len
    structseq = tuple.__new__(cls, seq_tuple[:min_len])

    # Fill the rest of the hidden fields
    for i in range(min_len, seq_len):
        key = cls._structseq_field_names[i]
        _instance_setattr(structseq, key, seq_tuple[min_len])

    # Fill the remaining from the dict or set to None
    for i in range(seq_len, max_len):
        key = cls._structseq_field_names[i]
        _instance_setattr(structseq, key, dict.get(key))

    return structseq


def _structseq_repr(self):
    _tuple_guard(self)
    if not hasattr(self, "n_sequence_fields"):
        raise TypeError("__repr__(): self is not a self")
    # TODO(T40273054): Iterate attributes and return field names
    tuple_values = ", ".join([repr(i) for i in self])
    return f"{_type(self).__name__}({tuple_values})"


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


@_patch
def bin(number) -> str:
    pass


class bool(int, bootstrap=True):
    def __new__(cls, val=False):
        pass

    def __repr__(self):
        return "True" if self else "False"

    def __str__(self) -> str:  # noqa: T484
        return bool.__repr__(self)


class bytearray(bootstrap=True):
    def __add__(self, other) -> bytearray:
        pass

    def __contains__(self, key):
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
        pass

    def __ge__(self, value):
        pass

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

    def __gt__(self, value):
        pass

    __hash__ = None

    def __iadd__(self, other) -> bytearray:
        pass

    def __imul__(self, n: int) -> bytearray:
        pass

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
        pass

    def __le__(self, value):
        pass

    def __len__(self) -> int:
        pass

    def __lt__(self, value):
        pass

    def __mod__(self, value):
        _unimplemented()

    def __mul__(self, n: int) -> bytearray:
        pass

    def __ne__(self, value):
        pass

    def __new__(cls, source=_Unbound, encoding=_Unbound, errors=_Unbound):
        pass

    def __repr__(self):
        pass

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
        _unimplemented()

    def capitalize(self):
        _unimplemented()

    def center(self):
        _unimplemented()

    def clear(self):
        _bytearray_guard(self)
        _bytearray_clear(self)

    def copy(self):
        _unimplemented()

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
        pass

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
        _unimplemented()

    def lstrip(self, bytes=None):
        pass

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
        pass

    def split(self, sep=None, maxsplit=-1):
        _unimplemented()

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
        pass

    def swapcase(self):
        _unimplemented()

    def title(self):
        _unimplemented()

    def translate(self, table, delete=b""):
        pass

    def upper(self):
        _unimplemented()

    def zfill(self):
        _unimplemented()


class bytearray_iterator(bootstrap=True):
    def __iter__(self):
        pass

    def __length_hint__(self):
        pass

    def __next__(self):
        pass


class bytes(bootstrap=True):
    def __add__(self, other: bytes) -> bytes:
        pass

    def __contains__(self, key):
        _unimplemented()

    def __eq__(self, other):
        pass

    def __ge__(self, other):
        pass

    def __getitem__(self, key):
        _bytes_guard(self)
        if _int_check(key):
            return _bytes_getitem(self, key)
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

    def __gt__(self, other):
        pass

    def __hash__(self):
        pass

    def __iter__(self):
        pass

    def __le__(self, other):
        pass

    def __len__(self) -> int:
        pass

    def __lt__(self, other):
        pass

    def __mod__(self, n):
        _unimplemented()

    def __mul__(self, n: int) -> bytes:
        pass

    def __ne__(self, other):
        pass

    def __new__(cls, source=_Unbound, encoding=_Unbound, errors=_Unbound):  # noqa: C901
        if not _type_check(cls):
            raise TypeError(
                f"bytes.__new__(X): X is not a type object ({_type(cls).__name__})"
            )
        if not _type_issubclass(cls, bytes):
            raise TypeError(
                f"bytes.__new__({cls.__name__}): "
                f"{cls.__name__} is not a subtype of bytes"
            )
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
        pass

    def __rmod__(self, n):
        _unimplemented()

    def __rmul__(self, n: int) -> bytes:
        _bytes_guard(self)
        return bytes.__mul__(self, n)

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
        _unimplemented()

    def hex(self) -> str:
        pass

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
        _unimplemented()

    def lstrip(self, bytes=None):
        pass

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
        _unimplemented()

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
        pass

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
        pass

    def swapcase(self):
        _unimplemented()

    def title(self):
        _unimplemented()

    def translate(self, table, delete=b""):
        pass

    def upper(self):
        _unimplemented()

    def zfill(self):
        _unimplemented()


class bytes_iterator(bootstrap=True):
    def __iter__(self):
        pass

    def __length_hint__(self):
        pass

    def __next__(self):
        pass


@_patch
def callable(f):
    pass


@_patch
def chr(c):
    pass


class code(bootstrap=True):
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
            code = _getframe_function(1).__code__
            flags |= code.co_flags & _compile_flags_mask
        except ValueError:
            pass  # May have been called on a fresh stackframe.

    return compile(source, filename, mode, flags, optimize)


class complex(bootstrap=True):
    def __add__(self, other):
        pass

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

    def __hash__(self) -> int:
        pass

    def __new__(cls, real=0.0, imag=0.0):
        pass

    __radd__ = __add__

    def __repr__(self):
        return f"({self.real}+{self.imag}j)"

    imag = _property(_complex_imag)

    real = _property(_complex_real)


copyright = ""


class coroutine(bootstrap=True):
    def send(self, value):
        pass

    def throw(self, exc, value=_Unbound, tb=_Unbound):
        pass

    def __repr__(self):
        return f"<coroutine object {self.__qualname__} at {_address(self):#x}>"


credits = ""


@_patch
def delattr(obj, name):
    pass


class dict(bootstrap=True):
    def __contains__(self, key) -> bool:
        _dict_guard(self)
        return _dict_get(self, key, _Unbound) is not _Unbound  # noqa: T484

    def __delitem__(self, key):
        pass

    def __eq__(self, other):
        pass

    def __getitem__(self, key):
        _dict_guard(self)
        result = _dict_get(self, key, _Unbound)
        if result is _Unbound:
            if not _dict_checkexact(self):
                dunder_missing = _object_type_getattr(self, "__missing__")
                if dunder_missing is not _Unbound:
                    # Subclass defined __missing__
                    return dunder_missing(key)
            raise KeyError(key)
        return result

    __hash__ = None

    def __init__(self, *args, **kwargs):
        if len(args) > 1:
            raise TypeError("dict expected at most 1 positional argument, got 2")
        if len(args) == 1:
            dict.update(self, args[0])
        dict.update(self, kwargs)

    def __iter__(self):
        pass

    def __len__(self):
        pass

    def __new__(cls, *args, **kwargs):
        pass

    def __repr__(self):
        if _repr_enter(self):
            return "{...}"
        kwpairs = [f"{key!r}: {self[key]!r}" for key in self.keys()]
        _repr_leave(self)
        return "{" + ", ".join(kwpairs) + "}"

    __setitem__ = _dict_setitem

    def clear(self):
        pass

    def copy(self):
        _dict_guard(self)
        result = {}
        result.update(self)
        return result

    get = _dict_get

    def items(self):
        pass

    def keys(self):
        pass

    def pop(self, key, default=_Unbound):
        _dict_guard(self)
        value = _dict_get(self, key, default)
        if value is _Unbound:
            raise KeyError(key)
        if key in self:
            dict.__delitem__(self, key)
        return value

    def popitem(self):
        _dict_guard(self)
        result = _dict_popitem(self)
        if result is None:
            raise KeyError("popitem(): dictionary is empty")
        return result

    def setdefault(self, key, default=None):
        _dict_guard(self)
        value = _dict_get(self, key, _Unbound)
        if value is _Unbound:
            _dict_setitem(self, key, default)
            return default
        return value

    def update(self, seq=_Unbound):
        if _dict_update(self, seq) is not _Unbound:
            return
        if seq is _Unbound:
            return
        if hasattr(seq, "keys"):
            for key in seq.keys():
                _dict_setitem(self, key, seq[key])
            return None
        num_items = 0
        for x in iter(seq):
            item = tuple(x)
            if _tuple_len(item) != 2:
                raise ValueError(
                    f"dictionary update sequence element #{num_items} has length "
                    f"{_tuple_len(item)}; 2 is required"
                )
            _dict_setitem(self, *item)
            num_items += 1

    def values(self):
        pass


class dict_itemiterator(bootstrap=True):
    def __iter__(self):
        pass

    def __length_hint__(self):
        pass

    def __next__(self):
        pass


class dict_items(bootstrap=True):
    def __iter__(self):
        pass


class dict_keyiterator(bootstrap=True):
    def __iter__(self):
        pass

    def __length_hint__(self):
        pass

    def __next__(self):
        pass


class dict_keys(bootstrap=True):
    def __iter__(self):
        pass


class dict_valueiterator(bootstrap=True):
    def __iter__(self):
        pass

    def __length_hint__(self):
        pass

    def __next__(self):
        pass


class dict_values(bootstrap=True):
    def __iter__(self):
        pass


def dir(obj=_Unbound):
    if obj is _Unbound:
        names = _getframe_locals(1).keys()
    else:
        names = _type(obj).__dir__(obj)
    return sorted(names)


divmod = _divmod


class ellipsis(bootstrap=True):
    def __repr__(self):
        return "Ellipsis"


class enumerate:
    def __init__(self, iterable, start=0):
        self.iterator = iter(iterable)
        self.index = start

    def __iter__(self):
        return self

    def __next__(self):
        result = (self.index, next(self.iterator))
        self.index += 1
        return result


def eval(source, globals=None, locals=None):
    if globals is None:
        caller = _getframe_function(1)
        mod = caller.__module_object__
        if locals is None:
            locals = _getframe_locals(1)
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
            caller = _getframe_function(1)
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
        caller = _getframe_function(1)
        mod = caller.__module_object__
        if locals is None:
            locals = _getframe_locals(1)
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
            caller = _getframe_function(1)
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


def exit():
    _unimplemented()


class filter:
    """filter(function or None, iterable) --> filter object

    Return an iterator yielding those items of iterable for which function(item)
    is true. If function is None, return the items that are true.
    """

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


class float(bootstrap=True):
    def __abs__(self) -> float:
        pass

    def __add__(self, n: float) -> float:
        pass

    def __bool__(self) -> bool:
        pass

    def __eq__(self, n: float) -> bool:  # noqa: T484
        pass

    def __divmod__(self, n) -> float:
        _float_guard(self)
        if _float_check(n):
            return _float_divmod(self, n)
        if _int_check(n):
            return _float_divmod(self, int.__float__(n))
        return NotImplemented

    def __float__(self) -> float:
        pass

    def __floordiv__(self, n) -> float:
        _float_guard(self)
        if _float_check(n):
            return _float_divmod(self, n)[0]
        if _int_check(n):
            return _float_divmod(self, int.__float__(n))[0]
        return NotImplemented

    def __ge__(self, n: float) -> bool:
        pass

    @classmethod
    def __getformat__(cls: type, typearg: str) -> str:
        _type_guard(cls)
        if not _type_issubclass(cls, float):
            raise TypeError(f"'__getformat__' {cls.__name__} is not a subtype of float")
        _str_guard(typearg)
        typearg = str(typearg)
        if typearg != "double" and typearg != "float":
            raise ValueError("__getformat__() argument 1 must be 'double' or 'float'")
        return f"IEEE, {_sys.byteorder}-endian"

    def __gt__(self, n: float) -> bool:
        pass

    def __hash__(self) -> int:
        pass

    def __int__(self) -> int:
        pass

    def __le__(self, n: float) -> bool:
        pass

    def __lt__(self, n: float) -> bool:
        pass

    def __mod__(self, n) -> float:
        _float_guard(self)
        if _float_check(n):
            return _float_divmod(self, n)[1]
        if _int_check(n):
            return _float_divmod(self, int.__float__(n))[1]
        return NotImplemented

    def __mul__(self, n: float) -> float:
        pass

    def __ne__(self, n: float) -> float:  # noqa: T484
        _float_guard(self)
        if not _float_check(n) and not _int_check(n):
            return NotImplemented
        return not float.__eq__(self, n)  # noqa: T484

    def __neg__(self) -> float:
        pass

    def __new__(cls, arg=0.0) -> float:
        if not _type_check(cls):
            raise TypeError(
                f"float.__new__(X): X is not a type object ({_type(cls).__name__})"
            )
        if not _type_issubclass(cls, float):
            raise TypeError(
                f"float.__new__({cls.__name__}): {cls.__name__} is not a "
                "subtype of float"
            )
        if _str_checkexact(arg):
            return _float_new_from_str(cls, arg)
        if _float_checkexact(arg):
            return _float_new_from_float(cls, arg)
        dunder_float = _object_type_getattr(arg, "__float__")
        if dunder_float is not _Unbound:
            result = dunder_float()
            if _float_checkexact(result):
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

    def __pow__(self, other, mod=None) -> float:
        pass

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
        pass

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
        pass

    def __str__(self) -> str:
        if not _float_check(self):
            raise TypeError(
                f"'__str__' requires a 'float' object "
                f"but received a '{_type(self).__name__}'"
            )
        return _float_format(self, "r", 0, False, True, False)

    def __sub__(self, n: float) -> float:
        pass

    def __truediv__(self, n: float) -> float:
        pass

    def __trunc__(self) -> int:
        pass


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
        pass

    def __contains__(self, value):
        pass

    def __eq__(self, other):
        pass

    def __ge__(self, other):
        pass

    def __gt__(self, other):
        pass

    def __hash__(self) -> int:
        pass

    def __iter__(self):
        pass

    def __le__(self, other):
        pass

    def __len__(self):
        pass

    def __lt__(self, other):
        pass

    def __ne__(self, other):
        pass

    def __new__(cls, iterable=_Unbound):
        pass

    def copy(self):
        pass

    def intersection(self, other):
        pass

    def isdisjoint(self, other):
        pass

    def issuperset(self, other):
        _frozenset_guard(self)
        if not _set_check(other) or not _frozenset_check(other):
            other = set(other)
        return set.__ge__(self, other)


class generator(bootstrap=True):
    def __iter__(self):
        pass

    def __next__(self):
        pass

    def send(self, value):
        pass

    def throw(self, exc, value=_Unbound, tb=_Unbound):
        pass

    def __repr__(self):
        return f"<generator object {self.__qualname__} at {_address(self):#x}>"


@_patch
def getattr(obj, key, default=_Unbound):
    pass


def globals():
    return _getframe_function(1).__module_object__.__dict__


@_patch
def hasattr(obj, name):
    pass


@_patch
def hash(obj) -> int:
    pass


def help(obj=_Unbound):
    _unimplemented()


@_patch
def hex(number) -> str:
    pass


@_patch
def id(obj):
    pass


def input(prompt=None):
    _unimplemented()


class instance_proxy:
    def __contains__(self, key):
        instance = self._instance
        _instance_guard(instance)
        return _instance_getattr(instance, key) is not _Unbound

    def __delitem__(self, key):
        instance = self._instance
        _instance_guard(instance)
        _instance_delattr(instance, key)

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
        # TODO(T53507197): Use _sequence_repr
        if _repr_enter(self):
            return "{...}"
        kwpairs = [f"{key!r}: {value!r}" for key, value in self.items()]
        _repr_leave(self)
        return "instance_proxy({" + ", ".join(kwpairs) + "})"

    def __setitem__(self, key, value):
        instance = self._instance
        _instance_guard(instance)
        _instance_setattr(instance, key, value)

    def clear(self):
        instance = self._instance
        _instance_guard(instance)
        for key in _object_keys(instance):
            _instance_delattr(instance, key)

    def update(self, d):
        instance = self._instance
        _instance_guard(instance)
        for key, value in d.items():
            _instance_setattr(instance, key, value)

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
        pass

    def __add__(self, n: int) -> int:
        pass

    def __and__(self, n: int) -> int:
        pass

    def __bool__(self) -> bool:
        pass

    def __ceil__(self) -> int:
        pass

    def __divmod__(self, n: int):
        pass

    def __eq__(self, n: int) -> bool:  # noqa: T484
        pass

    def __float__(self) -> float:
        pass

    def __floor__(self) -> int:
        pass

    def __floordiv__(self, n: int) -> int:
        pass

    def __format__(self, format_spec: str) -> str:
        pass

    def __ge__(self, n: int) -> bool:
        pass

    def __gt__(self, n: int) -> bool:
        pass

    def __hash__(self) -> int:
        pass

    def __index__(self) -> int:
        pass

    def __int__(self) -> int:
        pass

    def __invert__(self) -> int:
        pass

    def __le__(self, n: int) -> bool:
        pass

    def __lshift__(self, n: int) -> int:
        pass

    def __lt__(self, n: int) -> bool:
        pass

    def __mod__(self, n: int) -> int:
        pass

    def __mul__(self, n: int) -> int:
        pass

    def __ne__(self, n: int) -> int:  # noqa: T484
        pass

    def __neg__(self) -> int:
        pass

    def __new__(cls, x=_Unbound, base=_Unbound) -> int:  # noqa: C901
        if cls is bool:
            raise TypeError("int.__new__(bool) is not safe. Use bool.__new__()")
        if not _type_check(cls):
            raise TypeError(
                f"int.__new__(X): X is not a type object ({_type(cls).__name__})"
            )
        if not issubclass(cls, int):
            raise TypeError(
                f"bytes.__new__({cls.__name__}): {cls.__name__} is not a subtype of int"
            )
        if x is _Unbound:
            if base is _Unbound:
                return _int_new_from_int(cls, 0)
            raise TypeError("int() missing string argument")
        if base is _Unbound:
            if _int_checkexact(x):
                return _int_new_from_int(cls, x)
            if _object_type_hasattr(x, "__int__"):
                return _int_new_from_int(cls, _int(x))
            dunder_trunc = _object_type_getattr(x, "__trunc__")
            if dunder_trunc is not _Unbound:
                result = dunder_trunc()
                if _int_checkexact(result) and cls is int:
                    return result
                if _int_check(result):
                    return _int_new_from_int(cls, result)
                if _object_type_hasattr(result, "__int__"):
                    return _int_new_from_int(cls, _int(result))
                raise TypeError(
                    f"__trunc__ returned non-Integral (type {_type(result).__name__})"
                )
            if _str_check(x):
                return _int_new_from_str(cls, x, 10)
            if _bytes_check(x):
                return _int_new_from_bytes(cls, x, 10)
            if _bytearray_check(x):
                return _int_new_from_bytearray(cls, x, 10)
            raise TypeError(
                f"int() argument must be a string, a bytes-like object "
                f"or a number, not {_type(x).__name__}"
            )
        base = _index(base)
        if base > 36 or (base < 2 and base != 0):
            raise ValueError("int() base must be >= 2 and <= 36")
        if _str_check(x):
            return _int_new_from_str(cls, x, base)
        if _bytes_check(x):
            return _int_new_from_bytes(cls, x, base)
        if _bytearray_check(x):
            return _int_new_from_bytearray(cls, x, base)
        raise TypeError("int() can't convert non-string with explicit base")

    def __or__(self, n: int) -> int:
        pass

    def __pos__(self) -> int:
        pass

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
        pass

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
        pass

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
        pass

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
        pass

    def __sub__(self, n: int) -> int:
        pass

    def __truediv__(self, other):
        pass

    def __trunc__(self) -> int:
        pass

    def __xor__(self, n: int) -> int:
        pass

    def bit_length(self) -> int:
        pass

    def conjugate(self) -> int:
        pass

    @_property
    def denominator(self) -> int:
        return 1  # noqa: T484

    @classmethod
    def from_bytes(
        cls: type, bytes: bytes, byteorder: str, *, signed: bool = False
    ) -> int:
        _type_guard(cls)
        if not _type_issubclass(cls, int):
            raise TypeError(f"'from_bytes' {cls.__name__} is not a subtype of int")
        if not _str_check(byteorder):
            raise TypeError(
                f"from_bytes() argument 2 must be str, not "
                f"{type(byteorder).__name__}"
            )
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
        pass


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
        pass

    def __contains__(self, value):
        pass

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

    def __init__(self, iterable=()):
        self.extend(iterable)

    def __iter__(self):
        pass

    def __len__(self):
        pass

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
        pass

    def __imul__(self, other):
        pass

    def __new__(cls, iterable=()):
        pass

    def __repr__(self):
        if _repr_enter(self):
            return "[...]"
        result = "[" + ", ".join([repr(i) for i in self]) + "]"
        _repr_leave(self)
        return result

    def __setitem__(self, key, value):
        pass

    def append(self, other):
        pass

    def clear(self):
        pass

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
        _list_guard(self)
        if _tuple_checkexact(other) or _list_checkexact(other):
            return _list_extend(self, other)
        for item in other:
            list.append(self, item)

    def index(self, obj, start=0, end=_Unbound):
        _list_guard(self)
        length = _list_len(self)
        # enforce type int to avoid custom comparators
        i = (
            start
            if _int_checkexact(start)
            else _int_new_from_int(int, _slice_index_not_none(start))
        )
        if end is _Unbound:
            end = length
        elif not _int_checkexact(end):
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
        pass

    def pop(self, index=_Unbound):
        pass

    def remove(self, value):
        pass

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
        pass

    def __length_hint__(self):
        pass

    def __next__(self):
        pass


def locals():
    return _getframe_locals(1)


class longrange_iterator(bootstrap=True):
    def __iter__(self):
        pass

    def __length_hint__(self):
        pass

    def __next__(self):
        pass


class map:
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


class memoryview(bootstrap=True):
    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, exc_tb):
        memoryview.release(self)

    def __getitem__(self, index):
        pass

    def __len__(self) -> int:
        pass

    def __new__(cls, object):
        pass

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
        pass

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
        pass

    def __init__(self, name):
        pass

    def __new__(cls, *args, **kwargs):
        pass

    def __repr__(self):
        return _module_repr(self)

    def __setattr__(self, name, value):
        pass


class module_proxy(bootstrap=True):
    def __contains__(self, key) -> bool:
        _module_proxy_guard(self)
        return _module_proxy_get(self, key, _Unbound) is not _Unbound  # noqa: T484

    def __delitem__(self, key):
        _module_proxy_guard(self)
        _module_proxy_delitem(self, key)

    def __eq__(self, other):
        _unimplemented()

    def __getitem__(self, key):
        _module_proxy_guard(self)
        result = _module_proxy_get(self, key, _Unbound)
        if result is _Unbound:
            raise KeyError(key)
        return result

    __hash__ = None

    # module_proxy is not designed to be instantiated by the managed code.
    __init__ = None

    def __iter__(self):
        _module_proxy_guard(self)
        # TODO(T50379702): Return an iterable to avoid materializing a list of keys.
        return iter(_module_proxy_keys(self))

    def __len__(self):
        _module_proxy_guard(self)
        return _module_proxy_len(self)

    # module_proxy is not designed to be subclassed.
    __new__ = None

    def __repr__(self):
        _module_proxy_guard(self)
        if _repr_enter(self):
            return "{...}"
        kwpairs = [f"{key!r}: {self[key]!r}" for key in _module_proxy_keys(self)]
        _repr_leave(self)
        return "{" + ", ".join(kwpairs) + "}"

    def __setitem__(self, key, value):
        _module_proxy_guard(self)
        _module_proxy_setitem(self, key, value)

    def clear(self):
        _unimplemented()

    def copy(self):
        _module_proxy_guard(self)
        # TODO(T50379702): Return an iterable to avoid materializing the list of items.
        keys = _module_proxy_keys(self)
        values = _module_proxy_values(self)
        return {keys[i]: values[i] for i in range(len(keys))}

    def get(self, key, default=None):
        _module_proxy_guard(self)
        return _module_proxy_get(self, key, default)

    def items(self):
        _module_proxy_guard(self)
        # TODO(T50379702): Return an iterable to avoid materializing the list of items.
        keys = _module_proxy_keys(self)
        values = _module_proxy_values(self)
        return [(keys[i], values[i]) for i in range(len(keys))]

    def keys(self):
        _module_proxy_guard(self)
        # TODO(T50379702): Return an iterable to avoid materializing the list of keys.
        return _module_proxy_keys(self)

    def pop(self, key, default=_Unbound):
        _module_proxy_guard(self)
        value = _module_proxy_get(self, key, default)
        if value is _Unbound:
            raise KeyError(key)
        if key in self:
            _module_proxy_delitem(self, key)
        return value

    def setdefault(self, key, default=None):
        _module_proxy_guard(self)
        value = _module_proxy_get(self, key, _Unbound)
        if value is _Unbound:
            _module_proxy_setitem(self, key, default)
            return default
        return value

    def update(self, other=_Unbound):
        _module_proxy_guard(self)
        if other is _Unbound:
            return
        if hasattr(other, "keys"):
            for key in other.keys():
                _module_proxy_setitem(self, key, other[key])
            return
        num_items = 0
        for x in other:
            item = tuple(x)
            if _tuple_len(item) != 2:
                raise ValueError(
                    f"dictionary update sequence element #{num_items} has length "
                    f"{_tuple_len(item)}; 2 is required"
                )
            _module_proxy_setitem(self, *item)
            num_items += 1

    def values(self):
        _module_proxy_guard(self)
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


@_patch
def oct(number) -> str:
    pass


@_patch
def ord(c):
    pass


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

    def __contains__(self, num):
        _range_guard(self)
        if _int_checkexact(num) or _bool_check(num):
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
        pass

    def __le__(self, other):
        _range_guard(self)
        return NotImplemented

    def __len__(self):
        pass

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
        pass

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

    def count(self, value):
        _range_guard(self)
        if _int_checkexact(value) or _bool_check(value):
            return 1 if range.__contains__(self, value) else 0
        seen = 0
        for i in self:
            if i == value:
                seen += 1
        return seen

    def index(self, value):
        _range_guard(self)
        if _int_checkexact(value) or _bool_check(value):
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
        pass

    def __length_hint__(self):
        pass

    def __next__(self):
        pass


def repr(obj):
    result = _object_type_getattr(obj, "__repr__")()
    if not _str_check(result):
        raise TypeError("__repr__ returned non-string")
    return result


class reversed:
    def __iter__(self):
        return self

    def __new__(cls, seq, **kwargs):
        dunder_reversed = _object_type_getattr(seq, "__reversed__")
        if dunder_reversed is None:
            raise TypeError(f"'{_type(seq).__name__}' object is not reversible")
        if dunder_reversed is not _Unbound:
            return dunder_reversed()
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
        pass

    def __contains__(self, value):
        pass

    def __eq__(self, other):
        pass

    def __ge__(self, other):
        pass

    def __gt__(self, other):
        pass

    __hash__ = None

    def __iand__(self, other):
        pass

    def __init__(self, iterable=()):
        pass

    def __iter__(self):
        pass

    def __le__(self, other):
        pass

    def __len__(self):
        pass

    def __lt__(self, other):
        pass

    def __ne__(self, other):
        pass

    def __new__(cls, iterable=()):
        pass

    def __ior__(self, other):
        _set_guard(self)
        if not _set_check(other) and not _frozenset_check(other):
            return NotImplemented
        if self is other:
            return self
        set.update(self, other)
        return self

    def __or__(self, other):
        _set_guard(self)
        if not _set_check(other) and not _frozenset_check(other):
            return NotImplemented
        result = set.copy(self)
        if self is other:
            return result
        set.update(result, other)
        return result

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

    def __sub__(self, other):
        _set_guard(self)
        if not _set_check(other):
            return NotImplemented
        return set.difference(self, other)

    def add(self, value):
        pass

    def clear(self):
        pass

    def copy(self):
        pass

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
        pass

    def intersection(self, other):
        pass

    def isdisjoint(self, other):
        pass

    def issuperset(self, other):
        _set_guard(self)
        if not _set_check(other) or not _frozenset_check(other):
            other = set(other)
        return set.__ge__(self, other)

    def pop(self):
        pass

    def remove(self, elt):
        pass

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
        pass


class set_iterator(bootstrap=True):
    def __iter__(self):
        pass

    def __length_hint__(self):
        pass

    def __next__(self):
        pass


@_patch
def setattr(obj, name, value):
    pass


class slice(bootstrap=True):
    __hash__ = None

    def __new__(cls, start_or_stop, stop=_Unbound, step=None):
        pass

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
        pass

    def __bool__(self):
        pass

    def __contains__(self, other):
        _str_guard(self)
        if not _str_check(other):
            raise TypeError(
                "'in <string>' requires string as left operand, not "
                f"{_type(other).__name__}"
            )
        return _str_find(self, other, None, None) != -1

    def __eq__(self, other):
        pass

    def __format__(self, format_spec: str) -> str:
        pass

    def __ge__(self, other):
        pass

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

    def __gt__(self, other):
        pass

    def __hash__(self):
        pass

    def __iter__(self):
        pass

    def __le__(self, other):
        pass

    def __len__(self) -> int:
        pass

    def __lt__(self, other):
        pass

    def __mod__(self, other):
        if not _str_check(self):
            raise TypeError(
                f"'__mod__' requires a 'str' object "
                f"but received a '{_type(self).__name__}'"
            )
        return _str_mod.format(self, other)

    def __mul__(self, n: int) -> str:
        pass

    def __ne__(self, other):
        pass

    def __new__(cls, obj=_Unbound, encoding=_Unbound, errors=_Unbound):  # noqa: C901
        if not _type_check(cls):
            raise TypeError("cls is not a type object")
        if not issubclass(cls, str):
            raise TypeError("cls is not a subtype of str")
        if obj is _Unbound:
            return _str_from_str(cls, "")
        if encoding is _Unbound and errors is _Unbound:
            if _str_checkexact(obj):
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
        pass

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
        _unimplemented()

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
        pass

    def isalpha(self):
        pass

    def isdecimal(self):
        pass

    def isdigit(self):
        pass

    def isidentifier(self):
        pass

    def islower(self):
        pass

    def isnumeric(self):
        pass

    def isprintable(self):
        pass

    def isspace(self):
        pass

    def istitle(self):
        pass

    def isupper(self):
        pass

    def join(self, items) -> str:
        _str_guard(self)
        if _tuple_checkexact(tuple) or _list_checkexact(items):
            return _str_join(self, items)
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
        pass

    def lstrip(self, other=None):
        pass

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
        if count:
            count = _index(count)
        else:
            count = -1
        result = _str_replace(self, old, new, count)
        return str(result) if self is result else result

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
        pass

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
        pass

    def swapcase(self):
        _unimplemented()

    def title(self):
        _unimplemented()

    def translate(self, table):
        _dict_guard(table)
        result = _strarray()
        for key in self:
            value = table.get(ord(key), key)
            if value is None:
                continue
            if _int_check(value):
                value = chr(value)
            elif not _str_check(value):
                raise TypeError("character dict must return int, None or str")
            _strarray_iadd(result, value)
        return str(result)

    def upper(self):
        pass

    def zfill(self, width):
        _unimplemented()


class str_iterator(bootstrap=True):
    def __iter__(self):
        pass

    def __length_hint__(self):
        pass

    def __next__(self):
        pass


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
        pass

    def __getattribute__(self, name):
        pass

    def __new__(cls, type=_Unbound, type_or_obj=_Unbound):
        pass


class tuple(bootstrap=True):
    def __add__(self, other):
        pass

    def __contains__(self, key):
        pass

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
        pass

    def __iter__(self):
        pass

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
        pass

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
        pass

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
        if not _type_check(cls):
            raise TypeError(
                f"tuple.__new__(X): X is not a type object ({_type(cls).__name__})"
            )
        if not _type_issubclass(cls, tuple):
            raise TypeError(
                f"tuple.__new__(X): {_type(cls).__name__} is not a subtype of tuple"
            )
        if cls is tuple:
            if _tuple_checkexact(iterable):
                return iterable
            return (*iterable,)
        if _tuple_checkexact(iterable):
            return _tuple_new(cls, iterable)
        return _tuple_new(cls, (*iterable,))

    def __repr__(self):
        # TODO(T53507197): Use _sequence_repr
        _tuple_guard(self)
        if _repr_enter(self):
            return "(...)"
        num_elems = _tuple_len(self)
        output = "("
        i = 0
        while i < num_elems:
            if i != 0:
                output += ", "
            output += repr(self[i])
            i += 1
        _repr_leave(self)
        if num_elems == 1:
            output += ","
        return output + ")"


class tuple_iterator(bootstrap=True):
    def __iter__(self):
        pass

    def __length_hint__(self):
        pass

    def __next__(self):
        pass


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
        return _getframe_locals(1)
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
        result = [None] * length
        i = 0
        while i < length:
            result[i] = next(iterators[i])
            i += 1
        return (*result,)
