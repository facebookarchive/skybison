#pragma once

#include "Python.h"

#define PyDate_Check(op) (PyDate_Check_Func((PyObject*)(op)))
#define PyDateTime_Check(op) (PyDateTime_Check_Func((PyObject*)(op)))
#define PyDate_FromDate(year, month, day)                                      \
  (PyDate_FromDate_Func((year), (month), (day)))
#define PyDateTime_GET_DAY(op) (PyDateTime_GET_DAY_Func((PyObject*)(op)))
#define PyDateTime_IMPORT

#ifdef __cplusplus
extern "C" {
#endif

PyAPI_FUNC(int) PyDate_Check_Func(PyObject*);
PyAPI_FUNC(int) PyDateTime_Check_Func(PyObject*);
PyAPI_FUNC(PyObject*) PyDate_FromDate_Func(int year, int month, int day);
PyAPI_FUNC(int) PyDateTime_GET_DAY_Func(PyObject*);
PyAPI_FUNC(int) PyDateTime_GET_HOUR_Func(PyObject*);
PyAPI_FUNC(int) PyDateTime_GET_MINUTE_Func(PyObject*);
PyAPI_FUNC(int) PyDateTime_GET_MONTH_Func(PyObject*);
PyAPI_FUNC(int) PyDateTime_GET_SECOND_Func(PyObject*);
PyAPI_FUNC(int) PyDateTime_GET_YEAR_Func(PyObject*);

#ifdef __cplusplus
}
#endif
