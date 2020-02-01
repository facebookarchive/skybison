#pragma once

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

// Copy the code, entry, and interpreter info from base to patch.
void copyFunctionEntries(Thread* thread, const Function& base,
                         const Function& patch);

RawObject FUNC(_builtins, _address)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _bool_check)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(_builtins, _bool_guard)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(_builtins, _bound_method)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject FUNC(_builtins, _bytearray_append)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _bytearray_check)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _bytearray_clear)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _bytearray_contains)(Thread* thread, Frame* frame,
                                               word nargs);
RawObject FUNC(_builtins, _bytearray_delitem)(Thread* thread, Frame* frame,
                                              word nargs);
RawObject FUNC(_builtins, _bytearray_delslice)(Thread* thread, Frame* frame,
                                               word nargs);
RawObject FUNC(_builtins, _bytearray_getitem)(Thread* thread, Frame* frame,
                                              word nargs);
RawObject FUNC(_builtins, _bytearray_getslice)(Thread* thread, Frame* frame,
                                               word nargs);
RawObject FUNC(_builtins, _bytearray_guard)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _bytearray_join)(Thread* thread, Frame* frame,
                                           word nargs);
RawObject FUNC(_builtins, _bytearray_len)(Thread* thread, Frame* frame,
                                          word nargs);
RawObject FUNC(_builtins, _bytearray_setitem)(Thread* thread, Frame* frame,
                                              word nargs);
RawObject FUNC(_builtins, _bytearray_setslice)(Thread* thread, Frame* frame,
                                               word nargs);
RawObject FUNC(_builtins, _bytes_check)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_builtins, _bytes_contains)(Thread* thread, Frame* frame,
                                           word nargs);
RawObject FUNC(_builtins, _bytes_decode)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject FUNC(_builtins, _bytes_decode_ascii)(Thread* thread, Frame* frame,
                                               word nargs);
RawObject FUNC(_builtins, _bytes_decode_utf_8)(Thread* thread, Frame* frame,
                                               word nargs);
RawObject FUNC(_builtins, _bytes_from_bytes)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _bytes_from_ints)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _bytes_getitem)(Thread* thread, Frame* frame,
                                          word nargs);
RawObject FUNC(_builtins, _bytes_getslice)(Thread* thread, Frame* frame,
                                           word nargs);
RawObject FUNC(_builtins, _bytes_guard)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_builtins, _bytes_join)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(_builtins, _bytes_len)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _bytes_maketrans)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _bytes_repeat)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject FUNC(_builtins, _bytes_split)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_builtins, _bytes_split_whitespace)(Thread* thread, Frame* frame,
                                                   word nargs);
RawObject FUNC(_builtins, _byteslike_check)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _byteslike_compare_digest)(Thread* thread,
                                                     Frame* frame, word nargs);
RawObject FUNC(_builtins, _byteslike_count)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _byteslike_endswith)(Thread* thread, Frame* frame,
                                               word nargs);
RawObject FUNC(_builtins, _byteslike_find_byteslike)(Thread* thread,
                                                     Frame* frame, word nargs);
RawObject FUNC(_builtins, _byteslike_find_int)(Thread* thread, Frame* frame,
                                               word nargs);
RawObject FUNC(_builtins, _byteslike_guard)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _byteslike_rfind_byteslike)(Thread* thread,
                                                      Frame* frame, word nargs);
RawObject FUNC(_builtins, _byteslike_rfind_int)(Thread* thread, Frame* frame,
                                                word nargs);
RawObject FUNC(_builtins, _byteslike_startswith)(Thread* thread, Frame* frame,
                                                 word nargs);
RawObject FUNC(_builtins, _classmethod)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_builtins, _classmethod_isabstract)(Thread* thread, Frame* frame,
                                                   word nargs);
RawObject FUNC(_builtins, _code_check)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(_builtins, _code_guard)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(_builtins, _code_set_posonlyargcount)(Thread* thread,
                                                     Frame* frame, word nargs);
RawObject FUNC(_builtins, _complex_check)(Thread* thread, Frame* frame,
                                          word nargs);
RawObject FUNC(_builtins, _complex_checkexact)(Thread* thread, Frame* frame,
                                               word nargs);
