#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using SetExtensionApiTest = ExtensionApi;

TEST_F(SetExtensionApiTest, AddWithNonSetReturnsNegative) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr key(PyLong_FromLong(1));

  EXPECT_EQ(PySet_Add(dict, key), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(SetExtensionApiTest, ClearFreeListReturnsZeroPyro) {
  EXPECT_EQ(PySet_ClearFreeList(), 0);
}

TEST_F(SetExtensionApiTest, ContainsReturnsPositiveAfterAdd) {
  PyObjectPtr set(PySet_New(nullptr));
  PyObjectPtr key(PyLong_FromLong(1));

  ASSERT_EQ(PySet_Contains(set, key), 0);
  ASSERT_EQ(PySet_Add(set, key), 0);
  EXPECT_EQ(PySet_Contains(set, key), 1);
}

TEST_F(SetExtensionApiTest, ContainsWithEmptySetReturnsFalse) {
  PyObjectPtr set(PySet_New(nullptr));
  PyObjectPtr key(PyLong_FromLong(1));

  EXPECT_EQ(PySet_Contains(set, key), 0);
}

TEST_F(SetExtensionApiTest, ContainsWithNonSetReturnsNegative) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr key(PyLong_FromLong(1));

  EXPECT_EQ(PySet_Contains(dict, key), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(SetExtensionApiTest, NewWithDictCopiesKeys) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr key1(PyLong_FromLong(1));
  PyObjectPtr key2(PyLong_FromLong(2));
  PyObjectPtr key3(PyLong_FromLong(3));
  PyObjectPtr value(PyLong_FromLong(4));

  PyDict_SetItem(dict, key1, value);
  PyDict_SetItem(dict, key2, value);
  PyDict_SetItem(dict, key3, value);

  PyObjectPtr set(PySet_New(dict));

  EXPECT_EQ(PySet_Contains(set, key1), 1);
  EXPECT_EQ(PySet_Contains(set, key2), 1);
  EXPECT_EQ(PySet_Contains(set, key3), 1);
}

TEST_F(SetExtensionApiTest, NewFromSet) {
  PyObjectPtr set(PySet_New(nullptr));
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));

  ASSERT_EQ(PySet_Add(set, one), 0);
  ASSERT_EQ(PySet_Add(set, two), 0);

  PyObjectPtr set_copy(PySet_New(set));

  EXPECT_EQ(PySet_Contains(set_copy, one), 1);
  EXPECT_EQ(PySet_Contains(set_copy, two), 1);
  EXPECT_EQ(PySet_Size(set_copy), 2);
}

TEST_F(SetExtensionApiTest, NewWithList) {
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));
  PyObjectPtr list(PyList_New(0));

  PyList_Append(list, one);
  PyList_Append(list, two);
  PyList_Append(list, one);

  PyObjectPtr set(PySet_New(list));
  EXPECT_EQ(PySet_Contains(set, one), 1);
  EXPECT_EQ(PySet_Contains(set, two), 1);
  EXPECT_EQ(PySet_Size(set), 2);
}

TEST_F(SetExtensionApiTest, NewWithNonIterableReturnsNull) {
  PyObjectPtr num(PyLong_FromLong(1));

  EXPECT_EQ(PySet_New(num), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(SetExtensionApiTest, NewWithNullReturnsEmpty) {
  PyObjectPtr set(PySet_New(nullptr));
  ASSERT_NE(set, nullptr);
  EXPECT_EQ(PySet_Size(set), 0);
}

TEST_F(SetExtensionApiTest, SizeIncreasesAfterAdd) {
  PyObjectPtr set(PySet_New(nullptr));
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));

  EXPECT_EQ(PySet_Size(set), 0);
  ASSERT_EQ(PySet_Add(set, one), 0);
  EXPECT_EQ(PySet_Size(set), 1);
  ASSERT_EQ(PySet_Add(set, one), 0);
  EXPECT_EQ(PySet_Size(set), 1);
  ASSERT_EQ(PySet_Add(set, two), 0);
  EXPECT_EQ(PySet_Size(set), 2);
}

TEST_F(SetExtensionApiTest, SizeOfNonSetReturnsNegative) {
  PyObjectPtr list(PyList_New(2));

  EXPECT_EQ(PySet_Size(list), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

}  // namespace python
