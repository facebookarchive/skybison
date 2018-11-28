#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

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
  PyObject* value = createUniqueObject();

  // Insert the value into the dictionary
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);

  // Record the reference count of the value
  long refcnt = Py_REFCNT(value);

  // Get a new reference to the value from the dictionary
  PyObject* value2 = PyDict_GetItem(dict, key);

  // The new reference should be equal to the original reference
  EXPECT_EQ(value2, value);

  // The reference count should not be affected
  EXPECT_EQ(Py_REFCNT(value), refcnt);

  Py_DECREF(value);
  Py_DECREF(key);
  Py_DECREF(dict);
}

TEST_F(DictExtensionApiTest, GetItemStringReturnsValue) {
  PyObjectPtr dict(PyDict_New());
  const char* key_cstr = "key";
  PyObjectPtr key(PyUnicode_FromString(key_cstr));
  PyObjectPtr value(createUniqueObject());
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);

  PyObject* item = PyDict_GetItemString(dict, key_cstr);
  EXPECT_EQ(item, value);
}

TEST_F(DictExtensionApiTest, SizeWithNonDictReturnsNegative) {
  PyObject* list = PyList_New(0);
  EXPECT_EQ(PyDict_Size(list), -1);
  EXPECT_NE(PyErr_Occurred(), nullptr);

  const char* expected_message = "bad argument to internal function";
  EXPECT_TRUE(testing::exceptionValueMatches(expected_message));

  Py_DECREF(list);
}

TEST_F(DictExtensionApiTest, SizeWithEmptyDictReturnsZero) {
  PyObject* dict = PyDict_New();
  EXPECT_EQ(PyDict_Size(dict), 0);
  Py_DECREF(dict);
}

TEST_F(DictExtensionApiTest, SizeWithNonEmptyDict) {
  PyObject* dict = PyDict_New();
  PyObject* key1 = PyLong_FromLong(1);
  PyObject* key2 = PyLong_FromLong(2);
  PyObject* value1 = createUniqueObject();
  PyObject* value2 = createUniqueObject();
  PyObject* value3 = createUniqueObject();

  // Dict starts out empty
  EXPECT_EQ(PyDict_Size(dict), 0);

  // Inserting items for two different keys
  ASSERT_EQ(PyDict_SetItem(dict, key1, value1), 0);
  ASSERT_EQ(PyDict_SetItem(dict, key2, value2), 0);
  EXPECT_EQ(PyDict_Size(dict), 2);

  // Replace value for existing key
  ASSERT_EQ(PyDict_SetItem(dict, key1, value3), 0);
  EXPECT_EQ(PyDict_Size(dict), 2);

  Py_DECREF(dict);
  Py_DECREF(key1);
  Py_DECREF(key2);
  Py_DECREF(value1);
  Py_DECREF(value2);
  Py_DECREF(value3);
}

}  // namespace python
