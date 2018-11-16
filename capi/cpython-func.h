#ifndef CPYTHON_FUNC_H
#define CPYTHON_FUNC_H

#include "cpython-types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PyAPI_FUNC(RTYPE) RTYPE

/* Singletons */
PyAPI_FUNC(PyObject *) PyFalse_Ptr();
PyAPI_FUNC(PyObject *) PyNone_Ptr();
PyAPI_FUNC(PyObject *) PyTrue_Ptr();

PyAPI_FUNC(PyObject *) PyExc_BaseException_Ptr();
PyAPI_FUNC(PyObject *) PyExc_Exception_Ptr();
PyAPI_FUNC(PyObject *) PyExc_StopAsyncIteration_Ptr();
PyAPI_FUNC(PyObject *) PyExc_StopIteration_Ptr();
PyAPI_FUNC(PyObject *) PyExc_GeneratorExit_Ptr();
PyAPI_FUNC(PyObject *) PyExc_ArithmeticError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_LookupError_Ptr();

PyAPI_FUNC(PyObject *) PyExc_AssertionError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_AttributeError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_BufferError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_EOFError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_FloatingPointError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_OSError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_ImportError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_ModuleNotFoundError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_IndexError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_KeyError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_KeyboardInterrupt_Ptr();
PyAPI_FUNC(PyObject *) PyExc_MemoryError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_NameError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_OverflowError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_RuntimeError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_RecursionError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_NotImplementedError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_SyntaxError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_IndentationError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_TabError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_ReferenceError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_SystemError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_SystemExit_Ptr();
PyAPI_FUNC(PyObject *) PyExc_TypeError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_UnboundLocalError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_UnicodeError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_UnicodeEncodeError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_UnicodeDecodeError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_UnicodeTranslateError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_ValueError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_ZeroDivisionError_Ptr();

PyAPI_FUNC(PyObject *) PyExc_BlockingIOError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_BrokenPipeError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_ChildProcessError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_ConnectionError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_ConnectionAbortedError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_ConnectionRefusedError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_ConnectionResetError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_FileExistsError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_FileNotFoundError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_InterruptedError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_IsADirectoryError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_NotADirectoryError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_PermissionError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_ProcessLookupError_Ptr();
PyAPI_FUNC(PyObject *) PyExc_TimeoutError_Ptr();

PyAPI_FUNC(PyObject *) PyExc_Warning_Ptr();
PyAPI_FUNC(PyObject *) PyExc_UserWarning_Ptr();
PyAPI_FUNC(PyObject *) PyExc_DeprecationWarning_Ptr();
PyAPI_FUNC(PyObject *) PyExc_PendingDeprecationWarning_Ptr();
PyAPI_FUNC(PyObject *) PyExc_SyntaxWarning_Ptr();
PyAPI_FUNC(PyObject *) PyExc_RuntimeWarning_Ptr();
PyAPI_FUNC(PyObject *) PyExc_FutureWarning_Ptr();
PyAPI_FUNC(PyObject *) PyExc_ImportWarning_Ptr();
PyAPI_FUNC(PyObject *) PyExc_UnicodeWarning_Ptr();
PyAPI_FUNC(PyObject *) PyExc_BytesWarning_Ptr();
PyAPI_FUNC(PyObject *) PyExc_ResourceWarning_Ptr();

