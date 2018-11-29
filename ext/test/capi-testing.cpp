#include "gtest/gtest.h"

#include <cstring>

#include "Python.h"
#include "capi-testing.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {
namespace testing {

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

PyObject* importGetModule(PyObject* name) {
  PyObject* modules_dict = PyImport_GetModuleDict();
  PyObject* module = PyDict_GetItem(modules_dict, name);
  Py_XINCREF(module);  // Return a new reference
  return module;
}

}  // namespace testing
}  // namespace python