RawObject FUNC(_builtins, _complex_imag)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject FUNC(_builtins, _complex_new)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_builtins, _complex_new_from_str)(Thread* thread, Frame* frame,
                                                 word nargs);
RawObject FUNC(_builtins, _complex_real)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject FUNC(_builtins, _dict_bucket_insert)(Thread* thread, Frame* frame,
                                               word nargs);
RawObject FUNC(_builtins, _dict_bucket_key)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _dict_bucket_set_value)(Thread* thread, Frame* frame,
                                                  word nargs);
RawObject FUNC(_builtins, _dict_bucket_value)(Thread* thread, Frame* frame,
                                              word nargs);
RawObject FUNC(_builtins, _dict_check)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(_builtins, _dict_check_exact)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _dict_get)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _dict_guard)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(_builtins, _dict_len)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _dict_lookup)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_builtins, _dict_lookup_next)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _dict_update)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_builtins, _dict_popitem)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject FUNC(_builtins, _dict_setitem)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject FUNC(_builtins, _divmod)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _exec)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _float_check)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_builtins, _float_check_exact)(Thread* thread, Frame* frame,
                                              word nargs);
RawObject FUNC(_builtins, _float_divmod)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject FUNC(_builtins, _float_format)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject FUNC(_builtins, _float_guard)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_builtins, _float_new_from_byteslike)(Thread* thread,
                                                     Frame* frame, word nargs);
RawObject FUNC(_builtins, _float_new_from_float)(Thread* thread, Frame* frame,
                                                 word nargs);
RawObject FUNC(_builtins, _float_new_from_str)(Thread* thread, Frame* frame,
                                               word nargs);
RawObject FUNC(_builtins, _float_signbit)(Thread* thread, Frame* frame,
                                          word nargs);
RawObject FUNC(_builtins, _frozenset_check)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _frozenset_guard)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _function_globals)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _function_guard)(Thread* thread, Frame* frame,
                                           word nargs);
RawObject FUNC(_builtins, _gc)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _getframe_function)(Thread* thread, Frame* frame,
                                              word nargs);
RawObject FUNC(_builtins, _getframe_lineno)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _getframe_locals)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _get_member_byte)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _get_member_char)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _get_member_double)(Thread* thread, Frame* frame,
                                              word nargs);
RawObject FUNC(_builtins, _get_member_float)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _get_member_int)(Thread* thread, Frame* frame,
                                           word nargs);
RawObject FUNC(_builtins, _get_member_long)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _get_member_pyobject)(Thread* thread, Frame* frame,
                                                word nargs);
RawObject FUNC(_builtins, _get_member_short)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _get_member_string)(Thread* thread, Frame* frame,
                                              word nargs);
RawObject FUNC(_builtins, _get_member_ubyte)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _get_member_uint)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _get_member_ulong)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _get_member_ushort)(Thread* thread, Frame* frame,
                                              word nargs);
RawObject FUNC(_builtins, _instance_delattr)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _instance_getattr)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _instance_guard)(Thread* thread, Frame* frame,
                                           word nargs);
RawObject FUNC(_builtins, _instance_overflow_dict)(Thread* thread, Frame* frame,
                                                   word nargs);
RawObject FUNC(_builtins, _instance_setattr)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _int_check)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _int_check_exact)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _int_from_bytes)(Thread* thread, Frame* frame,
                                           word nargs);
RawObject FUNC(_builtins, _int_guard)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _int_new_from_bytearray)(Thread* thread, Frame* frame,
                                                   word nargs);
RawObject FUNC(_builtins, _int_new_from_bytes)(Thread* thread, Frame* frame,
                                               word nargs);
RawObject FUNC(_builtins, _int_new_from_int)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _int_new_from_str)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _iter)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _list_check)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(_builtins, _list_check_exact)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _list_delitem)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject FUNC(_builtins, _list_delslice)(Thread* thread, Frame* frame,
                                          word nargs);
RawObject FUNC(_builtins, _list_extend)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_builtins, _list_getitem)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject FUNC(_builtins, _list_getslice)(Thread* thread, Frame* frame,
                                          word nargs);
