#!/usr/bin/env python3


# These values are injected by our boot process. flake8 has no knowledge about
# their definitions and will complain without these circular assignments.
_patch = _patch  # noqa: F821
_Unbound = _Unbound  # noqa: F821


@_patch
def _address(c):
    pass


@_patch
def _bool_check(self):
    pass


@_patch
def _bound_method(fn, owner):
    pass


@_patch
def _bytearray_check(obj):
    pass


@_patch
def _bytearray_clear(obj):
    pass


@_patch
def _bytearray_contains(obj, key):
    pass


@_patch
def _bytearray_delitem(self, key):
    pass


@_patch
def _bytearray_delslice(self, start, stop, step):
    pass


@_patch
def _bytearray_getitem(self, key):
    pass


@_patch
def _bytearray_getslice(self, start, stop, step):
    pass


@_patch
def _bytearray_guard(obj):
    pass


@_patch
def _bytearray_join(self, iterable):
    pass


@_patch
def _bytearray_len(self):
    pass


@_patch
def _bytearray_setitem(self, key, value):
    pass


@_patch
def _bytearray_setslice(self, start, stop, step, value):
    pass


@_patch
def _bytes_check(obj):
    pass


@_patch
def _bytes_contains(obj, key):
    pass


@_patch
def _bytes_decode(obj, encoding):
    pass


@_patch
def _bytes_decode_ascii(obj):
    pass


@_patch
def _bytes_decode_utf_8(obj):
    pass


@_patch
def _bytes_from_bytes(cls, value):
    pass


@_patch
def _bytes_from_ints(source):
    pass


@_patch
def _bytes_getitem(self, index):
    pass


@_patch
def _bytes_getslice(self, start, stop, step):
    pass


@_patch
def _bytes_guard(obj):
    pass


@_patch
def _bytes_join(self, iterable):
    pass


@_patch
def _bytes_len(self):
    pass


@_patch
def _bytes_maketrans(frm, to):
    pass


@_patch
def _bytes_repeat(self, count):
    pass


@_patch
def _bytes_split(self, sep, maxsplit):
    pass


@_patch
def _bytes_split_whitespace(self, maxsplit):
    pass


@_patch
def _byteslike_check(obj):
    pass


@_patch
def _byteslike_compare_digest(a, b):
    pass


@_patch
def _byteslike_count(self, sub, start, end):
    pass


@_patch
def _byteslike_endswith(self, suffix, start, end):
    pass


@_patch
def _byteslike_find_byteslike(self, sub, start, end):
    pass


@_patch
def _byteslike_find_int(self, sub, start, end):
    pass


@_patch
def _byteslike_guard(obj):
    pass


@_patch
def _byteslike_rfind_byteslike(self, sub, start, end):
    pass


@_patch
def _byteslike_rfind_int(self, sub, start, end):
    pass


@_patch
def _byteslike_startswith(self, prefix, start, end):
    pass


@_patch
def _classmethod(function):
    pass


@_patch
def _classmethod_isabstract(self):
    pass


@_patch
def _code_check(obj):
    pass


@_patch
def _code_guard(c):
    pass


@_patch
def _code_set_posonlyargcount(code, value):
    pass


@_patch
def _complex_check(c):
    pass


@_patch
def _complex_imag(c):
    pass


@_patch
def _complex_real(c):
    pass


@_patch
def _dict_bucket_insert(self, index, key, key_hash, value):
    pass


@_patch
def _dict_bucket_key(self, index):
    pass


@_patch
def _dict_bucket_set_value(self, index, value):
    pass


@_patch
def _dict_bucket_value(self, index):
    pass


@_patch
def _dict_check(obj):
    pass


@_patch
def _dict_check_exact(obj):
    pass


# TODO(T56301601): Move this into a type-specific file.
@_patch
def _dict_get(self, key, default=None):
    pass


@_patch
def _dict_guard(obj):
    pass


@_patch
def _dict_len(self):
    pass


@_patch
def _dict_lookup(self, key, key_hash):
    pass


@_patch
def _dict_lookup_next(self, index, key, key_hash, perturb):
    pass


@_patch
def _dict_popitem(self):
    pass


# TODO(T56301601): Move this into a type-specific file.
@_patch
def _dict_setitem(self, key, value):
    pass


# TODO(T56301601): Move this into a type-specific file.
@_patch
def _dict_update(self, other):
    pass


@_patch
def _divmod(number, divisor):
    pass


def _eq(obj, other):
    "Same as obj == other."
    return obj == other


@_patch
def _exec(code, module, implicit_globals):
    pass


@_patch
def _float_check(obj):
    pass


@_patch
def _float_check_exact(obj):
    pass


@_patch
def _float_divmod(number, divisor):
    pass


@_patch
def _float_format(
    value, format_code, precision, skip_sign, add_dot_0, use_alt_formatting
):
    pass


@_patch
def _float_guard(obj):
    pass


@_patch
def _float_new_from_byteslike(cls, obj):
    pass


