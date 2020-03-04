#include "cpython-func.h"

#include "asserts.h"
#include "datetime.h"
#include "globals.h"

PY_EXPORT int PyDate_Check_Func(PyObject* obj) {
  PyObject* datetime_module = PyImport_ImportModule("datetime");
  if (datetime_module == nullptr) return -1;
  PyObject* date_type = PyObject_GetAttrString(datetime_module, "date");
  if (date_type == nullptr) return -1;
  int result = PyObject_IsInstance(obj, date_type);
  Py_DECREF(date_type);
  Py_DECREF(datetime_module);
  return result;
}

PY_EXPORT int PyDateTime_Check_Func(PyObject* obj) {
  PyObject* datetime_module = PyImport_ImportModule("datetime");
  if (datetime_module == nullptr) return -1;
  PyObject* datetime_type = PyObject_GetAttrString(datetime_module, "datetime");
  if (datetime_type == nullptr) return -1;
  int result = PyObject_IsInstance(obj, datetime_type);
  Py_DECREF(datetime_type);
  Py_DECREF(datetime_module);
  return result;
}

PY_EXPORT PyObject* PyDate_FromDate_Func(int year, int month, int day) {
  PyObject* datetime_module = PyImport_ImportModule("datetime");
  if (datetime_module == nullptr) return nullptr;
  PyObject* result =
      PyObject_CallMethod(datetime_module, "datetime", "iii", year, month, day);
  Py_DECREF(datetime_module);
  return result;
}

PY_EXPORT int PyDateTime_GET_DAY_Func(PyObject*) {
  UNIMPLEMENTED("PyDateTime_GET_DAY");
}

PY_EXPORT int PyDateTime_GET_HOUR_Func(PyObject*) {
  UNIMPLEMENTED("PyDateTime_GET_HOUR");
}

PY_EXPORT int PyDateTime_GET_MINUTE_Func(PyObject*) {
  UNIMPLEMENTED("PyDateTime_GET_MINUTE");
}

PY_EXPORT int PyDateTime_GET_MONTH_Func(PyObject*) {
  UNIMPLEMENTED("PyDateTime_GET_MONTH");
}

PY_EXPORT int PyDateTime_GET_SECOND_Func(PyObject*) {
  UNIMPLEMENTED("PyDateTime_GET_SECOND");
}

PY_EXPORT int PyDateTime_GET_YEAR_Func(PyObject*) {
  UNIMPLEMENTED("PyDateTime_GET_YEAR");
}
