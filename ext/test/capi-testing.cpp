#include "gtest/gtest.h"

#include <cstring>

#include "Python.h"
#include "capi-testing.h"
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
  Object value(&scope, thread->exceptionValue());
  if (!value->isStr()) {
    UNIMPLEMENTED("Handle non string exception objects");
  }
  Str exception(&scope, *value);
  if (exception->equalsCStr(message)) return ::testing::AssertionSuccess();

  return ::testing::AssertionFailure() << '"' << exception << '"';
}

PyObject* moduleGet(const char* module, const char* name) {
  PyObject* mods = PyImport_GetModuleDict();
  PyObject* module_name = PyUnicode_FromString(module);
  PyObject* mod = PyDict_GetItem(mods, module_name);
  if (mod == nullptr) return nullptr;
  Py_DECREF(module_name);
  return PyObject_GetAttrString(mod, name);
}

int moduleSet(const char* module, const char* name, PyObject* value) {
  PyObject* mods = PyImport_GetModuleDict();
  PyObject* module_name = PyUnicode_FromString(module);
  PyObject* mod = PyDict_GetItem(mods, module_name);
  if (mod == nullptr && strcmp(module, "__main__") == 0) {
    // create __main__ if not yet available
    PyRun_SimpleString("");
    mod = PyDict_GetItem(mods, module_name);
  }
  if (mod == nullptr) return -1;
  Py_DECREF(module_name);

  PyObject* name_obj = PyUnicode_FromString(name);
  int ret = PyObject_SetAttr(mod, name_obj, value);
  Py_DECREF(name_obj);
  return ret;
}

PyObject* createUniqueObject() {
  PyObject* pytuple = PyTuple_New(1);
  PyTuple_SetItem(pytuple, 0, Py_None);
  DCHECK(Py_REFCNT(pytuple) == 1, "ref count should be 1");
  return pytuple;
}

}  // namespace testing
}  // namespace python
