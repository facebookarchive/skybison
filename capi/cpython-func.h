#ifndef CPYTHON_FUNC_H
#define CPYTHON_FUNC_H

#include "cpython-types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PyAPI_FUNC(RTYPE) RTYPE

/* Singletons */
PyAPI_FUNC(PyObject*) PyFalse_Ptr(void);
PyAPI_FUNC(PyObject*) PyNone_Ptr(void);
PyAPI_FUNC(PyObject*) PyTrue_Ptr(void);

PyAPI_FUNC(PyObject*) PyExc_BaseException_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_Exception_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_StopAsyncIteration_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_StopIteration_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_GeneratorExit_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ArithmeticError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_LookupError_Ptr(void);

PyAPI_FUNC(PyObject*) PyExc_AssertionError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_AttributeError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_BufferError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_EOFError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_FloatingPointError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_OSError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ImportError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ModuleNotFoundError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_IndexError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_KeyError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_KeyboardInterrupt_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_MemoryError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_NameError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_OverflowError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_RuntimeError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_RecursionError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_NotImplementedError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_SyntaxError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_IndentationError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_TabError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ReferenceError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_SystemError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_SystemExit_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_TypeError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_UnboundLocalError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_UnicodeError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_UnicodeEncodeError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_UnicodeDecodeError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_UnicodeTranslateError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ValueError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ZeroDivisionError_Ptr(void);

PyAPI_FUNC(PyObject*) PyExc_BlockingIOError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_BrokenPipeError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ChildProcessError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ConnectionError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ConnectionAbortedError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ConnectionRefusedError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ConnectionResetError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_FileExistsError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_FileNotFoundError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_InterruptedError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_IsADirectoryError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_NotADirectoryError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_PermissionError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ProcessLookupError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_TimeoutError_Ptr(void);

PyAPI_FUNC(PyObject*) PyExc_Warning_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_UserWarning_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_DeprecationWarning_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_PendingDeprecationWarning_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_SyntaxWarning_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_RuntimeWarning_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_FutureWarning_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ImportWarning_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_UnicodeWarning_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_BytesWarning_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ResourceWarning_Ptr(void);

