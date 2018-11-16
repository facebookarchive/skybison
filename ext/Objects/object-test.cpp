#include "capi-fixture.h"

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
  int result = PyObject_GenericSetAttr(Py_None, key, value);
  EXPECT_EQ(result, -1);
}

TEST_F(ObjectExtensionApiTest, SetAttrWithInvalidKeyReturnsNegative) {
  PyModuleDef def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  PyObject* module = PyModule_Create(&def);
  PyObject* value = PyLong_FromLong(5);
  int result = PyObject_GenericSetAttr(module, Py_None, value);
  EXPECT_EQ(result, -1);
}

TEST_F(ObjectExtensionApiTest, SetAttrReturnsZero) {
  PyModuleDef def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  PyObject* module = PyModule_Create(&def);
  PyObject* key = PyUnicode_FromString("a_key");
  PyObject* value = PyLong_FromLong(5);
  int result = PyObject_GenericSetAttr(module, key, value);
  EXPECT_EQ(result, 0);
}

TEST_F(ObjectExtensionApiTest, GetAttrWithNoneExistingKeyReturnsNull) {
  PyModuleDef def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  int expected_int = 5;
  PyObject* module = PyModule_Create(&def);

  PyObject* key = PyUnicode_FromString("a_key");
  PyObject* dict_result = PyObject_GenericGetAttr(module, key);
  EXPECT_EQ(dict_result, nullptr);
}

TEST_F(ObjectExtensionApiTest, GetAttrWithInvalidTypeReturnsNull) {
  PyModuleDef def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  int expected_int = 5;
  PyObject* module = PyModule_Create(&def);
  PyObject* key = PyUnicode_FromString("a_key");
  PyObject* value = PyLong_FromLong(expected_int);
  ASSERT_EQ(PyObject_GenericSetAttr(module, key, value), 0);

  PyObject* dict_result = PyObject_GenericGetAttr(Py_None, key);
  EXPECT_EQ(dict_result, nullptr);
}

TEST_F(ObjectExtensionApiTest, GetAttrWithInvalidKeyReturnsNull) {
  PyModuleDef def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  int expected_int = 5;
  PyObject* module = PyModule_Create(&def);
  PyObject* key = PyUnicode_FromString("a_key");
  PyObject* value = PyLong_FromLong(expected_int);
  ASSERT_EQ(PyObject_GenericSetAttr(module, key, value), 0);

  PyObject* dict_result = PyObject_GenericGetAttr(module, Py_None);
  EXPECT_EQ(dict_result, nullptr);
}

TEST_F(ObjectExtensionApiTest, GetAttrReturnsZero) {
  PyModuleDef def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  int expected_int = 5;
  PyObject* module = PyModule_Create(&def);
  PyObject* key = PyUnicode_FromString("a_key");
  PyObject* value = PyLong_FromLong(expected_int);
  ASSERT_EQ(PyObject_GenericSetAttr(module, key, value), 0);

  PyObject* dict_result = PyObject_GenericGetAttr(module, key);
  int result = PyLong_AsLong(dict_result);
  EXPECT_EQ(result, expected_int);
}

TEST_F(ObjectExtensionApiTest, RefCountDecreaseDeallocsHandle) {
  long value = 10;
  PyObject* o = PyLong_FromLong(value);
  Py_DECREF(o);
}

}  // namespace python
