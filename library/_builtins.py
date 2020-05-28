#!/usr/bin/env python3


# These values are injected by our boot process. flake8 has no knowledge about
# their definitions and will complain without these circular assignments.
_Unbound = _Unbound  # noqa: F821


def _ContextVar_guard(obj):
    _builtin()


def _Token_guard(obj):
    _builtin()


def _builtin():
    """This function acts as a marker to `freeze_modules.py` it should never
    actually be called."""
    _unimplemented()


def _address(c):
    _builtin()


def _anyset_check(obj):
    _builtin()


def _bool_check(self):
    _builtin()


def _bool_guard(self):
    _builtin()


def _bound_method(fn, owner):
    _builtin()


def _byte_guard(obj):
    _builtin()


def _bytearray_append(obj, item):
    _builtin()


def _bytearray_check(obj):
    _builtin()


def _bytearray_clear(obj):
    _builtin()


def _bytearray_contains(obj, key):
    _builtin()


def _bytearray_copy(obj):
    _builtin()


def _bytearray_delitem(self, key):
    _builtin()


def _bytearray_delslice(self, start, stop, step):
    _builtin()


def _bytearray_getitem(self, key):
    _builtin()


def _bytearray_getslice(self, start, stop, step):
    _builtin()


def _bytearray_guard(obj):
    _builtin()


def _bytearray_join(self, iterable):
    _builtin()


def _bytearray_len(self):
    _builtin()


def _bytearray_ljust(self, width, fillbyte):
    _builtin()


def _bytearray_rjust(self, width, fillbyte):
    _builtin()


def _bytearray_setitem(self, key, value):
    _builtin()


def _bytearray_setslice(self, start, stop, step, value):
    _builtin()


def _bytes_check(obj):
    _builtin()


def _bytes_contains(obj, key):
    _builtin()


def _bytes_decode(obj, encoding):
    _builtin()


def _bytes_decode_ascii(obj):
    _builtin()


def _bytes_decode_utf_8(obj):
    _builtin()


def _bytes_from_bytes(cls, value):
    _builtin()


def _bytes_from_ints(source):
    _builtin()


def _bytes_getitem(self, index):
    _builtin()


def _bytes_getslice(self, start, stop, step):
    _builtin()


def _bytes_guard(obj):
    _builtin()


def _bytes_join(self, iterable):
    _builtin()


def _bytes_len(self):
    _builtin()


def _bytes_ljust(self, width, fillbyte):
    _builtin()


def _bytes_maketrans(frm, to):
    _builtin()


def _bytes_repeat(self, count):
    _builtin()


def _bytes_replace(self, old, new, count):
    _builtin()


def _bytes_split(self, sep, maxsplit):
    _builtin()


def _bytes_split_whitespace(self, maxsplit):
    _builtin()


def _byteslike_check(obj):
    _builtin()


def _byteslike_compare_digest(a, b):
    _builtin()


def _byteslike_count(self, sub, start, end):
    _builtin()


def _byteslike_endswith(self, suffix, start, end):
    _builtin()


def _byteslike_find_byteslike(self, sub, start, end):
    _builtin()


def _byteslike_find_int(self, sub, start, end):
    _builtin()


def _byteslike_guard(obj):
    _builtin()


def _byteslike_rfind_byteslike(self, sub, start, end):
    _builtin()


def _byteslike_rfind_int(self, sub, start, end):
    _builtin()


def _byteslike_startswith(self, prefix, start, end):
    _builtin()


def _caller_function():
    _builtin()


def _caller_locals():
    _builtin()


def _classmethod(function):
    _builtin()


def _classmethod_isabstract(self):
    _builtin()


def _code_check(obj):
    _builtin()


def _code_guard(c):
    _builtin()


def _code_new(
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
):
    _builtin()


def _code_set_filename(code, filename):
    _builtin()


def _code_set_posonlyargcount(code, value):
    _builtin()


def _complex_check(obj):
    _builtin()


def _complex_checkexact(obj):
    _builtin()


