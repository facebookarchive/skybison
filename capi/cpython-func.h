#ifndef CPYTHON_FUNC_H
#define CPYTHON_FUNC_H
#ifdef __cplusplus
extern "C" {
#endif

#include "cpython-types.h"

#define PyAPI_FUNC(RTYPE) RTYPE

/* Singletons */
PyAPI_FUNC(PyObject *) PyFalse_Ptr();
PyAPI_FUNC(PyObject *) PyNone_Ptr();
PyAPI_FUNC(PyObject *) PyTrue_Ptr();

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

PyAPI_FUNC(PyObject *) PyErr_Occurred(void);
PyAPI_FUNC(PyObject *) PyErr_NoMemory(void);
PyAPI_FUNC(void) PyErr_SetString(PyObject *, const char *);
PyAPI_FUNC(PyObject *) PyErr_Occurred(void);

PyAPI_FUNC(void) Py_Initialize(void);
PyAPI_FUNC(int) Py_FinalizeEx(void);
PyAPI_FUNC(int) PyRun_SimpleStringFlags(const char *, PyCompilerFlags *flags);

/* Non C-API functions */
PyAPI_FUNC(int) PyBool_Check_Func(PyObject *);
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

/* Macros */
/* Multiline macros should retain their structure to get properly substituted */
/* clang-format off */
#define _Py_Dealloc \
    (*_Py_Dealloc_Func)

#define PyBool_Check(op) (PyBool_Check_Func((PyObject*)op))
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
/* clang-format on */

#ifdef __cplusplus
}

#endif
#endif /* !CPYTHON_FUNC_H */
