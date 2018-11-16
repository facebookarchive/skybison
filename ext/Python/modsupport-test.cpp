#include "capi-fixture.h"

namespace python {

using ModSupportExtensionApiTest = ExtensionApi;

TEST_F(ModSupportExtensionApiTest, AddIntConstantAddsToModule) {
  PyModuleDef def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
  };

  PyObject* module = PyModule_Create(&def);
  ASSERT_NE(module, nullptr);

  int myglobal = PyModule_AddIntConstant(module, "myglobal", 123);
  ASSERT_NE(myglobal, -1);

  PyRun_SimpleString(R"(
import mymodule
x = mymodule.myglobal
)");

  PyObject* x = _PyModuleGet("__main__", "x");
  int result = PyLong_AsLong(x);
  ASSERT_EQ(result, 123);
}

TEST_F(ModSupportExtensionApiTest, AddIntConstantWithNullNameFails) {
  PyModuleDef def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
  };

  PyObject* module = PyModule_Create(&def);
  ASSERT_NE(module, nullptr);

  int result = PyModule_AddIntConstant(module, nullptr, 123);
  ASSERT_EQ(result, -1);
}

TEST_F(ModSupportExtensionApiTest, RepeatedAddIntConstantOverwritesValue) {
  PyModuleDef def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
  };

  PyObject* module = PyModule_Create(&def);
  ASSERT_NE(module, nullptr);

  int myglobal = PyModule_AddIntConstant(module, "myglobal", 123);
  ASSERT_NE(myglobal, -1);

  myglobal = PyModule_AddIntConstant(module, "myglobal", 456);
  ASSERT_NE(myglobal, -1);

  PyRun_SimpleString(R"(
import mymodule
x = mymodule.myglobal
)");

  PyObject* x = _PyModuleGet("__main__", "x");
  int result = PyLong_AsLong(x);
  ASSERT_EQ(result, 456);
}

}  // namespace python
