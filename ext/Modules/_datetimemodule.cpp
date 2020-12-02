#include "cpython-func.h"

#include "capi.h"
#include "datetime.h"
#include "globals.h"
#include "handles-decl.h"
#include "handles.h"
#include "runtime.h"
#include "thread.h"

using namespace py;

PY_EXPORT int PyDateTime_Check_Func(PyObject* obj) {
  return PyObject_TypeCheck(obj, datetime_global(CAPI.DateTimeType));
}

PY_EXPORT int PyDateTime_GET_DAY_Func(PyObject* obj) {
  return reinterpret_cast<PyDateTime_Date*>(obj)->data[3];
}

PY_EXPORT void PyDateTime_IMPORT_Func(void) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str name(&scope, Runtime::internStrFromCStr(thread, "_datetime"));
  CHECK(!moduleInitBuiltinExtension(thread, name).isErrorException(),
        "failed to initialize _datetime module");
}

PY_EXPORT int PyDate_Check_Func(PyObject* obj) {
  return PyObject_TypeCheck(obj, datetime_global(CAPI.DateType));
}

PY_EXPORT PyObject* PyDate_FromDate_Func(int year, int month, int day) {
  return datetime_global(CAPI.Date_FromDate)(year, month, day,
                                             datetime_global(CAPI.DateType));
}
