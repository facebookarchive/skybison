#include "cpython-func.h"

#include "capi.h"
#include "datetime.h"
#include "globals.h"
#include "handles-decl.h"
#include "handles.h"
#include "runtime.h"
#include "thread.h"

using namespace py;

PY_EXPORT void* PyDateTimeAPI_Func(void) {
  PyObject* module = PyState_FindModule(&datetimemodule);
  if (module == nullptr) return nullptr;
  return &(datetime_state(module)->CAPI);
}

PY_EXPORT int PyDateTime_Check_Func(PyObject* obj) {
  return PyObject_TypeCheck(obj, datetime_global(CAPI.DateTimeType));
}

PY_EXPORT int PyDateTime_DATE_GET_HOUR_Func(PyObject* obj) {
  DCHECK(PyDateTime_Check(obj), "expected datetime object");
  return reinterpret_cast<PyDateTime_DateTime*>(obj)->data[4];
}

PY_EXPORT int PyDateTime_DATE_GET_MICROSECOND_Func(PyObject* obj) {
  DCHECK(PyDateTime_Check(obj), "expected datetime object");
  return (reinterpret_cast<PyDateTime_DateTime*>(obj)->data[7] << 16) |
         (reinterpret_cast<PyDateTime_DateTime*>(obj)->data[8] << 8) |
         reinterpret_cast<PyDateTime_DateTime*>(obj)->data[9];
}

PY_EXPORT int PyDateTime_DATE_GET_MINUTE_Func(PyObject* obj) {
  DCHECK(PyDateTime_Check(obj), "expected datetime object");
  return reinterpret_cast<PyDateTime_DateTime*>(obj)->data[5];
}

PY_EXPORT int PyDateTime_DATE_GET_SECOND_Func(PyObject* obj) {
  DCHECK(PyDateTime_Check(obj), "expected datetime object");
  return reinterpret_cast<PyDateTime_DateTime*>(obj)->data[6];
}

PY_EXPORT int PyDateTime_DELTA_GET_DAYS_Func(PyObject* obj) {
  DCHECK(PyDelta_Check(obj), "expected delta object");
  return reinterpret_cast<PyDateTime_Delta*>(obj)->days;
}

PY_EXPORT int PyDateTime_DELTA_GET_MICROSECONDS_Func(PyObject* obj) {
  DCHECK(PyDelta_Check(obj), "expected delta object");
  return reinterpret_cast<PyDateTime_Delta*>(obj)->microseconds;
}

PY_EXPORT int PyDateTime_DELTA_GET_SECONDS_Func(PyObject* obj) {
  DCHECK(PyDelta_Check(obj), "expected delta object");
  return reinterpret_cast<PyDateTime_Delta*>(obj)->seconds;
}

PY_EXPORT PyObject* PyDateTime_FromDateAndTime_Func(int year, int month,
                                                    int day, int hour, int min,
                                                    int sec, int usec) {
  return datetime_global(CAPI.DateTime_FromDateAndTime)(
      year, month, day, hour, min, sec, usec, Py_None,
      datetime_global(CAPI.DateTimeType));
}

PY_EXPORT int PyDateTime_GET_DAY_Func(PyObject* obj) {
  DCHECK(PyDate_Check(obj) || PyDateTime_Check(obj), "expected date object");
  return reinterpret_cast<PyDateTime_Date*>(obj)->data[3];
}

PY_EXPORT int PyDateTime_GET_MONTH_Func(PyObject* obj) {
  DCHECK(PyDate_Check(obj) || PyDateTime_Check(obj), "expected date object");
  return reinterpret_cast<PyDateTime_Date*>(obj)->data[2];
}

PY_EXPORT int PyDateTime_GET_YEAR_Func(PyObject* obj) {
  DCHECK(PyDate_Check(obj) || PyDateTime_Check(obj), "expected date object");
  return (reinterpret_cast<PyDateTime_Date*>(obj)->data[0] << 8) |
         reinterpret_cast<PyDateTime_Date*>(obj)->data[1];
}

PY_EXPORT void PyDateTime_IMPORT_Func(void) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str name(&scope, Runtime::internStrFromCStr(thread, "_datetime"));
  CHECK(!moduleInitBuiltinExtension(thread, name).isErrorException(),
        "failed to initialize _datetime module");
}

PY_EXPORT int PyDateTime_TIME_GET_HOUR_Func(PyObject* obj) {
  DCHECK(PyTime_Check(obj), "expected time object");
  return reinterpret_cast<PyDateTime_Time*>(obj)->data[0];
}

PY_EXPORT int PyDateTime_TIME_GET_MICROSECOND_Func(PyObject* obj) {
  DCHECK(PyTime_Check(obj), "expected time object");
  return (reinterpret_cast<PyDateTime_Time*>(obj)->data[3] << 16) |
         (reinterpret_cast<PyDateTime_Time*>(obj)->data[4] << 8) |
         reinterpret_cast<PyDateTime_Time*>(obj)->data[5];
}

PY_EXPORT int PyDateTime_TIME_GET_MINUTE_Func(PyObject* obj) {
  DCHECK(PyTime_Check(obj), "expected time object");
  return reinterpret_cast<PyDateTime_Time*>(obj)->data[1];
}

PY_EXPORT int PyDateTime_TIME_GET_SECOND_Func(PyObject* obj) {
  DCHECK(PyTime_Check(obj), "expected time object");
  return reinterpret_cast<PyDateTime_Time*>(obj)->data[2];
}

PY_EXPORT int PyDate_Check_Func(PyObject* obj) {
  return PyObject_TypeCheck(obj, datetime_global(CAPI.DateType));
}

PY_EXPORT PyObject* PyDate_FromDate_Func(int year, int month, int day) {
  return datetime_global(CAPI.Date_FromDate)(year, month, day,
                                             datetime_global(CAPI.DateType));
}

PY_EXPORT int PyDelta_Check_Func(PyObject* obj) {
  return PyObject_TypeCheck(obj, datetime_global(CAPI.DeltaType));
}

PY_EXPORT PyObject* PyDelta_FromDSU_Func(int days, int seconds, int useconds) {
  return datetime_global(CAPI.Delta_FromDelta)(days, seconds, useconds, 1,
                                               datetime_global(CAPI.DeltaType));
}

PY_EXPORT int PyTime_Check_Func(PyObject* obj) {
  return PyObject_TypeCheck(obj, datetime_global(CAPI.TimeType));
}
