#ifndef CPYTHON_DATA_H
#define CPYTHON_DATA_H
#ifdef __cplusplus
extern "C" {
#endif

#include "cpython-func.h"

/* Singletons */
#define Py_False ((PyObject *)&(*PyFalse_Ptr()))
#define Py_None ((PyObject *)&(*PyNone_Ptr()))
#define Py_True ((PyObject *)&(*PyTrue_Ptr()))

#ifdef __cplusplus
}
#endif
#endif /* !CPYTHON_DATA_H */
