#pragma once

#include "Python.h"

#ifdef __cplusplus
extern "C" {
#endif

PyAPI_FUNC_DECL(void* PyDateTimeAPI_Func(void));
PyAPI_FUNC_DECL(int PyDateTime_Check_Func(PyObject*));

PyAPI_FUNC_DECL(int PyDateTime_DATE_GET_HOUR_Func(PyObject*));
PyAPI_FUNC_DECL(int PyDateTime_DATE_GET_MICROSECOND_Func(PyObject*));
PyAPI_FUNC_DECL(int PyDateTime_DATE_GET_MINUTE_Func(PyObject*));
PyAPI_FUNC_DECL(int PyDateTime_DATE_GET_SECOND_Func(PyObject*));

PyAPI_FUNC_DECL(int PyDateTime_DELTA_GET_DAYS_Func(PyObject*));
PyAPI_FUNC_DECL(int PyDateTime_DELTA_GET_MICROSECONDS_Func(PyObject*));
PyAPI_FUNC_DECL(int PyDateTime_DELTA_GET_SECONDS_Func(PyObject*));

PyAPI_FUNC_DECL(int PyDateTime_GET_DAY_Func(PyObject*));
PyAPI_FUNC_DECL(int PyDateTime_GET_MONTH_Func(PyObject*));
PyAPI_FUNC_DECL(int PyDateTime_GET_YEAR_Func(PyObject*));

PyAPI_FUNC_DECL(void PyDateTime_IMPORT_Func(void));
PyAPI_FUNC_DECL(int PyDate_Check_Func(PyObject*));
PyAPI_FUNC_DECL(PyObject* PyDate_FromDate_Func(int year, int month, int day));
PyAPI_FUNC_DECL(int PyDelta_Check_Func(PyObject*));

#define PyDateTimeAPI (PyDateTimeAPI_Func())
#define PyDateTime_Check(op) (PyDateTime_Check_Func((PyObject*)(op)))
#define PyDateTime_DATE_GET_HOUR(op)                                           \
  (PyDateTime_DATE_GET_HOUR_Func((PyObject*)(op)))
#define PyDateTime_DATE_GET_MICROSECOND(op)                                    \
  (PyDateTime_DATE_GET_MICROSECOND_Func((PyObject*)(op)))
#define PyDateTime_DATE_GET_MINUTE(op)                                         \
  (PyDateTime_DATE_GET_MINUTE_Func((PyObject*)(op)))
#define PyDateTime_DATE_GET_SECOND(op)                                         \
  (PyDateTime_DATE_GET_SECOND_Func((PyObject*)(op)))
#define PyDateTime_DELTA_GET_DAYS(op)                                          \
  (PyDateTime_DELTA_GET_DAYS_Func((PyObject*)(op)))
#define PyDateTime_DELTA_GET_MICROSECONDS(op)                                  \
  (PyDateTime_DELTA_GET_MICROSECONDS_Func((PyObject*)(op)))
#define PyDateTime_DELTA_GET_SECONDS(op)                                       \
  (PyDateTime_DELTA_GET_SECONDS_Func((PyObject*)(op)))
#define PyDateTime_GET_DAY(op) (PyDateTime_GET_DAY_Func((PyObject*)(op)))
#define PyDateTime_GET_MONTH(op) (PyDateTime_GET_MONTH_Func((PyObject*)(op)))
#define PyDateTime_GET_YEAR(op) (PyDateTime_GET_YEAR_Func((PyObject*)(op)))
#define PyDateTime_IMPORT (PyDateTime_IMPORT_Func())
#define PyDate_Check(op) (PyDate_Check_Func((PyObject*)(op)))
#define PyDate_FromDate(year, month, day)                                      \
  (PyDate_FromDate_Func((year), (month), (day)))
#define PyDelta_Check(op) (PyDelta_Check_Func((PyObject*)(op)))

#ifdef __cplusplus
}
#endif