def _complex_imag(c):
    _builtin()


def _complex_new(cls, imag, real):
    _builtin()


def _complex_real(c):
    _builtin()


def _dict_check(obj):
    _builtin()


def _dict_check_exact(obj):
    _builtin()


# TODO(T56301601): Move this into a type-specific file.
def _dict_get(self, key, default=None):
    _builtin()


def _dict_guard(obj):
    _builtin()


def _dict_items_guard(self):
    _builtin()


def _dict_keys_guard(self):
    _builtin()


def _dict_len(self):
    _builtin()


# TODO(T56301601): Move this into a type-specific file.
def _dict_setitem(self, key, value):
    _builtin()


# TODO(T56301601): Move this into a type-specific file.
def _dict_update(self, other, kwargs):
    _builtin()


def _divmod(number, divisor):
    _builtin()


def _exec(code, module, implicit_globals):
    _builtin()


def _float_check(obj):
    _builtin()


def _float_check_exact(obj):
    _builtin()


def _float_divmod(number, divisor):
    _builtin()


def _float_format(
    value, format_code, precision, skip_sign, add_dot_0, use_alt_formatting
):
    _builtin()


def _float_guard(obj):
    _builtin()


def _float_new_from_byteslike(cls, obj):
    _builtin()


def _float_new_from_float(cls, obj):
    _builtin()


def _float_new_from_str(cls, obj):
    _builtin()


def _float_signbit(value):
    _builtin()


def _frozenset_check(obj):
    _builtin()


def _frozenset_guard(obj):
    _builtin()


def _function_annotations(obj):
    _builtin()


def _function_closure(obj):
    _builtin()


def _function_defaults(obj):
    _builtin()


def _function_globals(obj):
    _builtin()


def _function_guard(obj):
    _builtin()


def _function_kwdefaults(obj):
    _builtin()


def _function_lineno(function, pc):
    _builtin()


def _function_new(self, code, mod, name, defaults, closure):
    _builtin()


def _function_set_annotations(obj, annotations):
    _builtin()


def _function_set_defaults(obj, defaults):
    _builtin()


def _function_set_kwdefaults(obj, kwdefaults):
    _builtin()


def _gc():
    _builtin()


def _get_member_byte(addr):
    _builtin()


def _get_member_char(addr):
    _builtin()


def _get_member_double(addr):
    _builtin()


def _get_member_float(addr):
    _builtin()


def _get_member_int(addr):
    _builtin()


def _get_member_long(addr):
    _builtin()


def _get_member_pyobject(addr, name):
    _builtin()


def _get_member_short(addr):
    _builtin()


def _get_member_string(addr):
    _builtin()


def _get_member_ubyte(addr):
    _builtin()


def _get_member_uint(addr):
    _builtin()


def _get_member_ulong(addr):
    _builtin()


def _get_member_ushort(addr):
    _builtin()


def _heap_dump(filename):
    _builtin()


def _instance_dunder_dict_set(obj, dict):
    _builtin()


def _instance_delattr(obj, name):
    _builtin()


def _instance_getattr(obj, name):
    _builtin()


def _instance_guard(obj):
    _builtin()


def _instance_overflow_dict(obj):
    _builtin()


def _instance_setattr(obj, name, value):
    _builtin()


def _int_check(obj):
    _builtin()


def _int_check_exact(obj):
    _builtin()


def _int_from_bytes(cls, bytes, byteorder_big, signed):
    _builtin()


def _int_guard(obj):
    _builtin()


def _int_new_from_bytearray(cls, x, base):
    _builtin()


def _int_new_from_bytes(cls, x, base):
    _builtin()


def _int_new_from_int(cls, value):
    _builtin()


def _int_new_from_str(cls, x, base):
    _builtin()


def _list_check(obj):
    _builtin()


def _list_check_exact(obj):
    _builtin()


def _list_delitem(self, key):
    _builtin()


def _list_delslice(self, start, stop, step):
    _builtin()


def _list_extend(self, other):
    _builtin()


