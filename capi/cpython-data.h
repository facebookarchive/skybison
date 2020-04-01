#ifndef CPYTHON_DATA_H
#define CPYTHON_DATA_H

#include "cpython-func.h"

#include "pyconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PyAPI_DATA(RTYPE) extern RTYPE
#define PyDoc_VAR(name) static char name[]
#define PyDoc_STRVAR(name, str) PyDoc_VAR(name) = PyDoc_STR(str)
#define PyDoc_STR(str) str

#define PY_PARSER_REQUIRES_FUTURE_KEYWORD
#define Py_USING_UNICODE
#define Py_UNICODE_WIDE
#define HAVE_PY_SET_53BIT_PRECISION 0

#if __SIZEOF_WCHAR_T__ < 4
#error sizeof(wchar_t) < 4 not supported
#endif

#define _Py_XSTRINGIFY(x) #x
#define Py_STRINGIFY(x) _Py_XSTRINGIFY(x)
#define Py_FORCE_EXPANSION(X) X

/* Singletons */
#define PyAsyncGen_Type (*PyAsyncGen_Type_Ptr())
#define PyBaseObject_Type (*PyBaseObject_Type_Ptr())
#define PyBool_Type (*PyBool_Type_Ptr())
#define PyByteArrayIter_Type (*PyByteArrayIter_Type_Ptr())
#define PyByteArray_Type (*PyByteArray_Type_Ptr())
#define PyBytesIter_Type (*PyBytesIter_Type_Ptr())
#define PyBytes_Type (*PyBytes_Type_Ptr())
#define PyClassMethod_Type (*PyClassMethod_Type_Ptr())
#define PyCode_Type (*PyCode_Type_Ptr())
#define PyComplex_Type (*PyComplex_Type_Ptr())
#define PyCoro_Type (*PyCoro_Type_Ptr())
#define PyDictItems_Type (*PyDictItems_Type_Ptr())
#define PyDictIterItem_Type (*PyDictIterItem_Type_Ptr())
#define PyDictIterKey_Type (*PyDictIterKey_Type_Ptr())
#define PyDictIterValue_Type (*PyDictIterValue_Type_Ptr())
#define PyDictKeys_Type (*PyDictKeys_Type_Ptr())
#define PyDictProxy_Type (*PyDictProxy_Type_Ptr())
#define PyDictValues_Type (*PyDictValues_Type_Ptr())
#define PyDict_Type (*PyDict_Type_Ptr())
#define PyEllipsis_Type (*PyEllipsis_Type_Ptr())
#define PyEnum_Type (*PyEnum_Type_Ptr())
#define PyExc_ArithmeticError PyExc_ArithmeticError_Ptr()
#define PyExc_AssertionError PyExc_AssertionError_Ptr()
#define PyExc_AttributeError PyExc_AttributeError_Ptr()
#define PyExc_BaseException PyExc_BaseException_Ptr()
#define PyExc_BlockingIOError PyExc_BlockingIOError_Ptr()
#define PyExc_BrokenPipeError PyExc_BrokenPipeError_Ptr()
#define PyExc_BufferError PyExc_BufferError_Ptr()
#define PyExc_BytesWarning PyExc_BytesWarning_Ptr()
#define PyExc_ChildProcessError PyExc_ChildProcessError_Ptr()
#define PyExc_ConnectionAbortedError PyExc_ConnectionAbortedError_Ptr()
#define PyExc_ConnectionError PyExc_ConnectionError_Ptr()
#define PyExc_ConnectionRefusedError PyExc_ConnectionRefusedError_Ptr()
#define PyExc_ConnectionResetError PyExc_ConnectionResetError_Ptr()
#define PyExc_DeprecationWarning PyExc_DeprecationWarning_Ptr()
#define PyExc_EOFError PyExc_EOFError_Ptr()
#define PyExc_EnvironmentError PyExc_OSError_Ptr()
#define PyExc_Exception PyExc_Exception_Ptr()
#define PyExc_FileExistsError PyExc_FileExistsError_Ptr()
#define PyExc_FileNotFoundError PyExc_FileNotFoundError_Ptr()
#define PyExc_FloatingPointError PyExc_FloatingPointError_Ptr()
#define PyExc_FutureWarning PyExc_FutureWarning_Ptr()
#define PyExc_GeneratorExit PyExc_GeneratorExit_Ptr()
#define PyExc_IOError PyExc_OSError_Ptr()
#define PyExc_ImportError PyExc_ImportError_Ptr()
#define PyExc_ImportWarning PyExc_ImportWarning_Ptr()
#define PyExc_IndentationError PyExc_IndentationError_Ptr()
#define PyExc_IndexError PyExc_IndexError_Ptr()
#define PyExc_InterruptedError PyExc_InterruptedError_Ptr()
#define PyExc_IsADirectoryError PyExc_IsADirectoryError_Ptr()
#define PyExc_KeyError PyExc_KeyError_Ptr()
#define PyExc_KeyboardInterrupt PyExc_KeyboardInterrupt_Ptr()
#define PyExc_LookupError PyExc_LookupError_Ptr()
#define PyExc_MemoryError PyExc_MemoryError_Ptr()
#define PyExc_ModuleNotFoundError PyExc_ModuleNotFoundError_Ptr()
#define PyExc_NameError PyExc_NameError_Ptr()
#define PyExc_NotADirectoryError PyExc_NotADirectoryError_Ptr()
#define PyExc_NotImplementedError PyExc_NotImplementedError_Ptr()
#define PyExc_OSError PyExc_OSError_Ptr()
#define PyExc_OverflowError PyExc_OverflowError_Ptr()
#define PyExc_PendingDeprecationWarning PyExc_PendingDeprecationWarning_Ptr()
#define PyExc_PermissionError PyExc_PermissionError_Ptr()
#define PyExc_ProcessLookupError PyExc_ProcessLookupError_Ptr()
#define PyExc_RecursionError PyExc_RecursionError_Ptr()
#define PyExc_ReferenceError PyExc_ReferenceError_Ptr()
#define PyExc_ResourceWarning PyExc_ResourceWarning_Ptr()
#define PyExc_RuntimeError PyExc_RuntimeError_Ptr()
#define PyExc_RuntimeWarning PyExc_RuntimeWarning_Ptr()
#define PyExc_StopAsyncIteration PyExc_StopAsyncIteration_Ptr()
#define PyExc_StopIteration PyExc_StopIteration_Ptr()
#define PyExc_SyntaxError PyExc_SyntaxError_Ptr()
#define PyExc_SyntaxWarning PyExc_SyntaxWarning_Ptr()
#define PyExc_SystemError PyExc_SystemError_Ptr()
#define PyExc_SystemExit PyExc_SystemExit_Ptr()
#define PyExc_TabError PyExc_TabError_Ptr()
#define PyExc_TimeoutError PyExc_TimeoutError_Ptr()
#define PyExc_TypeError PyExc_TypeError_Ptr()
#define PyExc_UnboundLocalError PyExc_UnboundLocalError_Ptr()
#define PyExc_UnicodeDecodeError PyExc_UnicodeDecodeError_Ptr()
#define PyExc_UnicodeEncodeError PyExc_UnicodeEncodeError_Ptr()
#define PyExc_UnicodeError PyExc_UnicodeError_Ptr()
#define PyExc_UnicodeTranslateError PyExc_UnicodeTranslateError_Ptr()
#define PyExc_UnicodeWarning PyExc_UnicodeWarning_Ptr()
#define PyExc_UserWarning PyExc_UserWarning_Ptr()
#define PyExc_ValueError PyExc_ValueError_Ptr()
#define PyExc_Warning PyExc_Warning_Ptr()
#define PyExc_ZeroDivisionError PyExc_ZeroDivisionError_Ptr()
#define PyFloat_Type (*PyFloat_Type_Ptr())
#define PyFrozenSet_Type (*PyFrozenSet_Type_Ptr())
#define PyFunction_Type (*PyFunction_Type_Ptr())
#define PyGen_Type (*PyGen_Type_Ptr())
#define PyListIter_Type (*PyListIter_Type_Ptr())
#define PyList_Type (*PyList_Type_Ptr())
#define PyLongRangeIter_Type (*PyLongRangeIter_Type_Ptr())
#define PyLong_Type (*PyLong_Type_Ptr())
#define PyMemoryView_Type (*PyMemoryView_Type_Ptr())
#define PyMethod_Type (*PyMethod_Type_Ptr())
#define PyModule_Type (*PyModule_Type_Ptr())
#define PyProperty_Type (*PyProperty_Type_Ptr())
#define PyRangeIter_Type (*PyRangeIter_Type_Ptr())
#define PyRange_Type (*PyRange_Type_Ptr())
#define PySeqIter_Type (*PySeqIter_Type_Ptr())
#define PySetIter_Type (*PySetIter_Type_Ptr())
#define PySet_Type (*PySet_Type_Ptr())
#define PySlice_Type (*PySlice_Type_Ptr())
#define PyStaticMethod_Type (*PyStaticMethod_Type_Ptr())
#define PySuper_Type (*PySuper_Type_Ptr())
#define PyTupleIter_Type (*PyTupleIter_Type_Ptr())
#define PyTuple_Type (*PyTuple_Type_Ptr())
#define PyType_Type (*PyType_Type_Ptr())
#define PyUnicodeIter_Type (*PyUnicodeIter_Type_Ptr())
#define PyUnicode_Type (*PyUnicode_Type_Ptr())
#define Py_Ellipsis PyEllipsis_Ptr()
#define Py_False PyFalse_Ptr()
#define Py_None PyNone_Ptr()
#define Py_NotImplemented PyNotImplemented_Ptr()
#define Py_True PyTrue_Ptr()
#define _PyLong_One _PyLong_One_Ptr()
#define _PyLong_Zero _PyLong_Zero_Ptr()
#define _PyNone_Type (*_PyNone_Type_Ptr())
#define _PyNotImplemented_Type (*_PyNotImplemented_Type_Ptr())

