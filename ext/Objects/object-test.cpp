#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using ObjectExtensionApiTest = ExtensionApi;

TEST_F(ObjectExtensionApiTest, PyNoneIdentityIsEqual) {
  // Test Identitiy
  PyObject* none1 = Py_None;
  PyObject* none2 = Py_None;
  EXPECT_EQ(none1, none2);
}

TEST_F(ObjectExtensionApiTest, SetAttrWithInvalidTypeReturnsNegative) {
  PyObject* key = PyUnicode_FromString("a_key");
  PyObject* value = PyLong_FromLong(5);
  EXPECT_EQ(PyObject_GenericSetAttr(Py_None, key, value), -1);
}

TEST_F(ObjectExtensionApiTest, SetAttrWithInvalidKeyReturnsNegative) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "test",
  };
  PyObject* module = PyModule_Create(&def);
  PyObject* value = PyLong_FromLong(5);
  EXPECT_EQ(PyObject_GenericSetAttr(module, Py_None, value), -1);
}

TEST_F(ObjectExtensionApiTest, SetAttrReturnsZero) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "test",
  };
  PyObject* module = PyModule_Create(&def);
  PyObject* key = PyUnicode_FromString("a_key");
  PyObject* value = PyLong_FromLong(5);
  EXPECT_EQ(PyObject_GenericSetAttr(module, key, value), 0);
}

TEST_F(ObjectExtensionApiTest, GetAttrWithNoneExistingKeyReturnsNull) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "test",
  };
  int expected_int = 5;
  PyObject* module = PyModule_Create(&def);

  PyObject* key = PyUnicode_FromString("a_key");
  EXPECT_EQ(PyObject_GenericGetAttr(module, key), nullptr);
}

TEST_F(ObjectExtensionApiTest, GetAttrWithInvalidTypeReturnsNull) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "test",
  };
  int expected_int = 5;
  PyObject* module = PyModule_Create(&def);
  PyObject* key = PyUnicode_FromString("a_key");
  PyObject* value = PyLong_FromLong(expected_int);
  ASSERT_EQ(PyObject_GenericSetAttr(module, key, value), 0);

  EXPECT_EQ(PyObject_GenericGetAttr(Py_None, key), nullptr);
}

TEST_F(ObjectExtensionApiTest, GetAttrWithInvalidKeyReturnsNull) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "test",
  };
  int expected_int = 5;
  PyObject* module = PyModule_Create(&def);
  PyObject* key = PyUnicode_FromString("a_key");
  PyObject* value = PyLong_FromLong(expected_int);
  ASSERT_EQ(PyObject_GenericSetAttr(module, key, value), 0);

  EXPECT_EQ(PyObject_GenericGetAttr(module, Py_None), nullptr);
}

TEST_F(ObjectExtensionApiTest, GetAttrReturnsCorrectValue) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "test",
  };
  int expected_int = 5;
  PyObject* module = PyModule_Create(&def);
  PyObject* key = PyUnicode_FromString("a_key");
  PyObject* value = PyLong_FromLong(expected_int);
  ASSERT_EQ(PyObject_GenericSetAttr(module, key, value), 0);

  PyObject* dict_result = PyObject_GenericGetAttr(module, key);
  ASSERT_NE(dict_result, nullptr);
  EXPECT_EQ(PyLong_AsLong(dict_result), expected_int);
}

TEST_F(ObjectExtensionApiTest, GetAttrStringReturnsCorrectValue) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "test",
  };
  const char* key = "a_key";
  int expected_int = 5;
  PyObject* module = PyModule_Create(&def);
  PyObject* value = PyLong_FromLong(expected_int);
  ASSERT_EQ(PyObject_SetAttrString(module, key, value), 0);

  PyObject* dict_result = PyObject_GetAttrString(module, key);
  ASSERT_NE(dict_result, nullptr);
  EXPECT_EQ(PyLong_AsLong(dict_result), expected_int);
}

TEST_F(ObjectExtensionApiTest, RefCountDecreaseDeallocsHandle) {
  long value = 10;
  PyObject* o = PyLong_FromLong(value);
  Py_DECREF(o);
}

TEST_F(ObjectExtensionApiTest, IncrementDecrementRefCount) {
  PyObject* o = PyLong_FromLong(42);
  EXPECT_EQ(Py_REFCNT(o), 1);
  Py_INCREF(o);
  EXPECT_EQ(Py_REFCNT(o), 2);
  Py_DECREF(o);
  EXPECT_EQ(Py_REFCNT(o), 1);
}

}  // namespace python
