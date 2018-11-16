#include "capi-testing.h"
#include "cpython-func.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {
namespace testing {

::testing::AssertionResult exceptionValueMatches(const char* message) {
  Thread* thread = Thread::currentThread();
  if (!thread->hasPendingException()) {
    return ::testing::AssertionFailure() << "no pending exception";
  }
  HandleScope scope(thread);
  Handle<Object> value(&scope, thread->exceptionValue());
  if (!value->isStr()) {
    UNIMPLEMENTED("Handle non string exception objects");
  }
  Handle<Str> exception(&scope, *value);
  if (exception->equalsCStr(message)) return ::testing::AssertionSuccess();

  return ::testing::AssertionFailure() << '"' << exception << '"';
}

PyObject* moduleGet(const char* module, const char* name) {
  PyObject* mods = PyImport_GetModuleDict();
  PyObject* module_name = PyUnicode_FromString(module);
  PyObject* mod = PyDict_GetItem(mods, module_name);
  Py_DECREF(module_name);
  return PyObject_GetAttrString(mod, name);
}

bool isBorrowed(PyObject* pyobj) {
  return ApiHandle::fromPyObject(pyobj)->isBorrowed();
}

}  // namespace testing
}  // namespace python