#define PYTHON_ABI_STRING "3"
#define PYTHON_ABI_VERSION 3
#define PYTHON_API_STRING "1013"
#define PYTHON_API_VERSION 1013

#define PY_MAJOR_VERSION 3
#define PY_MICRO_VERSION 8
#define PY_MINOR_VERSION 6
#define PY_RELEASE_LEVEL PY_RELEASE_LEVEL_FINAL
#define PY_RELEASE_LEVEL_ALPHA 0xA
#define PY_RELEASE_LEVEL_BETA 0xB
#define PY_RELEASE_LEVEL_FINAL 0xF
#define PY_RELEASE_LEVEL_GAMMA 0xC
#define PY_RELEASE_SERIAL 0
#define PY_VERSION                                                             \
  Py_STRINGIFY(PY_MAJOR_VERSION) "." Py_STRINGIFY(                             \
      PY_MINOR_VERSION) "." Py_STRINGIFY(PY_MICRO_VERSION) "+"
#define PY_VERSION_HEX                                                         \
  ((PY_MAJOR_VERSION << 24) | (PY_MINOR_VERSION << 16) |                       \
   (PY_MICRO_VERSION << 8) | (PY_RELEASE_LEVEL << 4) |                         \
   (PY_RELEASE_SERIAL << 0))