RawObject FUNC(_builtins, _list_guard)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(_builtins, _list_len)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _list_new)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _list_setitem)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject FUNC(_builtins, _list_setslice)(Thread* thread, Frame* frame,
                                          word nargs);
RawObject FUNC(_builtins, _list_sort)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _list_swap)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _mappingproxy_guard)(Thread* thread, Frame* frame,
                                               word nargs);
RawObject FUNC(_builtins, _mappingproxy_mapping)(Thread* thread, Frame* frame,
                                                 word nargs);
RawObject FUNC(_builtins, _mappingproxy_set_mapping)(Thread* thread,
                                                     Frame* frame, word nargs);
RawObject FUNC(_builtins, _memoryview_check)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _memoryview_guard)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _memoryview_itemsize)(Thread* thread, Frame* frame,
                                                word nargs);
RawObject FUNC(_builtins, _memoryview_nbytes)(Thread* thread, Frame* frame,
                                              word nargs);
RawObject FUNC(_builtins, _memoryview_setitem)(Thread* thread, Frame* frame,
                                               word nargs);
RawObject FUNC(_builtins, _memoryview_setslice)(Thread* thread, Frame* frame,
                                                word nargs);
RawObject FUNC(_builtins, _module_dir)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(_builtins, _module_proxy)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject FUNC(_builtins, _module_proxy_check)(Thread* thread, Frame* frame,
                                               word nargs);
RawObject FUNC(_builtins, _module_proxy_get)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _module_proxy_guard)(Thread* thread, Frame* frame,
                                               word nargs);
RawObject FUNC(_builtins, _module_proxy_keys)(Thread* thread, Frame* frame,
                                              word nargs);
RawObject FUNC(_builtins, _module_proxy_len)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _module_proxy_delitem)(Thread* thread, Frame* frame,
                                                 word nargs);
RawObject FUNC(_builtins, _module_proxy_setitem)(Thread* thread, Frame* frame,
                                                 word nargs);
RawObject FUNC(_builtins, _module_proxy_values)(Thread* thread, Frame* frame,
                                                word nargs);
RawObject FUNC(_builtins, _object_keys)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_builtins, _object_type_getattr)(Thread* thread, Frame* frame,
                                                word nargs);
RawObject FUNC(_builtins, _object_type_hasattr)(Thread* thread, Frame* frame,
                                                word nargs);
RawObject FUNC(_builtins, _os_write)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _patch)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _property)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _property_isabstract)(Thread* thread, Frame* frame,
                                                word nargs);
RawObject FUNC(_builtins, _pyobject_offset)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _range_check)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_builtins, _range_guard)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_builtins, _range_len)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _repr_enter)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(_builtins, _repr_leave)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(_builtins, _seq_index)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _seq_iterable)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject FUNC(_builtins, _seq_set_index)(Thread* thread, Frame* frame,
                                          word nargs);
RawObject FUNC(_builtins, _seq_set_iterable)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _set_check)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _set_guard)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _set_len)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _set_member_double)(Thread* thread, Frame* frame,
                                              word nargs);
RawObject FUNC(_builtins, _set_member_float)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _set_member_integral)(Thread* thread, Frame* frame,
                                                word nargs);
RawObject FUNC(_builtins, _set_member_pyobject)(Thread* thread, Frame* frame,
                                                word nargs);
RawObject FUNC(_builtins, _slice_check)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_builtins, _slice_guard)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_builtins, _slice_start)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_builtins, _slice_start_long)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _slice_step)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(_builtins, _slice_step_long)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _slice_stop)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(_builtins, _slice_stop_long)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _staticmethod_isabstract)(Thread* thread,
                                                    Frame* frame, word nargs);
RawObject FUNC(_builtins, _stop_iteration_ctor)(Thread* thread, Frame* frame,
                                                word nargs);
RawObject FUNC(_builtins, _strarray_clear)(Thread* thread, Frame* frame,
                                           word nargs);
RawObject FUNC(_builtins, _strarray_ctor)(Thread* thread, Frame* frame,
                                          word nargs);
RawObject FUNC(_builtins, _strarray_iadd)(Thread* thread, Frame* frame,
                                          word nargs);
