#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using DictExtensionApiTest = ExtensionApi;

TEST_F(DictExtensionApiTest, ClearFreeListReturnsZero) {
  EXPECT_EQ(PyDict_ClearFreeList(), 0);
}

TEST_F(DictExtensionApiTest, GetItemFromNonDictionaryReturnsNull) {
  // Pass a non dictionary
  PyObject* result = PyDict_GetItem(Py_None, Py_None);
  EXPECT_EQ(result, nullptr);
}

TEST_F(DictExtensionApiTest, GetItemNonExistingKeyReturnsNull) {
  PyObject* dict = PyDict_New();
  PyObject* nonkey = PyLong_FromLong(10);

  // Pass a non existing key
  PyObject* result = PyDict_GetItem(dict, nonkey);
  EXPECT_EQ(result, nullptr);
}

TEST_F(DictExtensionApiTest, GetItemReturnsBorrowedValue) {
  PyObject* dict = PyDict_New();
  PyObject* key = PyLong_FromLong(10);
  PyObject* value = testing::createUniqueObject();
  EXPECT_EQ(Py_REFCNT(value), 1);

  // Insert key value
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);
  EXPECT_EQ(Py_REFCNT(value), 2);

  // Pass existing key
  PyObject* result = PyDict_GetItem(dict, key);
  ASSERT_NE(result, nullptr);

  // Check result value
  EXPECT_EQ(result, value);
  // PyDict_GetItem "borrows" a reference for the return value.  Verify the
  // reference count did not change.
  EXPECT_EQ(Py_REFCNT(result), 2);
}

TEST_F(DictExtensionApiTest, SizeWithNonDictReturnsNegative) {
  PyObject* list = PyList_New(0);
  EXPECT_EQ(PyDict_Size(list), -1);
  EXPECT_NE(PyErr_Occurred(), nullptr);

  const char* expected_message = "bad argument to internal function";
  EXPECT_TRUE(testing::exceptionValueMatches(expected_message));

  Py_DECREF_Func(list);
}

TEST_F(DictExtensionApiTest, SizeWithEmptyDictReturnsZero) {
  PyObject* dict = PyDict_New();
  EXPECT_EQ(PyDict_Size(dict), 0);
  Py_DECREF_Func(dict);
}

TEST_F(DictExtensionApiTest, SizeWithNonEmptyDict) {
  PyObject* dict = PyDict_New();
  PyObject* key1 = PyLong_FromLong(1);
  PyObject* key2 = PyLong_FromLong(2);
  PyObject* value1 = testing::createUniqueObject();
  PyObject* value2 = testing::createUniqueObject();
  PyObject* value3 = testing::createUniqueObject();

  // Dict starts out empty
  EXPECT_EQ(PyDict_Size(dict), 0);

  // Inserting items for two different keys
  ASSERT_EQ(PyDict_SetItem(dict, key1, value1), 0);
  ASSERT_EQ(PyDict_SetItem(dict, key2, value2), 0);
  EXPECT_EQ(PyDict_Size(dict), 2);

  // Replace value for existing key
  ASSERT_EQ(PyDict_SetItem(dict, key1, value3), 0);
  EXPECT_EQ(PyDict_Size(dict), 2);

  Py_DECREF_Func(dict);
  Py_DECREF_Func(key1);
  Py_DECREF_Func(key2);
  Py_DECREF_Func(value1);
  Py_DECREF_Func(value2);
  Py_DECREF_Func(value3);
}

}  // namespace python