#define Py_MATH_PIl 3.1415926535897932384626433832795029L
#define Py_MATH_PI 3.14159265358979323846
#define Py_MATH_El 2.7182818284590452353602874713526625L
#define Py_MATH_E 2.7182818284590452354
#define Py_MATH_TAU 6.2831853071795864769252867665590057683943L

extern int Py_BytesWarningFlag;
extern int Py_DebugFlag;
extern int Py_DontWriteBytecodeFlag;
extern int Py_FrozenFlag;
extern int Py_HashRandomizationFlag;
extern int Py_IgnoreEnvironmentFlag;
extern int Py_InspectFlag;
extern int Py_InteractiveFlag;
extern int Py_IsolatedFlag;
extern int Py_NoSiteFlag;
extern int Py_NoUserSiteDirectory;
extern int Py_OptimizeFlag;
extern int Py_QuietFlag;
extern int Py_UnbufferedStdioFlag;
extern int Py_UseClassExceptionsFlag;
extern int Py_VerboseFlag;

#define PY_CTF_LOWER 0x01
#define PY_CTF_UPPER 0x02
#define PY_CTF_ALPHA (PY_CTF_LOWER | PY_CTF_UPPER)
#define PY_CTF_DIGIT 0x04
#define PY_CTF_ALNUM (PY_CTF_ALPHA | PY_CTF_DIGIT)
#define PY_CTF_SPACE 0x08
#define PY_CTF_XDIGIT 0x10