/* Functions */
PyAPI_FUNC(PyObject *) PyBool_FromLong(long);
PyAPI_FUNC(int) PyDict_SetItem(PyObject *, PyObject *, PyObject *);
PyAPI_FUNC(int) PyDict_SetItemString(PyObject *, const char *, PyObject *);
PyAPI_FUNC(PyObject *) PyDict_New();
PyAPI_FUNC(PyObject *) PyDict_GetItem(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyFloat_FromDouble(double);
PyAPI_FUNC(PyObject *) PyList_New(Py_ssize_t);
PyAPI_FUNC(PyObject *) PyModule_Create2(struct PyModuleDef *, int);
PyAPI_FUNC(PyModuleDef *) PyModule_GetDef(PyObject *);
PyAPI_FUNC(PyObject *) PyModule_GetDict(PyObject *);
PyAPI_FUNC(PyObject *) PyLong_FromLong(long);
PyAPI_FUNC(long) PyLong_AsLong(PyObject *);
PyAPI_FUNC(void) _Py_Dealloc_Func(PyObject *);
PyAPI_FUNC(PyObject *) PyTuple_New(Py_ssize_t);
PyAPI_FUNC(Py_ssize_t) PyTuple_Size(PyObject *);
PyAPI_FUNC(int) PyTuple_SetItem(PyObject *, Py_ssize_t, PyObject *);
PyAPI_FUNC(PyObject *) PyTuple_GetItem(PyObject *, Py_ssize_t);
PyAPI_FUNC(PyObject *) PyTuple_Pack(Py_ssize_t, ...);
PyAPI_FUNC(PyObject *) PyUnicode_FromString(const char *);
PyAPI_FUNC(char *) PyUnicode_AsUTF8AndSize(PyObject *, Py_ssize_t *);
PyAPI_FUNC(int) PyType_Ready(PyTypeObject *);
PyAPI_FUNC(unsigned long) PyType_GetFlags(PyTypeObject *);
PyAPI_FUNC(PyObject *) PyObject_GenericGetAttr(PyObject *, PyObject *);
PyAPI_FUNC(int) PyObject_GenericSetAttr(PyObject *, PyObject *, PyObject *);
PyAPI_FUNC(int) PyModule_AddObject(PyObject *, const char *, PyObject *);

PyAPI_FUNC(PyObject *) PyErr_Occurred(void);
PyAPI_FUNC(PyObject *) PyErr_NoMemory(void);
PyAPI_FUNC(void) PyErr_SetString(PyObject *, const char *);
PyAPI_FUNC(PyObject *) PyErr_Occurred(void);

PyAPI_FUNC(void) Py_Initialize(void);
PyAPI_FUNC(int) Py_FinalizeEx(void);
PyAPI_FUNC(int) PyRun_SimpleStringFlags(const char *, PyCompilerFlags *flags);

PyAPI_FUNC(void *) PyObject_Malloc(size_t size);
PyAPI_FUNC(void *) PyObject_Calloc(size_t nelem, size_t elsize);
PyAPI_FUNC(void *) PyObject_Realloc(void *ptr, size_t new_size);
PyAPI_FUNC(void) PyObject_Free(void *ptr);
PyAPI_FUNC(void *) PyMem_Malloc(size_t size);
PyAPI_FUNC(void *) PyMem_Calloc(size_t nelem, size_t elsize);
PyAPI_FUNC(void *) PyMem_Realloc(void *ptr, size_t new_size);
PyAPI_FUNC(void) PyMem_Free(void *ptr);
PyAPI_FUNC(void *) PyMem_RawMalloc(size_t size);
PyAPI_FUNC(void *) PyMem_RawCalloc(size_t nelem, size_t elsize);
PyAPI_FUNC(void *) PyMem_RawRealloc(void *ptr, size_t new_size);
PyAPI_FUNC(void) PyMem_RawFree(void *ptr);

/* Reused C-API functions */
PyAPI_FUNC(int) PyModule_AddIntConstant(PyObject *, const char *, long);
PyAPI_FUNC(int)
    PyModule_AddStringConstant(PyObject *, const char *, const char *);

/* Non C-API functions */
PyAPI_FUNC(int) PyBool_Check_Func(PyObject *);
PyAPI_FUNC(int) PyByteArray_Check_Func(PyObject *);
PyAPI_FUNC(int) PyLong_Check_Func(PyObject *);
PyAPI_FUNC(int) PyLong_CheckExact_Func(PyObject *);
PyAPI_FUNC(int) PyFloat_Check_Func(PyObject *);
PyAPI_FUNC(int) PyFloat_CheckExact_Func(PyObject *);
PyAPI_FUNC(int) PyUnicode_Check_Func(PyObject *);
PyAPI_FUNC(int) PyUnicode_CheckExact_Func(PyObject *);
PyAPI_FUNC(int) PyTuple_Check_Func(PyObject *);
PyAPI_FUNC(int) PyTuple_CheckExact_Func(PyObject *);
PyAPI_FUNC(int) PyList_Check_Func(PyObject *);
PyAPI_FUNC(int) PyList_CheckExact_Func(PyObject *);
PyAPI_FUNC(int) PyDict_Check_Func(PyObject *);
PyAPI_FUNC(int) PyDict_CheckExact_Func(PyObject *);
PyAPI_FUNC(int) PyType_Check_Func(PyObject *);
PyAPI_FUNC(int) PyType_CheckExact_Func(PyObject *);

PyAPI_FUNC(char *) PyByteArray_AS_STRING_Func(PyObject *);

/* Macros */
/* Multiline macros should retain their structure to get properly substituted */
/* clang-format off */
#define _Py_Dealloc \
    (*_Py_Dealloc_Func)

#define PyBool_Check(op) (PyBool_Check_Func((PyObject*)op))
#define PyByteArray_Check(op) (PyByteArray_Check_Func((PyObject*)op))
#define PyLong_Check(op) \
  PyLong_Check_Func((PyObject*)op)
#define PyLong_CheckExact(op) (PyLong_CheckExact_Func((PyObject*)op))
#define PyFloat_Check(op) PyFloat_Check_Func((PyObject*)op)
#define PyFloat_CheckExact(op) (PyFloat_CheckExact_Func((PyObject*)op))
#define PyUnicode_Check(op) \
  PyUnicode_Check_Func((PyObject*)op)
#define PyUnicode_CheckExact(op) (PyUnicode_CheckExact_Func((PyObject*)op))
#define PyTuple_Check(op) \
  PyTuple_Check_Func((PyObject*)op)
#define PyTuple_CheckExact(op) (PyTuple_CheckExact_Func((PyObject*)op))
#define PyList_Check(op) \
  PyList_Check_Func((PyObject*)op)
#define PyList_CheckExact(op) (PyList_CheckExact_Func((PyObject*)op))
#define PyDict_Check(op) \
  PyDict_Check_Func((PyObject*)op)
#define PyDict_CheckExact(op) (PyDict_CheckExact_Func((PyObject*)op))
#define PyType_Check(op) \
  PyType_Check_Func((PyObject*)op)
#define PyType_CheckExact(op) (PyType_CheckExact_Func((PyObject*)op))

#define PyByteArray_AS_STRING(self) \
    PyByteArray_AS_STRING_Func((PyObject*)self)

#define PyTuple_GET_SIZE(op) PyTuple_Size((PyObject*)op)
// TODO(T33954927): redefine PyTuple_GET_ITEM in a way that doesnt break pyro
#define PyTuple_SET_ITEM(op, i, v) PyTuple_SetItem((PyObject*)op, i, v)

#define PyUnicode_READY(op) \
  0

/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* !CPYTHON_FUNC_H */