RawObject FUNC(_builtins, _str_check)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _str_check_exact)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _str_compare_digest)(Thread* thread, Frame* frame,
                                               word nargs);
RawObject FUNC(_builtins, _str_count)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _str_encode)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(_builtins, _str_encode_ascii)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _str_endswith)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject FUNC(_builtins, _str_escape_non_ascii)(Thread* thread, Frame* frame,
                                                 word nargs);
RawObject FUNC(_builtins, _str_find)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _str_from_str)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject FUNC(_builtins, _str_getitem)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_builtins, _str_getslice)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject FUNC(_builtins, _str_guard)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _str_ischr)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _str_join)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _str_len)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _str_mod_fast_path)(Thread* thread, Frame* frame,
                                              word nargs);
RawObject FUNC(_builtins, _str_partition)(Thread* thread, Frame* frame,
                                          word nargs);
RawObject FUNC(_builtins, _str_replace)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_builtins, _str_rfind)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _str_rpartition)(Thread* thread, Frame* frame,
                                           word nargs);
RawObject FUNC(_builtins, _str_split)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _str_splitlines)(Thread* thread, Frame* frame,
                                           word nargs);
RawObject FUNC(_builtins, _str_startswith)(Thread* thread, Frame* frame,
                                           word nargs);
RawObject FUNC(_builtins, _tuple_check)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_builtins, _tuple_check_exact)(Thread* thread, Frame* frame,
                                              word nargs);
RawObject FUNC(_builtins, _tuple_getitem)(Thread* thread, Frame* frame,
                                          word nargs);
RawObject FUNC(_builtins, _tuple_getslice)(Thread* thread, Frame* frame,
                                           word nargs);
RawObject FUNC(_builtins, _tuple_guard)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_builtins, _tuple_len)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _tuple_new)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _type)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _type_abstractmethods_del)(Thread* thread,
                                                     Frame* frame, word nargs);
RawObject FUNC(_builtins, _type_abstractmethods_get)(Thread* thread,
                                                     Frame* frame, word nargs);
RawObject FUNC(_builtins, _type_abstractmethods_set)(Thread* thread,
                                                     Frame* frame, word nargs);
RawObject FUNC(_builtins, _type_bases_del)(Thread* thread, Frame* frame,
                                           word nargs);
RawObject FUNC(_builtins, _type_bases_get)(Thread* thread, Frame* frame,
                                           word nargs);
RawObject FUNC(_builtins, _type_bases_set)(Thread* thread, Frame* frame,
                                           word nargs);
RawObject FUNC(_builtins, _type_check)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(_builtins, _type_check_exact)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _type_dunder_call)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _type_guard)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(_builtins, _type_init)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _type_issubclass)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _type_new)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _type_proxy)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(_builtins, _type_proxy_check)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _type_proxy_get)(Thread* thread, Frame* frame,
                                           word nargs);
RawObject FUNC(_builtins, _type_proxy_guard)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _type_proxy_keys)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject FUNC(_builtins, _type_proxy_len)(Thread* thread, Frame* frame,
                                           word nargs);
RawObject FUNC(_builtins, _type_proxy_values)(Thread* thread, Frame* frame,
                                              word nargs);
RawObject FUNC(_builtins, _type_subclass_guard)(Thread* thread, Frame* frame,
                                                word nargs);
RawObject FUNC(_builtins, _unimplemented)(Thread* thread, Frame* frame,
                                          word nargs);
RawObject FUNC(_builtins, _warn)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_builtins, _weakref_callback)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject FUNC(_builtins, _weakref_check)(Thread* thread, Frame* frame,
                                          word nargs);
RawObject FUNC(_builtins, _weakref_guard)(Thread* thread, Frame* frame,
                                          word nargs);
RawObject FUNC(_builtins, _weakref_referent)(Thread* thread, Frame* frame,
                                             word nargs);

class UnderBuiltinsModule {
 public:
  static void initialize(Thread* thread, const Module& module);

  static const BuiltinFunction kBuiltinFunctions[];

 private:
  static const SymbolId kIntrinsicIds[];
};

}  // namespace py