extern const unsigned char _Py_ascii_whitespace[128];
extern const unsigned int _Py_ctype_table[256];
extern const unsigned char _Py_ctype_tolower[256];
extern const unsigned char _Py_ctype_toupper[256];
extern const unsigned char _PyLong_DigitValue[256];

#define Py_CLEANUP_SUPPORTED 0x20000

/* Flags for getting buffers */
#define PyBUF_SIMPLE 0
#define PyBUF_WRITABLE 0x0001
#define PyBUF_FORMAT 0x0004
#define PyBUF_ND 0x0008
#define PyBUF_STRIDES (0x0010 | PyBUF_ND)
#define PyBUF_C_CONTIGUOUS (0x0020 | PyBUF_STRIDES)
#define PyBUF_F_CONTIGUOUS (0x0040 | PyBUF_STRIDES)
#define PyBUF_ANY_CONTIGUOUS (0x0080 | PyBUF_STRIDES)
#define PyBUF_INDIRECT (0x0100 | PyBUF_STRIDES)
#define PyBUF_CONTIG (PyBUF_ND | PyBUF_WRITABLE)
#define PyBUF_CONTIG_RO (PyBUF_ND)
#define PyBUF_STRIDED (PyBUF_STRIDES | PyBUF_WRITABLE)
#define PyBUF_STRIDED_RO (PyBUF_STRIDES)
#define PyBUF_RECORDS (PyBUF_STRIDES | PyBUF_WRITABLE | PyBUF_FORMAT)
#define PyBUF_RECORDS_RO (PyBUF_STRIDES | PyBUF_FORMAT)
#define PyBUF_FULL (PyBUF_INDIRECT | PyBUF_WRITABLE | PyBUF_FORMAT)
#define PyBUF_FULL_RO (PyBUF_INDIRECT | PyBUF_FORMAT)

#define PyBUF_READ 0x100
#define PyBUF_WRITE 0x200

/* Type Slots */
#define Py_mp_ass_subscript 3
#define Py_mp_length 4
#define Py_mp_subscript 5
#define Py_nb_absolute 6
#define Py_nb_add 7
#define Py_nb_and 8
#define Py_nb_bool 9
#define Py_nb_divmod 10
#define Py_nb_float 11
#define Py_nb_floor_divide 12
#define Py_nb_index 13
#define Py_nb_inplace_add 14
#define Py_nb_inplace_and 15
#define Py_nb_inplace_floor_divide 16
#define Py_nb_inplace_lshift 17
#define Py_nb_inplace_multiply 18
#define Py_nb_inplace_or 19
#define Py_nb_inplace_power 20
#define Py_nb_inplace_remainder 21
#define Py_nb_inplace_rshift 22
#define Py_nb_inplace_subtract 23
#define Py_nb_inplace_true_divide 24
#define Py_nb_inplace_xor 25
#define Py_nb_int 26
#define Py_nb_invert 27
#define Py_nb_lshift 28
#define Py_nb_multiply 29
#define Py_nb_negative 30
#define Py_nb_or 31
#define Py_nb_positive 32
#define Py_nb_power 33
#define Py_nb_remainder 34
#define Py_nb_rshift 35
#define Py_nb_subtract 36
#define Py_nb_true_divide 37
#define Py_nb_xor 38
#define Py_sq_ass_item 39
#define Py_sq_concat 40
#define Py_sq_contains 41
#define Py_sq_inplace_concat 42
#define Py_sq_inplace_repeat 43
#define Py_sq_item 44
#define Py_sq_length 45
#define Py_sq_repeat 46
#define Py_tp_alloc 47
#define Py_tp_base 48
#define Py_tp_bases 49
#define Py_tp_call 50
#define Py_tp_clear 51
#define Py_tp_dealloc 52
#define Py_tp_del 53
#define Py_tp_descr_get 54
#define Py_tp_descr_set 55
#define Py_tp_doc 56
#define Py_tp_getattr 57
#define Py_tp_getattro 58
#define Py_tp_hash 59
#define Py_tp_init 60
#define Py_tp_is_gc 61
#define Py_tp_iter 62
#define Py_tp_iternext 63
#define Py_tp_methods 64
#define Py_tp_new 65
#define Py_tp_repr 66
#define Py_tp_richcompare 67
#define Py_tp_setattr 68
#define Py_tp_setattro 69
#define Py_tp_str 70
#define Py_tp_traverse 71
#define Py_tp_members 72
#define Py_tp_getset 73
#define Py_tp_free 74
#define Py_nb_matrix_multiply 75
#define Py_nb_inplace_matrix_multiply 76
#define Py_am_await 77
#define Py_am_aiter 78
#define Py_am_anext 79
#define Py_tp_finalize 80

