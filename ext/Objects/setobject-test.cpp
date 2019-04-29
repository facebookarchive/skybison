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

TEST_F(SetExtensionApiTest, FrozenSetCheckWithSetReturnsFalse) {
  PyObjectPtr set(PySet_New(nullptr));
  EXPECT_FALSE(PyFrozenSet_Check(set));
}

TEST_F(SetExtensionApiTest, FrozenSetCheckWithFrozenSetSubclassReturnsTrue) {
  PyRun_SimpleString(R"(
class C(frozenset):
  pass
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_TRUE(PyFrozenSet_Check(c));
}

TEST_F(SetExtensionApiTest, FrozenSetCheckWithFrozenSetReturnsTrue) {
  PyObjectPtr set(PyFrozenSet_New(nullptr));
  EXPECT_TRUE(PyFrozenSet_Check(set));
}

TEST_F(SetExtensionApiTest, FrozenSetCheckExactWithSetReturnsFalse) {
  PyObjectPtr set(PySet_New(nullptr));
  EXPECT_FALSE(PyFrozenSet_CheckExact(set));
}

TEST_F(SetExtensionApiTest,
       FrozenSetCheckExactWithFrozenSetSubclassReturnsFalse) {
  PyRun_SimpleString(R"(
class C(frozenset):
  pass
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_FALSE(PyFrozenSet_CheckExact(c));
}

TEST_F(SetExtensionApiTest, FrozenSetCheckExactWithFrozenSetReturnsTrue) {
  PyObjectPtr set(PyFrozenSet_New(nullptr));
  EXPECT_TRUE(PyFrozenSet_CheckExact(set));
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

TEST_F(SetExtensionApiTest, FrozenSetNewWithDictCopiesKeys) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr value(PyLong_FromLong(4));
  PyObjectPtr key1(PyLong_FromLong(1));
  PyDict_SetItem(dict, key1, value);
  PyObjectPtr key2(PyLong_FromLong(2));
  PyDict_SetItem(dict, key2, value);
  PyObjectPtr key3(PyLong_FromLong(3));
  PyDict_SetItem(dict, key3, value);

  PyObjectPtr set(PyFrozenSet_New(dict));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PySet_Contains(set, key1), 1);
  EXPECT_EQ(PySet_Contains(set, key2), 1);
  EXPECT_EQ(PySet_Contains(set, key3), 1);
}

TEST_F(SetExtensionApiTest, FrozenSetNewFromSetContainsElementsOfSet) {
  PyObjectPtr set(PySet_New(nullptr));
  PyObjectPtr one(PyLong_FromLong(1));
  ASSERT_EQ(PySet_Add(set, one), 0);
  PyObjectPtr two(PyLong_FromLong(2));
  ASSERT_EQ(PySet_Add(set, two), 0);

  PyObjectPtr set_copy(PyFrozenSet_New(set));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PySet_Contains(set_copy, one), 1);
  EXPECT_EQ(PySet_Contains(set_copy, two), 1);
  EXPECT_EQ(PySet_Size(set_copy), 2);
}

TEST_F(SetExtensionApiTest, FrozenSetNewWithListContainsElementsOfList) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr one(PyLong_FromLong(1));
  PyList_Append(list, one);
  PyObjectPtr two(PyLong_FromLong(2));
  PyList_Append(list, two);

  PyObjectPtr set(PyFrozenSet_New(list));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PySet_Contains(set, one), 1);
  EXPECT_EQ(PySet_Contains(set, two), 1);
  EXPECT_EQ(PySet_Size(set), 2);
}

TEST_F(SetExtensionApiTest, FrozenSetNewWithNonIterableRaisesTypeError) {
  PyObjectPtr num(PyLong_FromLong(1));
  EXPECT_EQ(PyFrozenSet_New(num), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(SetExtensionApiTest, FrozenSetNewWithNullReturnsEmpty) {
  PyObjectPtr set(PyFrozenSet_New(nullptr));
  ASSERT_NE(set, nullptr);
  EXPECT_EQ(PySet_Size(set), 0);
}

TEST_F(SetExtensionApiTest, ContainsWithFrozenSetDoesNotRaiseSystemError) {
  PyObjectPtr set(PyFrozenSet_New(nullptr));
  EXPECT_EQ(PySet_Contains(set, Py_None), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(SetExtensionApiTest, SizeWithFrozenSetDoesNotRaiseSystemError) {
  PyObjectPtr set(PyFrozenSet_New(nullptr));
  EXPECT_EQ(PySet_Size(set), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(SetExtensionApiTest, ClearWithNonSetRaisesSystemError) {
  ASSERT_EQ(PySet_Clear(Py_None), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(SetExtensionApiTest, ClearRemovesAllItems) {
  PyObjectPtr set(PySet_New(nullptr));
  PyObjectPtr one(PyLong_FromLong(1));
  PySet_Add(set, one);
  PyObjectPtr two(PyLong_FromLong(2));
  PySet_Add(set, two);
  PyObjectPtr three(PyLong_FromLong(3));
  PySet_Add(set, three);

  ASSERT_EQ(PySet_Clear(set), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PySet_Size(set), 0);
}

TEST_F(SetExtensionApiTest, PopWithNonSetRaisesSystemError) {
  ASSERT_EQ(PySet_Pop(Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(SetExtensionApiTest, PopWithEmptySetRaisesKeyError) {
  ASSERT_EQ(PySet_Pop(PySet_New(nullptr)), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_KeyError));
}

TEST_F(SetExtensionApiTest, PopWithNonEmptySetRemovesItem) {
  PyObjectPtr set(PySet_New(nullptr));
  PyObjectPtr elt(PyLong_FromLong(5));
  ASSERT_EQ(PySet_Add(set, elt), 0);
  ASSERT_EQ(PySet_Size(set), 1);
  ASSERT_EQ(PySet_Pop(set), elt);
  EXPECT_EQ(PySet_Size(set), 0);
}

TEST_F(SetExtensionApiTest, PopWithSetContainingErrorsRemovesItem) {
  PyObjectPtr set(PySet_New(nullptr));
  PyObjectPtr elt(PyExc_KeyError);
  ASSERT_EQ(PySet_Add(set, elt), 0);
  ASSERT_EQ(PySet_Size(set), 1);
  ASSERT_EQ(PySet_Pop(set), elt);
  EXPECT_EQ(PySet_Size(set), 0);
}

TEST_F(SetExtensionApiTest, DiscardWithNonSetRaisesSystemError) {
  ASSERT_EQ(PySet_Discard(Py_None, Py_None), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(SetExtensionApiTest, DiscardWithSetRemovesItem) {
  PyObjectPtr set(PySet_New(nullptr));
  PyObjectPtr elt(PyLong_FromLong(5));
  ASSERT_EQ(PySet_Add(set, elt), 0);
  ASSERT_EQ(PySet_Size(set), 1);
  ASSERT_EQ(PySet_Discard(set, elt), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PySet_Size(set), 0);
}

}  // namespace python
