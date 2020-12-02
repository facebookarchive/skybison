#pragma once

#include "Python.h"

#ifdef __cplusplus
extern "C" {
#endif

PyAPI_FUNC_DECL(int PyDateTime_Check_Func(PyObject*));
PyAPI_FUNC_DECL(int PyDateTime_GET_DAY_Func(PyObject*));
PyAPI_FUNC_DECL(void PyDateTime_IMPORT_Func(void));
PyAPI_FUNC_DECL(int PyDate_Check_Func(PyObject*));
PyAPI_FUNC_DECL(PyObject* PyDate_FromDate_Func(int year, int month, int day));

#define PyDateTime_Check(op) (PyDateTime_Check_Func((PyObject*)(op)))
#define PyDateTime_GET_DAY(op) (PyDateTime_GET_DAY_Func((PyObject*)(op)))
#define PyDateTime_IMPORT (PyDateTime_IMPORT_Func())
#define PyDate_Check(op) (PyDate_Check_Func((PyObject*)(op)))
#define PyDate_FromDate(year, month, day)                                      \
  (PyDate_FromDate_Func((year), (month), (day)))

#ifdef __cplusplus
}
#endif