/* Method Types */
#define METH_VARARGS 0x0001
#define METH_KEYWORDS 0x0002
#define METH_NOARGS 0x0004
#define METH_O 0x0008
#define METH_CLASS 0x0010
#define METH_STATIC 0x0020
#define METH_COEXIST 0x0040
#define METH_FASTCALL 0x0080

/* Rich comparison opcodes */
#define Py_LT 0
#define Py_LE 1
#define Py_EQ 2
#define Py_NE 3
#define Py_GT 4
#define Py_GE 5

/* Masks and values used by FORMAT_VALUE opcode. */
#define FVC_MASK 0x3
#define FVC_NONE 0x0
#define FVC_STR 0x1
#define FVC_REPR 0x2
#define FVC_ASCII 0x3
#define FVS_MASK 0x4
#define FVS_HAVE_SPEC 0x4

/* Flag bits for printing: */
#define Py_PRINT_RAW 1 /* No string quotes etc. */

#define Py_DTSF_SIGN 0x01
#define Py_DTSF_ADD_DOT_0 0x02
#define Py_DTSF_ALT 0x04

#define Py_DTST_FINITE 0
#define Py_DTST_INFINITE 1
#define Py_DTST_NAN 2

extern char* PyStructSequence_UnnamedField;

extern const char* Py_FileSystemDefaultEncodeErrors;
extern const char* Py_hexdigits;

/* C struct member types */
#define T_SHORT 0
#define T_INT 1
#define T_LONG 2
#define T_FLOAT 3
#define T_DOUBLE 4
#define T_STRING 5
#define T_OBJECT 6
#define T_CHAR 7
#define T_BYTE 8
#define T_UBYTE 9
#define T_USHORT 10
#define T_UINT 11
#define T_ULONG 12
#define T_STRING_INPLACE 13
#define T_BOOL 14
#define T_OBJECT_EX 16
#define T_LONGLONG 17
#define T_ULONGLONG 18
#define T_PYSSIZET 19
#define T_NONE 20

/* C struct member flags */
#define READONLY 1

/* Type flags (tp_flags) */
#define Py_TPFLAGS_HAVE_FINALIZE (1UL << 0)
#define Py_TPFLAGS_HEAPTYPE (1UL << 9)
#define Py_TPFLAGS_BASETYPE (1UL << 10)
#define Py_TPFLAGS_READY (1UL << 12)
#define Py_TPFLAGS_READYING (1UL << 13)
#define Py_TPFLAGS_HAVE_GC (1UL << 14)
#define Py_TPFLAGS_HAVE_STACKLESS_EXTENSION 0
#define Py_TPFLAGS_HAVE_VERSION_TAG (1UL << 18)
#define Py_TPFLAGS_VALID_VERSION_TAG (1UL << 19)
#define Py_TPFLAGS_IS_ABSTRACT (1UL << 20)
#define Py_TPFLAGS_LONG_SUBCLASS (1UL << 24)
#define Py_TPFLAGS_LIST_SUBCLASS (1UL << 25)
#define Py_TPFLAGS_TUPLE_SUBCLASS (1UL << 26)
#define Py_TPFLAGS_BYTES_SUBCLASS (1UL << 27)
#define Py_TPFLAGS_UNICODE_SUBCLASS (1UL << 28)
#define Py_TPFLAGS_DICT_SUBCLASS (1UL << 29)
#define Py_TPFLAGS_BASE_EXC_SUBCLASS (1UL << 30)
#define Py_TPFLAGS_TYPE_SUBCLASS (1UL << 31)