@_patch
def _float_new_from_float(cls, obj):
    pass


@_patch
def _float_new_from_str(cls, obj):
    pass


@_patch
def _float_signbit(value):
    pass


@_patch
def _frozenset_check(obj):
    pass


@_patch
def _frozenset_guard(obj):
    pass


@_patch
def _function_globals(obj):
    pass


@_patch
def _function_guard(obj):
    pass


@_patch
def _gc():
    pass


@_patch
def _get_member_byte(addr):
    pass


@_patch
def _get_member_char(addr):
    pass


@_patch
def _get_member_double(addr):
    pass


@_patch
def _get_member_float(addr):
    pass


@_patch
def _get_member_int(addr):
    pass


@_patch
def _get_member_long(addr):
    pass


@_patch
def _get_member_pyobject(addr, name):
    pass


@_patch
def _get_member_short(addr):
    pass


@_patch
def _get_member_string(addr):
    pass


@_patch
def _get_member_ubyte(addr):
    pass


@_patch
def _get_member_uint(addr):
    pass


@_patch
def _get_member_ulong(addr):
    pass


@_patch
def _get_member_ushort(addr):
    pass


@_patch
def _getframe_function(depth):
    pass


@_patch
def _getframe_lineno(depth):
    pass


@_patch
def _getframe_locals(depth):
    pass


@_patch
def _instance_delattr(obj, name):
    pass


@_patch
def _instance_getattr(obj, name):
    pass


@_patch
def _instance_guard(obj):
    pass


@_patch
def _instance_overflow_dict(obj):
    pass


@_patch
def _instance_setattr(obj, name, value):
    pass


@_patch
def _int_check(obj):
    pass


@_patch
def _int_check_exact(obj):
    pass


@_patch
def _int_from_bytes(cls, bytes, byteorder_big, signed):
    pass


@_patch
def _int_guard(obj):
    pass


@_patch
def _int_new_from_bytearray(cls, x, base):
    pass


@_patch
def _int_new_from_bytes(cls, x, base):
    pass


@_patch
def _int_new_from_int(cls, value):
    pass


@_patch
def _int_new_from_str(cls, x, base):
    pass


@_patch
def _list_check(obj):
    pass


@_patch
def _list_check_exact(obj):
    pass


@_patch
def _list_delitem(self, key):
    pass


@_patch
def _list_delslice(self, start, stop, step):
    pass


@_patch
def _list_extend(obj, other):
    pass


@_patch
def _list_getitem(self, key):
    pass


@_patch
def _list_getslice(self, start, stop, step):
    pass


@_patch
def _list_guard(obj):
    pass


@_patch
def _list_len(self):
    pass


@_patch
def _list_sort(list):
    pass


@_patch
def _list_swap(list, i, j):
    pass


def _lt(obj, other):
    "Same as obj < other."
    return obj < other


@_patch
def _mappingproxy_guard(obj):
    pass


@_patch
def _mappingproxy_mapping(obj):
    pass


@_patch
def _mappingproxy_set_mapping(obj, mapping):
    pass


@_patch
def _memoryview_check(obj):
    pass


@_patch
def _memoryview_guard(obj):
    pass


@_patch
def _memoryview_itemsize(obj):
    pass


@_patch
def _memoryview_nbytes(self):
    pass


@_patch
def _memoryview_setitem(self, key, value):
    pass


@_patch
def _memoryview_setslice(self, start, stop, step, value):
    pass


@_patch
def _module_dir(module):
    pass


@_patch
def _module_proxy(module):
    pass


@_patch
def _module_proxy_check(obj):
    pass


@_patch
def _module_proxy_delitem(self, key):
    pass


@_patch
def _module_proxy_get(self, key, default):
    pass


@_patch
def _module_proxy_guard(module):
    pass


@_patch
def _module_proxy_keys(self):
    pass


@_patch
def _module_proxy_len(self):
    pass


@_patch
def _module_proxy_setitem(self, key, value):
    pass


@_patch
def _module_proxy_values(self):
    pass


@_patch
def _iter(self):
    pass


@_patch
def _object_keys(self):
    pass


@_patch
def _object_type_getattr(obj, name):
    """Looks up the named attribute on the object's type, resolving descriptors.
Behaves like _PyObject_LookupSpecial."""
    pass


@_patch
def _object_type_hasattr(obj, name):
    pass


@_patch
def _os_write(fd, buf):
    pass


# _patch is patched manually to avoid circularity problems


@_patch
def _property(fget=None, fset=None, fdel=None, doc=None):
    """Has the same effect as property(), but can be used for bootstrapping."""
    pass


@_patch
def _property_isabstract(self):
    pass


@_patch
def _pyobject_offset(instance, offset):
    pass


@_patch
def _range_check(obj):
    pass


@_patch
def _range_guard(obj):
    pass


@_patch
def _range_len(self):
    pass


@_patch
def _repr_enter(obj):
    pass


@_patch
def _repr_leave(obj):
    pass


