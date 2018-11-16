#ifndef CPYTHON_DATA_H
#define CPYTHON_DATA_H
#ifdef __cplusplus
extern "C" {
#endif

#include "cpython-func.h"

/* Types */
#define PyBaseObject_Type (*PyBaseObject_Type_Ptr())
#define PyBool_Type (*PyBool_Type_Ptr())
#define PyDict_Type (*PyDict_Type_Ptr())
#define PyFloat_Type (*PyFloat_Type_Ptr())
#define PyList_Type (*PyList_Type_Ptr())
#define PyLong_Type (*PyLong_Type_Ptr())
#define PyModule_Type (*PyModule_Type_Ptr())
#define PyTuple_Type (*PyTuple_Type_Ptr())
#define PyType_Type (*PyType_Type_Ptr())
#define PyUnicode_Type (*PyUnicode_Type_Ptr())
#define _PyNone_Type (*PyNone_Type_Ptr())

/* Singletons */
#define Py_False ((PyObject *)&(*PyFalse_Ptr()))
#define Py_None ((PyObject *)&(*PyNone_Ptr()))
#define Py_True ((PyObject *)&(*PyTrue_Ptr()))

#ifdef __cplusplus
}
#endif
#endif /* !CPYTHON_DATA_H */
