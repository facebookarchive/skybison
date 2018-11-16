#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using DictExtensionApiTest = ExtensionApi;

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
  PyObject* value = PyLong_FromLong(20);

  // Insert key value
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);

  // Pass existing key
  PyObject* result = PyDict_GetItem(dict, key);
  ASSERT_NE(result, nullptr);

  // Check result value
  EXPECT_EQ(PyLong_AsLong(result), 20);
  EXPECT_TRUE(testing::isBorrowed(result));
}

}  // namespace python
