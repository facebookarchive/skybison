#ifndef CPYTHON_DATA_H
#define CPYTHON_DATA_H

#include "cpython-func.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Singletons */
#define Py_Ellipsis PyEllipsis_Ptr()
#define Py_False PyFalse_Ptr()
#define Py_None PyNone_Ptr()
#define Py_True PyTrue_Ptr()

#define PyExc_BaseException PyExc_BaseException_Ptr()
#define PyExc_Exception PyExc_Exception_Ptr()
#define PyExc_StopAsyncIteration PyExc_StopAsyncIteration_Ptr()
#define PyExc_StopIteration PyExc_StopIteration_Ptr()
#define PyExc_GeneratorExit PyExc_GeneratorExit_Ptr()
#define PyExc_ArithmeticError PyExc_ArithmeticError_Ptr()
#define PyExc_LookupError PyExc_LookupError_Ptr()

#define PyExc_AssertionError PyExc_AssertionError_Ptr()
#define PyExc_AttributeError PyExc_AttributeError_Ptr()
#define PyExc_BufferError PyExc_BufferError_Ptr()
#define PyExc_EOFError PyExc_EOFError_Ptr()
#define PyExc_FloatingPointError PyExc_FloatingPointError_Ptr()
#define PyExc_OSError PyExc_OSError_Ptr()
#define PyExc_ImportError PyExc_ImportError_Ptr()
#define PyExc_ModuleNotFoundError PyExc_ModuleNotFoundError_Ptr()
#define PyExc_IndexError PyExc_IndexError_Ptr()
#define PyExc_KeyError PyExc_KeyError_Ptr()
#define PyExc_KeyboardInterrupt PyExc_KeyboardInterrupt_Ptr()
#define PyExc_MemoryError PyExc_MemoryError_Ptr()
#define PyExc_NameError PyExc_NameError_Ptr()
#define PyExc_OverflowError PyExc_OverflowError_Ptr()
#define PyExc_RuntimeError PyExc_RuntimeError_Ptr()
#define PyExc_RecursionError PyExc_RecursionError_Ptr()
#define PyExc_NotImplementedError PyExc_NotImplementedError_Ptr()
#define PyExc_SyntaxError PyExc_SyntaxError_Ptr()
#define PyExc_IndentationError PyExc_IndentationError_Ptr()
#define PyExc_TabError PyExc_TabError_Ptr()
#define PyExc_ReferenceError PyExc_ReferenceError_Ptr()
#define PyExc_SystemError PyExc_SystemError_Ptr()
#define PyExc_SystemExit PyExc_SystemExit_Ptr()
#define PyExc_TypeError PyExc_TypeError_Ptr()
#define PyExc_UnboundLocalError PyExc_UnboundLocalError_Ptr()
#define PyExc_UnicodeError PyExc_UnicodeError_Ptr()
#define PyExc_UnicodeEncodeError PyExc_UnicodeEncodeError_Ptr()
#define PyExc_UnicodeDecodeError PyExc_UnicodeDecodeError_Ptr()
#define PyExc_UnicodeTranslateError PyExc_UnicodeTranslateError_Ptr()
#define PyExc_ValueError PyExc_ValueError_Ptr()
#define PyExc_ZeroDivisionError PyExc_ZeroDivisionError_Ptr()

#define PyExc_BlockingIOError PyExc_BlockingIOError_Ptr()
#define PyExc_BrokenPipeError PyExc_BrokenPipeError_Ptr()
#define PyExc_ChildProcessError PyExc_ChildProcessError_Ptr()
#define PyExc_ConnectionError PyExc_ConnectionError_Ptr()
#define PyExc_ConnectionAbortedError PyExc_ConnectionAbortedError_Ptr()
#define PyExc_ConnectionRefusedError PyExc_ConnectionRefusedError_Ptr()
#define PyExc_ConnectionResetError PyExc_ConnectionResetError_Ptr()
#define PyExc_FileExistsError PyExc_FileExistsError_Ptr()
#define PyExc_FileNotFoundError PyExc_FileNotFoundError_Ptr()
#define PyExc_InterruptedError PyExc_InterruptedError_Ptr()
#define PyExc_IsADirectoryError PyExc_IsADirectoryError_Ptr()
#define PyExc_NotADirectoryError PyExc_NotADirectoryError_Ptr()
#define PyExc_PermissionError PyExc_PermissionError_Ptr()
#define PyExc_ProcessLookupError PyExc_ProcessLookupError_Ptr()
#define PyExc_TimeoutError PyExc_TimeoutError_Ptr()

#define PyExc_EnvironmentError PyExc_OSError_Ptr()
#define PyExc_IOError PyExc_OSError_Ptr()

#define PyExc_Warning PyExc_Warning_Ptr()
#define PyExc_UserWarning PyExc_UserWarning_Ptr()
#define PyExc_DeprecationWarning PyExc_DeprecationWarning_Ptr()
#define PyExc_PendingDeprecationWarning PyExc_PendingDeprecationWarning_Ptr()
#define PyExc_SyntaxWarning PyExc_SyntaxWarning_Ptr()
#define PyExc_RuntimeWarning PyExc_RuntimeWarning_Ptr()
#define PyExc_FutureWarning PyExc_FutureWarning_Ptr()
#define PyExc_ImportWarning PyExc_ImportWarning_Ptr()
#define PyExc_UnicodeWarning PyExc_UnicodeWarning_Ptr()
#define PyExc_BytesWarning PyExc_BytesWarning_Ptr()
#define PyExc_ResourceWarning PyExc_ResourceWarning_Ptr()

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

extern const unsigned int _Py_ctype_table[256];
extern const unsigned char _Py_ctype_tolower[256];
extern const unsigned char _Py_ctype_toupper[256];

#define Py_CLEANUP_SUPPORTED 0x20000

/* Define to printf format modifier for Py_ssize_t */
#define PY_FORMAT_SIZE_T "z"

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

/* Flag bits for printing: */
#define Py_PRINT_RAW 1 /* No string quotes etc. */

extern char* PyStructSequence_UnnamedField;

extern const char* Py_FileSystemDefaultEncodeErrors;

#ifdef __cplusplus
}
#endif

#endif /* !CPYTHON_DATA_H */
