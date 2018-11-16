#include "gtest/gtest.h"

#include "runtime/runtime.h"
#include "runtime/test-utils.h"
#include "Python.h"

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

TEST(ExtensionTest, SpamModule) {
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

} // namespace python