@_patch
def _seq_index(obj):
    pass


@_patch
def _seq_iterable(obj):
    pass


@_patch
def _seq_set_index(obj, index):
    pass


@_patch
def _seq_set_iterable(obj, iterable):
    pass


@_patch
def _set_check(obj):
    pass


@_patch
def _set_guard(obj):
    pass


@_patch
def _set_len(self):
    pass


@_patch
def _set_member_double(addr, value):
    pass


@_patch
def _set_member_float(addr, value):
    pass


@_patch
def _set_member_integral(addr, value, num_bytes):
    pass


@_patch
def _set_member_pyobject(addr, value):
    pass


@_patch
def _slice_check(obj):
    pass


@_patch
def _slice_guard(obj):
    pass


@_patch
def _slice_start(start, step, length):
    pass


@_patch
def _slice_start_long(start, step, length):
    pass


@_patch
def _slice_step(step):
    pass


@_patch
def _slice_step_long(step):
    pass


@_patch
def _slice_stop(stop, step, length):
    pass


@_patch
def _slice_stop_long(stop, step, length):
    pass


@_patch
def _staticmethod_isabstract(self):
    pass


@_patch
def _stop_iteration_ctor(cls, *args):
    pass


@_patch
def _str_check(obj):
    pass


@_patch
def _str_check_exact(obj):
    pass


@_patch
def _str_compare_digest(a, b):
    pass


@_patch
def _str_count(self, sub, start, end):
    pass


@_patch
def _str_encode(self, encoding):
    pass


@_patch
def _str_encode_ascii(self):
    pass


@_patch
def _str_endswith(self, suffix, start, end):
    pass


@_patch
def _str_getitem(self, key):
    pass


@_patch
def _str_getslice(self, start, stop, step):
    pass


@_patch
def _str_guard(obj):
    pass


@_patch
def _str_ischr(obj):
    pass


@_patch
def _str_join(sep, iterable):
    pass


@_patch
def _str_escape_non_ascii(s):
    pass


@_patch
def _str_find(self, sub, start, end):
    pass


@_patch
def _str_from_str(cls, value):
    pass


@_patch
def _str_len(self):
    pass


@_patch
def _str_partition(self, sep):
    pass


@_patch
def _str_replace(self, old, newstr, count):
    pass


@_patch
def _str_rfind(self, sub, start, end):
    pass


@_patch
def _str_rpartition(self, sep):
    pass


@_patch
def _str_split(self, sep, maxsplit):
    pass


@_patch
def _str_splitlines(self, keepends):
    pass


@_patch
def _str_startswith(self, prefix, start, end):
    pass


@_patch
def _strarray_clear(self):
    pass


@_patch
def _strarray_iadd(self, other):
    pass


@_patch
def _strarray_ctor(cls, source=_Unbound):
    pass


@_patch
def _tuple_check(obj):
    pass


@_patch
def _tuple_check_exact(obj):
    pass


@_patch
def _tuple_getitem(self, index):
    pass


@_patch
def _tuple_getslice(self, start, stop, step):
    pass


@_patch
def _tuple_guard(obj):
    pass


@_patch
def _tuple_len(self):
    pass


@_patch
def _tuple_new(cls, old_tuple):
    pass


@_patch
def _type(obj):
    pass


@_patch
def _type_abstractmethods_del(self):
    pass


@_patch
def _type_abstractmethods_get(self):
    pass


@_patch
def _type_abstractmethods_set(self, value):
    pass


@_patch
def _type_bases_del(self):
    pass


@_patch
def _type_bases_get(self):
    pass


@_patch
def _type_bases_set(self, value):
    pass


@_patch
def _type_check(obj):
    pass


@_patch
def _type_check_exact(obj):
    pass


@_patch
def _type_dunder_call(self, *args, **kwargs):
    pass


@_patch
def _type_guard(obj):
    pass


@_patch
def _type_issubclass(subclass, superclass):
    pass


@_patch
def _type_proxy(type_obj):
    pass


@_patch
def _type_init(metaclass_instance, name, dict, mro):
    pass


@_patch
def _type_new(cls, bases):
    pass


@_patch
def _type_proxy_check(obj):
    pass


@_patch
def _type_proxy_get(self, key, default):
    pass


@_patch
def _type_proxy_guard(obj):
    pass


@_patch
def _type_proxy_keys(self):
    pass


@_patch
def _type_proxy_len(self):
    pass


@_patch
def _type_proxy_values(self):
    pass


@_patch
def _type_subclass_guard(subclass, superclass):
    pass


@_patch
def _unimplemented():
    """Prints a message and a stacktrace, and stops the program execution."""
    pass


@_patch
def _warn(message, category=None, stacklevel=1, source=None):
    """Calls warnings.warn."""
    pass


@_patch
def _weakref_callback(self):
    pass


@_patch
def _weakref_check(self):
    pass


@_patch
def _weakref_guard(self):
    pass


@_patch
def _weakref_referent(self):
    pass