/* Functions */
PyAPI_FUNC(PyObject*) PyBool_FromLong(long);
PyAPI_FUNC(int) PyDict_SetItem(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(int) PyDict_SetItemString(PyObject*, const char*, PyObject*);
PyAPI_FUNC(PyObject*) PyDict_New(void);
PyAPI_FUNC(PyObject*) PyDict_GetItem(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyFloat_FromDouble(double);
PyAPI_FUNC(PyObject*) PyList_New(Py_ssize_t);
PyAPI_FUNC(PyObject*) PyModule_Create2(struct PyModuleDef*, int);
PyAPI_FUNC(PyModuleDef*) PyModule_GetDef(PyObject*);
PyAPI_FUNC(PyObject*) PyModule_GetDict(PyObject*);
PyAPI_FUNC(PyObject*) PyLong_FromLong(long);
PyAPI_FUNC(long) PyLong_AsLong(PyObject*);
PyAPI_FUNC(void) _Py_Dealloc_Func(PyObject*);
PyAPI_FUNC(PyObject*) PyTuple_New(Py_ssize_t);
PyAPI_FUNC(Py_ssize_t) PyTuple_Size(PyObject*);
PyAPI_FUNC(int) PyTuple_SetItem(PyObject*, Py_ssize_t, PyObject*);
PyAPI_FUNC(PyObject*) PyTuple_GetItem(PyObject*, Py_ssize_t);
PyAPI_FUNC(PyObject*) PyTuple_Pack(Py_ssize_t, ...);
PyAPI_FUNC(PyObject*) PyUnicode_FromString(const char*);
PyAPI_FUNC(char*) PyUnicode_AsUTF8AndSize(PyObject*, Py_ssize_t*);
PyAPI_FUNC(char*) PyUnicode_AsUTF8(PyObject*);
PyAPI_FUNC(int) PyUnicode_Compare(PyObject*, PyObject*);
PyAPI_FUNC(int) PyUnicode_CompareWithASCIIString(PyObject*, const char*);
PyAPI_FUNC(int) _PyUnicode_EqualToASCIIString(PyObject*, const char*);
PyAPI_FUNC(int) PyType_Ready(PyTypeObject*);
PyAPI_FUNC(unsigned long) PyType_GetFlags(PyTypeObject*);
PyAPI_FUNC(PyObject*) PyObject_GenericGetAttr(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyObject_GetAttr(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyObject_GetAttrString(PyObject*, const char*);
PyAPI_FUNC(int) PyObject_GenericSetAttr(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(int) PyObject_SetAttr(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(int) PyObject_SetAttrString(PyObject*, const char*, PyObject*);
PyAPI_FUNC(int) PyModule_AddObject(PyObject*, const char*, PyObject*);
PyAPI_FUNC(PyObject*) PyImport_GetModuleDict(void);
PyAPI_FUNC(PyObject*) PyImport_AddModule(const char*);
PyAPI_FUNC(PyObject*) PyImport_AddModuleObject(PyObject*);
PyAPI_FUNC(int) PyArg_ParseTuple(PyObject* args, const char* format, ...);
PyAPI_FUNC(PyObject*) PyLong_FromLongLong(long long val);
PyAPI_FUNC(PyObject*) PyLong_FromSsize_t(ssize_t val);
PyAPI_FUNC(PyObject*) PyLong_FromUnsignedLong(unsigned long val);
PyAPI_FUNC(PyObject*) PyLong_FromUnsignedLongLong(unsigned long long val);
PyAPI_FUNC(long long) PyLong_AsLongLong(PyObject* obj);
PyAPI_FUNC(ssize_t) PyLong_AsSsize_t(PyObject* obj);
PyAPI_FUNC(size_t) PyLong_AsSize_t(PyObject* obj);
PyAPI_FUNC(unsigned long) PyLong_AsUnsignedLong(PyObject* obj);
PyAPI_FUNC(unsigned long long) PyLong_AsUnsignedLongLong(PyObject* obj);
PyAPI_FUNC(long) PyLong_AsLongAndOverflow(PyObject* obj, int* ovf);
PyAPI_FUNC(long long) PyLong_AsLongLongAndOverflow(PyObject* obj, int* ovf);
PyAPI_FUNC(PyObject*) PyModule_Create(PyModuleDef* def);
PyAPI_FUNC(unsigned long) PyLong_AsUnsignedLongMask(PyObject* obj);
PyAPI_FUNC(unsigned long long) PyLong_AsUnsignedLongLongMask(PyObject* obj);

PyAPI_FUNC(PyObject*) PyErr_Occurred(void);
PyAPI_FUNC(PyObject*) PyErr_NoMemory(void);
PyAPI_FUNC(void) PyErr_SetString(PyObject*, const char*);
PyAPI_FUNC(PyObject*) PyErr_Occurred(void);
PyAPI_FUNC(void) PyErr_Clear(void);

PyAPI_FUNC(void) Py_Initialize(void);
PyAPI_FUNC(int) Py_FinalizeEx(void);
PyAPI_FUNC(int) PyRun_SimpleStringFlags(const char*, PyCompilerFlags* flags);

PyAPI_FUNC(void*) PyObject_Malloc(size_t size);
PyAPI_FUNC(void*) PyObject_Calloc(size_t nelem, size_t elsize);
PyAPI_FUNC(void*) PyObject_Realloc(void* ptr, size_t new_size);
PyAPI_FUNC(void) PyObject_Free(void* ptr);
PyAPI_FUNC(void*) PyMem_Malloc(size_t size);
PyAPI_FUNC(void*) PyMem_Calloc(size_t nelem, size_t elsize);
PyAPI_FUNC(void*) PyMem_Realloc(void* ptr, size_t new_size);
PyAPI_FUNC(void) PyMem_Free(void* ptr);
PyAPI_FUNC(void*) PyMem_RawMalloc(size_t size);
PyAPI_FUNC(void*) PyMem_RawCalloc(size_t nelem, size_t elsize);
PyAPI_FUNC(void*) PyMem_RawRealloc(void* ptr, size_t new_size);
PyAPI_FUNC(void) PyMem_RawFree(void* ptr);

/* Reused C-API functions */
PyAPI_FUNC(int) PyModule_AddIntConstant(PyObject*, const char*, long);
PyAPI_FUNC(int) PyModule_AddStringConstant(PyObject*, const char*, const char*);

/* Non C-API functions */
PyAPI_FUNC(int) PyBool_Check_Func(PyObject*);
PyAPI_FUNC(int) PyByteArray_Check_Func(PyObject*);
PyAPI_FUNC(int) PyBytes_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyBytes_Check_Func(PyObject*);
PyAPI_FUNC(int) PyDict_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyDict_Check_Func(PyObject*);
PyAPI_FUNC(int) PyFloat_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyFloat_Check_Func(PyObject*);
PyAPI_FUNC(int) PyList_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyList_Check_Func(PyObject*);
PyAPI_FUNC(int) PyLong_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyLong_Check_Func(PyObject*);
PyAPI_FUNC(int) PyModule_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyModule_Check_Func(PyObject*);
PyAPI_FUNC(int) PyTuple_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyTuple_Check_Func(PyObject*);
PyAPI_FUNC(int) PyType_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyType_Check_Func(PyObject*);
PyAPI_FUNC(int) PyUnicode_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyUnicode_Check_Func(PyObject*);

PyAPI_FUNC(void) Py_DECREF_Func(PyObject*);
PyAPI_FUNC(void) Py_INCREF_Func(PyObject*);
PyAPI_FUNC(Py_ssize_t) Py_REFCNT_Func(PyObject*);
PyAPI_FUNC(void) Py_XDECREF_Func(PyObject*);
PyAPI_FUNC(void) Py_XINCREF_Func(PyObject*);

PyAPI_FUNC(char*) PyByteArray_AS_STRING_Func(PyObject*);

/* Macros */
/* Multiline macros should retain their structure to get properly substituted */
/* clang-format off */
#define _Py_Dealloc \
    (*_Py_Dealloc_Func)

/* Parentheses around the whole expressions are needed to compile
 * "if PyDict_CheckExact(other) {". Parentheses around "op" are needed for
 * "PyBytes_Check(state = PyTuple_GET_ITEM(args, 0))" */
#define PyBool_Check(op) (PyBool_Check_Func((PyObject*)(op)))
#define PyByteArray_Check(op) (PyByteArray_Check_Func((PyObject*)(op)))
#define PyBytes_Check(op) \
  (PyBytes_Check_Func((PyObject*)(op)))
#define PyBytes_CheckExact(op) (PyBytes_CheckExact_Func((PyObject*)(op)))
#define PyDict_Check(op) \
  (PyDict_Check_Func((PyObject*)(op)))
#define PyDict_CheckExact(op) (PyDict_CheckExact_Func((PyObject*)(op)))
#define PyFloat_Check(op) (PyFloat_Check_Func((PyObject*)(op)))
#define PyFloat_CheckExact(op) (PyFloat_CheckExact_Func((PyObject*)(op)))
#define PyList_Check(op) \
  (PyList_Check_Func((PyObject*)(op)))
#define PyList_CheckExact(op) (PyList_CheckExact_Func((PyObject*)(op)))
#define PyLong_Check(op) \
  (PyLong_Check_Func((PyObject*)(op)))
#define PyLong_CheckExact(op) (PyLong_CheckExact_Func((PyObject*)(op)))
#define PyModule_Check(op) (PyModule_Check_Func((PyObject*)(op)))
#define PyModule_CheckExact(op) (PyModule_CheckExact_Func((PyObject*)(op)))
#define PyTuple_Check(op) \
  (PyTuple_Check_Func((PyObject*)(op)))
#define PyTuple_CheckExact(op) (PyTuple_CheckExact_Func((PyObject*)(op)))
#define PyType_Check(op) \
  (PyType_Check_Func((PyObject*)(op)))
#define PyType_CheckExact(op) (PyType_CheckExact_Func((PyObject*)(op)))
#define PyUnicode_Check(op) \
  (PyUnicode_Check_Func((PyObject*)(op)))
#define PyUnicode_CheckExact(op) (PyUnicode_CheckExact_Func((PyObject*)(op)))

#define PyByteArray_AS_STRING(self) \
    PyByteArray_AS_STRING_Func((PyObject*)self)

#define PYTHON_API_VERSION 1013
#define PyModule_AddIntMacro(m, c) PyModule_AddIntConstant(m, #c, c)
#define PyModule_Create(module) \
    PyModule_Create2(module, PYTHON_API_VERSION)

#define PySet_GET_SIZE(op) PySet_Size((PyObject*)op)
#define PyTuple_GET_SIZE(op) PyTuple_Size((PyObject*)op)
// TODO(T33954927): redefine PyTuple_GET_ITEM in a way that doesnt break pyro
#define PyTuple_SET_ITEM(op, i, v) PyTuple_SetItem((PyObject*)op, i, v)

#define PyUnicode_READY(op) \
  0

#define Py_INCREF(op) (                         \
    Py_INCREF_Func((PyObject*)op))
#define Py_DECREF(op)                           \
    Py_DECREF_Func((PyObject*)op)
#define Py_REFCNT(op) Py_REFCNT_Func((PyObject*)op)
#define Py_XINCREF(op) (                         \
    Py_XINCREF_Func((PyObject*)op))
#define Py_XDECREF(op)                           \
    Py_XDECREF_Func((PyObject*)op)

#define PyObject_INIT(op, typeobj) \
    PyObject_Init((PyObject*)op, (PyTypeObject*)typeobj)
#define PyObject_INIT_VAR(op, typeobj, size) \
    PyObject_InitVar((PyVarObject*)op, (PyTypeObject*)typeobj, size)

/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* !CPYTHON_FUNC_H */