def _list_getitem(self, key):
    _builtin()


def _list_getslice(self, start, stop, step):
    _builtin()


def _list_guard(obj):
    _builtin()


def _list_len(self):
    _builtin()


def _list_new(size, fill=None):
    _builtin()


def _list_setitem(self, key, value):
    _builtin()


def _list_setslice(self, start, stop, step, value):
    _builtin()


def _list_sort(list):
    _builtin()


def _list_sort_by_key(list):
    _builtin()


def _list_swap(list, i, j):
    _builtin()


def _lt(obj, other):
    "Same as obj < other."
    return obj < other


def _lt_key(obj, other):
    return _tuple_getitem(obj, 0) < _tuple_getitem(other, 0)


def _mappingproxy_guard(obj):
    _builtin()


def _mappingproxy_mapping(obj):
    _builtin()


def _mappingproxy_set_mapping(obj, mapping):
    _builtin()


def _memoryview_check(obj):
    _builtin()


def _memoryview_getitem(obj, key):
    _builtin()


def _memoryview_getslice(self, start, stop, step):
    _builtin()


def _memoryview_guard(obj):
    _builtin()


def _memoryview_itemsize(obj):
    _builtin()


def _memoryview_nbytes(self):
    _builtin()


def _memoryview_setitem(self, key, value):
    _builtin()


def _memoryview_setslice(self, start, stop, step, value):
    _builtin()


def _memoryview_start(self):
    _builtin()


def _mmap_check(obj):
    _builtin()


def _module_dir(module):
    _builtin()


def _module_proxy(module):
    _builtin()


def _module_proxy_check(obj):
    _builtin()


def _module_proxy_guard(module):
    _builtin()


def _module_proxy_keys(self):
    _builtin()


def _module_proxy_setitem(self, key, value):
    _builtin()


def _module_proxy_values(self):
    _builtin()


def _iter(self):
    _builtin()


def _object_class_set(obj, name):
    _builtin()


def _object_keys(self):
    _builtin()


def _object_type_getattr(obj, name):
    """Looks up the named attribute on the object's type, resolving descriptors.
Behaves like _PyObject_LookupSpecial."""
    _builtin()


def _object_type_hasattr(obj, name):
    _builtin()


def _os_write(fd, buf):
    _builtin()


def _os_error_subclass_from_errno(errno):
    _builtin()


def _property(fget=None, fset=None, fdel=None, doc=None):
    """Has the same effect as property(), but can be used for bootstrapping."""
    _builtin()


def _property_isabstract(self):
    _builtin()


def _pyobject_offset(instance, offset):
    _builtin()


def _range_check(obj):
    _builtin()


def _range_guard(obj):
    _builtin()


def _range_len(self):
    _builtin()


def _readline(prompt):
    _builtin()


def _repr_enter(obj):
    _builtin()


def _repr_leave(obj):
    _builtin()


def _seq_index(obj):
    _builtin()


def _seq_iterable(obj):
    _builtin()


def _seq_set_index(obj, index):
    _builtin()


def _seq_set_iterable(obj, iterable):
    _builtin()


def _set_check(obj):
    _builtin()


def _set_function_flag_iterable_coroutine(code):
    _builtin()


def _set_guard(obj):
    _builtin()


def _set_len(self):
    _builtin()


def _set_member_double(addr, value):
    _builtin()


def _set_member_float(addr, value):
    _builtin()


def _set_member_integral(addr, value, num_bytes):
    _builtin()


def _set_member_integral_unsigned(addr, value, num_bytes):
    _builtin()


def _set_member_pyobject(addr, value):
    _builtin()


def _slice_check(obj):
    _builtin()


def _slice_guard(obj):
    _builtin()


def _slice_start(start, step, length):
    _builtin()


def _slice_start_long(start, step, length):
    _builtin()


def _slice_step(step):
    _builtin()


def _slice_step_long(step):
    _builtin()


def _slice_stop(stop, step, length):
    _builtin()