#define Py_TPFLAGS_DEFAULT                                                     \
  (Py_TPFLAGS_HAVE_STACKLESS_EXTENSION | Py_TPFLAGS_HAVE_VERSION_TAG)

/* Masks for PyCodeObject co_flags */
#define CO_OPTIMIZED 0x0001
#define CO_NEWLOCALS 0x0002
#define CO_VARARGS 0x0004
#define CO_VARKEYWORDS 0x0008
#define CO_NESTED 0x0010
#define CO_GENERATOR 0x0020
#define CO_NOFREE 0x0040
#define CO_COROUTINE 0x0080
#define CO_ITERABLE_COROUTINE 0x0100
#define CO_ASYNC_GENERATOR 0x0200

#define CO_FUTURE_DIVISION 0x2000
#define CO_FUTURE_ABSOLUTE_IMPORT 0x4000
#define CO_FUTURE_WITH_STATEMENT 0x8000
#define CO_FUTURE_PRINT_FUNCTION 0x10000
#define CO_FUTURE_UNICODE_LITERALS 0x20000
#define CO_FUTURE_BARRY_AS_BDFL 0x40000
#define CO_FUTURE_GENERATOR_STOP 0x80000
#define CO_FUTURE_ANNOTATIONS 0x100000

#define CO_CELL_NOT_AN_ARG 255

#define CO_MAXBLOCKS 20

#define FUTURE_NESTED_SCOPES "nested_scopes"
#define FUTURE_GENERATORS "generators"
#define FUTURE_DIVISION "division"
#define FUTURE_ABSOLUTE_IMPORT "absolute_import"
#define FUTURE_WITH_STATEMENT "with_statement"
#define FUTURE_PRINT_FUNCTION "print_function"
#define FUTURE_UNICODE_LITERALS "unicode_literals"
#define FUTURE_BARRY_AS_BDFL "barry_as_FLUFL"
#define FUTURE_GENERATOR_STOP "generator_stop"
#define FUTURE_ANNOTATIONS "annotations"

#define PY_INVALID_STACK_EFFECT INT_MAX

/* Compiler Flags */
#define Py_single_input 256
#define Py_file_input 257
#define Py_eval_input 258

#define E_EOF 11

#define PyCF_MASK                                                              \
  (CO_FUTURE_DIVISION | CO_FUTURE_ABSOLUTE_IMPORT | CO_FUTURE_WITH_STATEMENT | \
   CO_FUTURE_PRINT_FUNCTION | CO_FUTURE_UNICODE_LITERALS |                     \
   CO_FUTURE_BARRY_AS_BDFL | CO_FUTURE_GENERATOR_STOP | CO_FUTURE_ANNOTATIONS)
#define PyCF_MASK_OBSOLETE (CO_NESTED)
#define PyCF_SOURCE_IS_UTF8 0x0100
#define PyCF_DONT_IMPLY_DEDENT 0x0200
#define PyCF_ONLY_AST 0x0400
#define PyCF_IGNORE_COOKIE 0x0800

#define _PyHASH_MULTIPLIER 1000003UL
#define _PyHASH_BITS (sizeof(void*) < 8 ? 31 : 61)
#define _PyHASH_MODULUS (((size_t)1 << _PyHASH_BITS) - 1)
#define _PyHASH_INF 314159
#define _PyHASH_NAN 0
#define _PyHASH_IMAG _PyHASH_MULTIPLIER

#define Py_HASH_EXTERNAL 0
#define Py_HASH_SIPHASH24 1
#define Py_HASH_FNV 2

#define Py_UNICODE_REPLACEMENT_CHARACTER ((Py_UCS4)0xFFFD)

#define WAIT_LOCK 1
#define NOWAIT_LOCK 0

#ifdef __cplusplus
}
#endif

#endif /* !CPYTHON_DATA_H */
