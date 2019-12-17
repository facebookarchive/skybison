#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {

using ModSupportExtensionApiTest = ExtensionApi;

TEST_F(ModSupportExtensionApiTest, AddObjectAddsToModule) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
  };

  testing::PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);

  PyObject* obj = PyList_New(1);
  int myobj = PyModule_AddObject(module, "myobj", obj);
  ASSERT_NE(myobj, -1);

  ASSERT_EQ(testing::moduleSet("__main__", "mymodule", module), 0);
  PyRun_SimpleString("x = mymodule.myobj");

  testing::PyObjectPtr x(testing::moduleGet("__main__", "x"));
  ASSERT_TRUE(PyList_CheckExact(x));
}

TEST_F(ModSupportExtensionApiTest, RepeatedAddObjectOverwritesValue) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
  };

  testing::PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);

  PyObject* listobj = PyList_New(1);
  int myobj = PyModule_AddObject(module, "myobj", listobj);
  ASSERT_NE(myobj, -1);

  PyObject* tupleobj = PyTuple_New(1);
  myobj = PyModule_AddObject(module, "myobj", tupleobj);
  ASSERT_NE(myobj, -1);

  ASSERT_EQ(testing::moduleSet("__main__", "mymodule", module), 0);
  PyRun_SimpleString("x = mymodule.myobj");

  testing::PyObjectPtr x(testing::moduleGet("__main__", "x"));
  ASSERT_FALSE(PyList_CheckExact(x));
  ASSERT_TRUE(PyTuple_CheckExact(x));
}

TEST_F(ModSupportExtensionApiTest, AddStringConstantAddsToModule) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
  };

  testing::PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);

  const char* c_str = "string";
  ASSERT_EQ(PyModule_AddStringConstant(module, "mystr", c_str), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  testing::PyObjectPtr str(PyObject_GetAttrString(module, "mystr"));
  EXPECT_TRUE(isUnicodeEqualsCStr(str, c_str));
}

TEST_F(ModSupportExtensionApiTest, RepeatedAddStringConstantOverwritesValue) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
  };

  testing::PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);

  ASSERT_EQ(PyModule_AddStringConstant(module, "mystr", "hello"), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  const char* c_str = "world";
  ASSERT_EQ(PyModule_AddStringConstant(module, "mystr", c_str), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  testing::PyObjectPtr str(PyObject_GetAttrString(module, "mystr"));
  EXPECT_TRUE(isUnicodeEqualsCStr(str, c_str));
}

TEST_F(ModSupportExtensionApiTest, AddIntMacroAddsInt) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
  };

  testing::PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);

#pragma push_macro("MYINT")
#undef MYINT
#define MYINT 5
  ASSERT_EQ(PyModule_AddIntMacro(module, MYINT), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  testing::PyObjectPtr myint(PyObject_GetAttrString(module, "MYINT"));
  EXPECT_EQ(PyLong_AsLong(myint), MYINT);
#pragma pop_macro("MYINT")
}

TEST_F(ModSupportExtensionApiTest, BuildValue) {
  testing::PyObjectPtr a_str(Py_BuildValue("s", "hello, world"));
  EXPECT_TRUE(isUnicodeEqualsCStr(a_str, "hello, world"));
}

TEST_F(ModSupportExtensionApiTest, BuildValueStringLength) {
  testing::PyObjectPtr a_str(Py_BuildValue("s#", "hello, world", 5));
  EXPECT_TRUE(isUnicodeEqualsCStr(a_str, "hello"));
}

TEST_F(ModSupportExtensionApiTest, BuildValueInt) {
  testing::PyObjectPtr an_int(Py_BuildValue("i", 42));
  ASSERT_NE(an_int, nullptr);
  ASSERT_TRUE(PyLong_Check(an_int));
  ASSERT_EQ(PyLong_AsLong(an_int), 42);
}

TEST_F(ModSupportExtensionApiTest, BuildValueTupleOfInt) {
  testing::PyObjectPtr a_tuple(Py_BuildValue("iiii", 111, 222, 333, 444));
  ASSERT_NE(a_tuple, nullptr);
  ASSERT_TRUE(PyTuple_Check(a_tuple));

  PyObject* item0 = PyTuple_GetItem(a_tuple, 0);
  ASSERT_NE(item0, nullptr);
  ASSERT_EQ(PyLong_AsLong(item0), 111);

  PyObject* item1 = PyTuple_GetItem(a_tuple, 1);
  ASSERT_NE(item1, nullptr);
  ASSERT_EQ(PyLong_AsLong(item1), 222);

  PyObject* item2 = PyTuple_GetItem(a_tuple, 2);
  ASSERT_NE(item2, nullptr);
  ASSERT_EQ(PyLong_AsLong(item2), 333);

  PyObject* item3 = PyTuple_GetItem(a_tuple, 3);
  ASSERT_NE(item3, nullptr);
  ASSERT_EQ(PyLong_AsLong(item3), 444);
}

}  // namespace py