def _slice_stop_long(stop, step, length):
    _builtin()


def _staticmethod_isabstract(self):
    _builtin()


def _stop_iteration_ctor(cls, *args):
    _builtin()


def _str_check(obj):
    _builtin()


def _str_check_exact(obj):
    _builtin()


def _str_compare_digest(a, b):
    _builtin()


def _str_count(self, sub, start, end):
    _builtin()


def _str_encode(self, encoding):
    _builtin()


def _str_encode_ascii(self):
    _builtin()


def _str_endswith(self, suffix, start, end):
    _builtin()


def _str_getitem(self, key):
    _builtin()


def _str_getslice(self, start, stop, step):
    _builtin()


def _str_guard(obj):
    _builtin()


def _str_ischr(obj):
    _builtin()


def _str_join(sep, iterable):
    _builtin()


def _str_escape_non_ascii(s):
    _builtin()


def _str_find(self, sub, start, end):
    _builtin()


def _str_from_str(cls, value):
    _builtin()


def _str_len(self):
    _builtin()


def _str_mod_fast_path(self, other):
    _builtin()


def _str_partition(self, sep):
    _builtin()


def _str_replace(self, old, newstr, count):
    _builtin()


def _str_rfind(self, sub, start, end):
    _builtin()


def _str_rpartition(self, sep):
    _builtin()


def _str_split(self, sep, maxsplit):
    _builtin()


def _str_splitlines(self, keepends):
    _builtin()


def _str_startswith(self, prefix, start, end):
    _builtin()


def _str_translate(obj, table):
    _builtin()


def _strarray_clear(self):
    _builtin()


def _strarray_iadd(self, other):
    _builtin()


def _strarray_ctor(cls, source=_Unbound):
    _builtin()


def _structseq_getitem(structseq, index):
    _builtin()


def _structseq_new_type(name, field_names, num_in_sequence=_Unbound):
    _builtin()


def _structseq_setitem(structseq, index, value):
    _builtin()


def _super(cls):
    _builtin()


def _tuple_check(obj):
    _builtin()


def _tuple_check_exact(obj):
    _builtin()


def _tuple_getitem(self, index):
    _builtin()


def _tuple_getslice(self, start, stop, step):
    _builtin()


def _tuple_guard(obj):
    _builtin()


def _tuple_len(self):
    _builtin()


def _tuple_new(cls, old_tuple):
    _builtin()


def _type(obj):
    _builtin()


def _type_abstractmethods_del(self):
    _builtin()


def _type_abstractmethods_get(self):
    _builtin()


def _type_abstractmethods_set(self, value):
    _builtin()


def _type_bases_del(self):
    _builtin()


def _type_bases_get(self):
    _builtin()


def _type_bases_set(self, value):
    _builtin()


def _type_check(obj):
    _builtin()


def _type_check_exact(obj):
    _builtin()


def _type_dunder_call(self, *args, **kwargs):
    _builtin()


def _type_guard(obj):
    _builtin()


def _type_issubclass(subclass, superclass):
    _builtin()


def _type_proxy(type_obj):
    _builtin()


def _type_init(metaclass_instance, name, dict, mro):
    _builtin()


def _type_new(cls, bases):
    _builtin()


def _type_proxy_check(obj):
    _builtin()


def _type_proxy_get(self, key, default):
    _builtin()


def _type_proxy_guard(obj):
    _builtin()


def _type_proxy_keys(self):
    _builtin()


def _type_proxy_len(self):
    _builtin()


def _type_proxy_values(self):
    _builtin()


def _type_subclass_guard(subclass, superclass):
    _builtin()


def _unimplemented():
    """Prints a message and a stacktrace, and stops the program execution."""
    _builtin()


def _warn(message, category=None, stacklevel=1, source=None):
    """Calls warnings.warn."""
    _builtin()


def _weakref_callback(self):
    _builtin()


def _weakref_check(self):
    _builtin()


def _weakref_guard(self):
    _builtin()


def _weakref_referent(self):
    _builtin()
