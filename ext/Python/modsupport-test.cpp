#include "gtest/gtest.h"

#include "Python.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ModSupport, AddIntConstantAddsToModule) {
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

  int result = PyModule_AddIntConstant(module, "myglobal", 123);
  ASSERT_NE(result, -1);

  runtime.runFromCString(R"(
import mymodule
x = mymodule.myglobal
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> x(&scope, moduleAt(&runtime, main, "x"));
  ASSERT_TRUE(x->isSmallInt());
  ASSERT_EQ(SmallInt::cast(*x)->value(), 123);
}

TEST(ModSupport, AddIntConstantWithNullNameFails) {
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

  int result = PyModule_AddIntConstant(module, nullptr, 123);
  ASSERT_EQ(result, -1);
}

TEST(ModSupport, RepeatedAddIntConstantOverwritesValue) {
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

  int result = PyModule_AddIntConstant(module, "myglobal", 123);
  ASSERT_NE(result, -1);

  result = PyModule_AddIntConstant(module, "myglobal", 456);
  ASSERT_NE(result, -1);

  runtime.runFromCString(R"(
import mymodule
x = mymodule.myglobal
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> x(&scope, moduleAt(&runtime, main, "x"));
  ASSERT_TRUE(x->isSmallInt());
  ASSERT_EQ(SmallInt::cast(*x)->value(), 456);
}

}  // namespace python
