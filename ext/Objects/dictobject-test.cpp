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

}  // namespace python
