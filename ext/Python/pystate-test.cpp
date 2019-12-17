#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {

using namespace testing;

using PystateExtensionApiTest = ExtensionApi;
using PystateExtensionApiTestDeathTest = ExtensionApi;

TEST_F(PystateExtensionApiTestDeathTest, AddModuleWithNullDefDeathTest) {
  EXPECT_DEATH(PyState_AddModule(Py_None, nullptr),
               "Module Definition is NULL");
}

TEST_F(PystateExtensionApiTestDeathTest, AddExistingModuleDoesNotOverride) {
  static struct PyModuleDef def = {
      PyModuleDef_HEAD_INIT,
      "foo",
      "docs",
      0,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
  };
  PyModuleDef_Init(&def);
  PyObjectPtr module(PyModule_New("foo"));
  ASSERT_NE(module, nullptr);
  ASSERT_EQ(PyState_AddModule(module, &def), 0);
  EXPECT_DEATH(PyState_AddModule(module, &def), "Module already added!");
}

TEST_F(PystateExtensionApiTest, AddModuleWithSlotsRaisesSystemError) {
  struct PyModuleDef def = {
      PyModuleDef_HEAD_INIT,
      "rutabaga",
      "I'm sure this module will turnip somewhere.",
      0,
      nullptr,
      reinterpret_cast<PyModuleDef_Slot*>(5),
      nullptr,
      nullptr,
  };
  PyModuleDef_Init(&def);
  EXPECT_EQ(PyState_AddModule(Py_None, &def), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(PystateExtensionApiTest, AddModuleWithNullDocUsesNone) {
  static struct PyModuleDef def = {
      PyModuleDef_HEAD_INIT,
      "foo",
      nullptr,
      0,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
  };
  PyObjectPtr module(PyModule_Create(&def));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr docstring(PyObject_GetAttrString(module, "__doc__"));
  EXPECT_EQ(docstring, Py_None);
}

TEST_F(PystateExtensionApiTest, AddModuleAddsModule) {
  static struct PyModuleDef def = {
      PyModuleDef_HEAD_INIT,
      "rutabaga",
      "I'm sure this module will turnip somewhere.",
      0,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
  };
  PyObject* module = PyModule_Create(&def);
  ASSERT_NE(module, nullptr);
  ASSERT_EQ(PyState_AddModule(module, &def), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObject* found_module = PyState_FindModule(&def);
  ASSERT_NE(found_module, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyModule_CheckExact(found_module));
  PyObjectPtr found_name(PyModule_GetNameObject(found_module));
  PyObjectPtr module_name(PyModule_GetNameObject(module));
  EXPECT_EQ(PyUnicode_Compare(found_name, module_name), 0);
  Py_DECREF(module);
}

TEST_F(PystateExtensionApiTest, FindModuleWithSlotsReturnsNull) {
  struct PyModuleDef def = {
      PyModuleDef_HEAD_INIT,
      "builtins",
      "Uh, the builtins module, I guess",
      0,
      nullptr,
      reinterpret_cast<PyModuleDef_Slot*>(5),
      nullptr,
      nullptr,
  };
  PyModuleDef_Init(&def);
  EXPECT_EQ(PyState_FindModule(&def), nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(PystateExtensionApiTest, FindModuleWithNonExistentModuleReturnsNull) {
  struct PyModuleDef def = {
      PyModuleDef_HEAD_INIT,
      "rutabaga",
      "I'm sure this module will turnip somewhere.",
      0,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
  };
  PyModuleDef_Init(&def);
  EXPECT_EQ(PyState_FindModule(&def), nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

static int recurseUntilLimit(PyThreadState* tstate, int limit) {
  if (Py_EnterRecursiveCall("") != 0) {
    return -1;
  }

  int result = 0;
  if (_PyThreadState_GetRecursionDepth(tstate) != limit) {
    result = recurseUntilLimit(tstate, limit);
  }
  Py_LeaveRecursiveCall();
  return result;
}

TEST_F(PystateExtensionApiTest, RecursionDepthStopsInfiniteRecursion) {
  PyThreadState* tstate = PyThreadState_Get();
  Py_SetRecursionLimit(50);
  int limit = Py_GetRecursionLimit() - 1;
  EXPECT_EQ(recurseUntilLimit(tstate, limit), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(PystateExtensionApiTest,
       RecursionDepthExceedingLimitRaisesRecursionError) {
  PyThreadState* tstate = PyThreadState_Get();
  Py_SetRecursionLimit(50);
  int limit = Py_GetRecursionLimit() + 1;
  EXPECT_EQ(recurseUntilLimit(tstate, limit), -1);
  EXPECT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_RecursionError));
}

}  // namespace py
