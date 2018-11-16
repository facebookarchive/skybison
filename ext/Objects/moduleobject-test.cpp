#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using ModuleExtensionApiTest = ExtensionApi;

TEST_F(ModuleExtensionApiTest, SpamModule) {
  PyModuleDef def = {
      PyModuleDef_HEAD_INIT,
      "spam",
  };

  // PyInit_spam
  const int val = 5;
  {
    PyObject *m, *d, *de;
    m = PyModule_Create(&def);
    d = PyModule_GetDict(m);
    de = PyDict_New();
    PyDict_SetItemString(d, "constants", de);

    PyObject* u = PyUnicode_FromString("CONST");
    PyObject* v = PyLong_FromLong(val);
    PyDict_SetItem(d, u, v);
    PyDict_SetItem(de, v, u);
  }

  PyRun_SimpleString(R"(
import spam
x = spam.CONST
)");

  PyObject* x = testing::moduleGet("__main__", "x");
  int result = PyLong_AsLong(x);
  ASSERT_EQ(result, val);
}

TEST_F(ModuleExtensionApiTest, GetDefWithExtensionModuleRetunsNonNull) {
  PyModuleDef def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
  };

  PyObject* module = PyModule_Create(&def);
  ASSERT_NE(module, nullptr);

  PyModuleDef* result = PyModule_GetDef(module);
  EXPECT_EQ(result, &def);
}

TEST_F(ModuleExtensionApiTest, GetDefWithNonModuleRetunsNull) {
  PyObject* integer = PyBool_FromLong(0);
  PyModuleDef* result = PyModule_GetDef(integer);
  EXPECT_EQ(result, nullptr);
}

}  // namespace python
