#include "gtest/gtest.h"

#include "Python.h"
#include "runtime.h"
#include "test-utils.h"

static PyMethodDef SpamMethods[] = {{NULL, NULL, -1, NULL}};

static struct PyModuleDef spammodule = {
    PyModuleDef_HEAD_INIT,
    "spam",
    NULL,
    -1,
    SpamMethods,
    NULL,
    NULL,
    NULL,
    NULL,
};

PyMODINIT_FUNC PyInit_spam(void) {
  PyObject *m, *d, *de;
  m = PyModule_Create(&spammodule);
  d = PyModule_GetDict(m);
  de = PyDict_New();
  PyDict_SetItemString(d, "constants", de);

  PyObject* u = PyUnicode_FromString("CONST");
  PyObject* v = PyLong_FromLong(5);
  PyDict_SetItem(d, u, v);
  PyDict_SetItem(de, v, u);
  return m;
}

namespace python {

using namespace testing;

TEST(ModuleObject, SpamModule) {
  Runtime runtime;
  HandleScope scope;

  PyInit_spam();

  const char* src = R"(
import spam
print(spam.CONST)
)";

  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "5\n");
}

TEST(ModuleObject, GetDefwithExtensionModuleRetunsNonNull) {
  Runtime runtime;
  HandleScope scope;

  PyModuleDef def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
      nullptr,
      -1,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
  };

  PyObject* module = PyModule_Create(&def);
  ASSERT_NE(module, nullptr);

  PyModuleDef* result = PyModule_GetDef(module);
  EXPECT_EQ(result, &def);
}

TEST(ModuleObject, GetDefWithNonModuleRetunsNull) {
  Runtime runtime;
  HandleScope scope;

  PyObject* integer = PyBool_FromLong(0);
  PyModuleDef* result = PyModule_GetDef(integer);
  EXPECT_EQ(result, nullptr);
}

}  // namespace python
