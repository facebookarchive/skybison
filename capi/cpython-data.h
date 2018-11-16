#ifndef CPYTHON_DATA_H
#define CPYTHON_DATA_H

#include "cpython-func.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Singletons */
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

#ifdef __cplusplus
}
#endif

#endif /* !CPYTHON_DATA_H */
