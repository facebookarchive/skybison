#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {

using namespace testing;

using ListExtensionApiTest = ExtensionApi;

TEST_F(ListExtensionApiTest, NewWithBadLengthReturnsNull) {
  Py_ssize_t invalid_length = -1;
  PyObject* pyresult = PyList_New(invalid_length);
  EXPECT_EQ(pyresult, nullptr);
}

TEST_F(ListExtensionApiTest, NewReturnsEmptyList) {
  Py_ssize_t length = 0;
  PyObject* pyresult = PyList_New(length);
  EXPECT_TRUE(PyList_CheckExact(pyresult));
  EXPECT_EQ(PyList_Size(pyresult), length);

  Py_DECREF(pyresult);
}

TEST_F(ListExtensionApiTest, NewReturnsList) {
  Py_ssize_t length = 5;
  PyObject* pyresult = PyList_New(length);
  EXPECT_TRUE(PyList_CheckExact(pyresult));
  EXPECT_EQ(PyList_Size(pyresult), length);

  Py_DECREF(pyresult);
}

TEST_F(ListExtensionApiTest, AppendToNonListReturnsNegative) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr pylong(PyLong_FromLong(10));
  int result = PyList_Append(dict, pylong);
  EXPECT_EQ(result, -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ListExtensionApiTest, AppendWithNullValueReturnsNegative) {
  PyObjectPtr list(PyList_New(0));
  int result = PyList_Append(list, nullptr);
  EXPECT_EQ(result, -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ListExtensionApiTest, AppendReturnsZero) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr pylong(PyLong_FromLong(10));
  int result = PyList_Append(list, pylong);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ListExtensionApiTest, ClearFreeListReturnsZeroPyro) {
  EXPECT_EQ(PyList_ClearFreeList(), 0);
}

TEST_F(ListExtensionApiTest, SizeIncreasesAfterAppend) {
  Py_ssize_t length = 4;
  PyObject* list = PyList_New(length);
  EXPECT_TRUE(PyList_CheckExact(list));
  EXPECT_EQ(PyList_Size(list), length);

  PyObject* item = PyLong_FromLong(1);
  EXPECT_EQ(PyList_Append(list, item), 0);
  EXPECT_EQ(PyList_Size(list), length + 1);

  Py_DECREF(item);
  Py_DECREF(list);
}

TEST_F(ListExtensionApiTest, SizeWithNonListReturnsNegative) {
  PyObject* dict = PyDict_New();
  EXPECT_EQ(PyList_Size(dict), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));

  Py_DECREF(dict);
}

TEST_F(ListExtensionApiTest, AsTupleWithNullRaisesSystemError) {
  ASSERT_EQ(PyList_AsTuple(nullptr), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ListExtensionApiTest, AsTupleWithNonListRaisesSystemError) {
  ASSERT_EQ(PyList_AsTuple(Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ListExtensionApiTest, AsTupleWithListReturnsAllElementsFromList) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));
  PyObjectPtr three(PyLong_FromLong(3));
  PyList_Append(list, one);
  PyList_Append(list, two);
  PyList_Append(list, three);

  PyObjectPtr tuple(PyList_AsTuple(list));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyTuple_CheckExact(tuple));
  ASSERT_EQ(PyTuple_Size(tuple), 3);
  EXPECT_EQ(PyTuple_GetItem(tuple, 0), one);
  EXPECT_EQ(PyTuple_GetItem(tuple, 1), two);
  EXPECT_EQ(PyTuple_GetItem(tuple, 2), three);
}

TEST_F(ListExtensionApiTest, GetItemWithNonListReturnsNull) {
  ASSERT_EQ(PyList_GetItem(Py_None, 0), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ListExtensionApiTest, GetItemWithBadIndexRaisesIndexError) {
  Py_ssize_t size = 0;
  PyObjectPtr list(PyList_New(size));
  ASSERT_EQ(PyList_GetItem(list, size + 1), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_IndexError));
}

TEST_F(ListExtensionApiTest, GetItemWithListReturnsElementAtIndex) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));
  PyObjectPtr three(PyLong_FromLong(3));
  PyList_Append(list, one);
  PyList_Append(list, two);
  PyList_Append(list, three);

  EXPECT_EQ(PyList_GetItem(list, 0), one);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyList_GetItem(list, 1), two);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyList_GetItem(list, 2), three);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ListExtensionApiTest, SetItemWithNonListReturnsNegativeOne) {
  ASSERT_EQ(PyList_SetItem(Py_None, 0, nullptr), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ListExtensionApiTest, SetItemWithBadIndexRaisesIndexError) {
  Py_ssize_t size = 0;
  PyObjectPtr list(PyList_New(size));
  ASSERT_EQ(PyList_SetItem(list, size + 1, nullptr), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_IndexError));
}

TEST_F(ListExtensionApiTest, SetItemWithListSetsItemAtIndex) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));
  PyObjectPtr three(PyLong_FromLong(3));
  PyList_Append(list, one);
  PyList_Append(list, two);
  PyList_Append(list, three);

  Py_ssize_t idx = 2;
  PyObjectPtr four(PyLong_FromLong(4));
  Py_INCREF(four);
  EXPECT_EQ(PyList_SetItem(list, idx, four), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyList_GetItem(list, idx), four);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ListExtensionApiTest, SETITEMWithListSetsItemAtIndex) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));
  PyObjectPtr three(PyLong_FromLong(3));
  PyList_Append(list, one);
  PyList_Append(list, two);
  PyList_Append(list, three);

  // Replace three with four
  Py_ssize_t three_refcnt = Py_REFCNT(three.get());
  Py_ssize_t idx = 2;
  PyObjectPtr four(PyLong_FromLong(4));
  Py_INCREF(four);  // keep an extra reference for checking below SetItem
  PyList_SET_ITEM(list.get(), idx, four);
  EXPECT_EQ(Py_REFCNT(three.get()), three_refcnt);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyList_GetItem(list, idx), four);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ListExtensionApiTest, SetSliceOnNonListRaisesSystemError) {
  PyObjectPtr rhs(PyList_New(0));
  ASSERT_EQ(PyList_SetSlice(Py_None, 0, 0, rhs), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ListExtensionApiTest, SetSliceWithNegativeLowStartsAtZero) {
  PyObjectPtr lhs(PyList_New(0));
  PyObjectPtr zero(PyLong_FromLong(0));
  PyList_Append(lhs, zero);
  PyObjectPtr one(PyLong_FromLong(1));
  PyList_Append(lhs, one);
  PyObjectPtr two(PyLong_FromLong(2));
  PyList_Append(lhs, two);

  PyObjectPtr rhs(PyList_New(0));
  PyObjectPtr five(PyLong_FromLong(5));
  PyList_Append(rhs, five);

  ASSERT_EQ(PyList_SetSlice(lhs, -1, 1, rhs), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyList_Size(lhs), 3);
  EXPECT_EQ(PyList_GetItem(lhs, 0), five);
  EXPECT_EQ(PyList_GetItem(lhs, 1), one);
  EXPECT_EQ(PyList_GetItem(lhs, 2), two);
}

TEST_F(ListExtensionApiTest, SetSliceWithNullItemsDeletesSlice) {
  PyObjectPtr lhs(PyList_New(0));
  PyObjectPtr zero(PyLong_FromLong(0));
  PyList_Append(lhs, zero);
  PyObjectPtr one(PyLong_FromLong(1));
  PyList_Append(lhs, one);
  PyObjectPtr two(PyLong_FromLong(2));
  PyList_Append(lhs, two);

  ASSERT_EQ(PyList_SetSlice(lhs, 0, 1, nullptr), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyList_Size(lhs), 2);
  EXPECT_EQ(PyList_GetItem(lhs, 0), one);
  EXPECT_EQ(PyList_GetItem(lhs, 1), two);
}

TEST_F(ListExtensionApiTest, GetSliceOnNonListRaisesSystemError) {
  ASSERT_EQ(PyList_GetSlice(Py_None, 0, 0), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ListExtensionApiTest, GetSliceOnEmptyListReturnsEmptyList) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr result(PyList_GetSlice(list, 0, 0));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyList_Size(result), 0);
}

TEST_F(ListExtensionApiTest, GetSliceWithNegativeOutOfBoundsLowStartsAtZero) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr zero(PyLong_FromLong(0));
  PyList_Append(list, zero);
  PyObjectPtr one(PyLong_FromLong(1));
  PyList_Append(list, one);
  PyObjectPtr two(PyLong_FromLong(2));
  PyList_Append(list, two);

  PyObjectPtr result(PyList_GetSlice(list, -5, 3));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyList_CheckExact(result));
  EXPECT_EQ(PyList_Size(result), 3);
  EXPECT_EQ(PyList_GetItem(result, 0), zero);
  EXPECT_EQ(PyList_GetItem(result, 1), one);
  EXPECT_EQ(PyList_GetItem(result, 2), two);
}

TEST_F(ListExtensionApiTest, GetSliceWithPositiveOutOfBoundsLowStartsAtLength) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr zero(PyLong_FromLong(0));
  PyList_Append(list, zero);
  PyObjectPtr one(PyLong_FromLong(1));
  PyList_Append(list, one);
  PyObjectPtr two(PyLong_FromLong(2));
  PyList_Append(list, two);

  PyObjectPtr result(PyList_GetSlice(list, 15, 3));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyList_CheckExact(result));
  EXPECT_EQ(PyList_Size(result), 0);
}

TEST_F(ListExtensionApiTest, GetSliceOutOfBoundsHighStartsAtLow) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr zero(PyLong_FromLong(0));
  PyList_Append(list, zero);
  PyObjectPtr one(PyLong_FromLong(1));
  PyList_Append(list, one);
  PyObjectPtr two(PyLong_FromLong(2));
  PyList_Append(list, two);

  PyObjectPtr result(PyList_GetSlice(list, 5, 0));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyList_CheckExact(result));
  EXPECT_EQ(PyList_Size(result), 0);
}

TEST_F(ListExtensionApiTest, GetSliceWithOutOfBoundsHighEndsAtLength) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr zero(PyLong_FromLong(0));
  PyList_Append(list, zero);
  PyObjectPtr one(PyLong_FromLong(1));
  PyList_Append(list, one);
  PyObjectPtr two(PyLong_FromLong(2));
  PyList_Append(list, two);

  PyObjectPtr result(PyList_GetSlice(list, 0, 20));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyList_CheckExact(result));
  EXPECT_EQ(PyList_Size(result), 3);
  EXPECT_EQ(PyList_GetItem(result, 0), zero);
  EXPECT_EQ(PyList_GetItem(result, 1), one);
  EXPECT_EQ(PyList_GetItem(result, 2), two);
}

TEST_F(ListExtensionApiTest, InsertWithNonListRaisesSystemError) {
  ASSERT_EQ(PyList_Insert(Py_None, 0, Py_None), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ListExtensionApiTest, InsertWithNullItemRaisesSystemError) {
  PyObjectPtr list(PyList_New(0));
  ASSERT_EQ(PyList_Insert(list, 0, nullptr), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ListExtensionApiTest, InsertIncreasesSizeByOne) {
  Py_ssize_t num_items = 0;
  PyObjectPtr list(PyList_New(num_items));
  PyObjectPtr val(PyLong_FromLong(666));
  ASSERT_EQ(PyList_Insert(list, 0, val), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyList_Size(list), num_items + 1);
  EXPECT_EQ(PyList_GetItem(list, 0), val);
}

TEST_F(ListExtensionApiTest, InsertIntoListAtFrontShiftsItems) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr one(PyLong_FromLong(1));
  PyList_Append(list, one);
  PyObjectPtr two(PyLong_FromLong(2));
  PyList_Append(list, two);

  PyObjectPtr val(PyLong_FromLong(666));
  ASSERT_EQ(PyList_Insert(list, 0, val), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_EQ(PyList_Size(list), 3);
  EXPECT_EQ(PyList_GetItem(list, 0), val);
  EXPECT_EQ(PyList_GetItem(list, 1), one);
  EXPECT_EQ(PyList_GetItem(list, 2), two);
}

TEST_F(ListExtensionApiTest, InsertIntoListPastRearInsertsAtEnd) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr one(PyLong_FromLong(1));
  PyList_Append(list, one);
  PyObjectPtr two(PyLong_FromLong(2));
  PyList_Append(list, two);

  PyObjectPtr val(PyLong_FromLong(666));
  ASSERT_EQ(PyList_Insert(list, 100, val), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyList_GetItem(list, 0), one);
  EXPECT_EQ(PyList_GetItem(list, 1), two);
  ASSERT_EQ(PyList_GetItem(list, 2), val);
}

TEST_F(ListExtensionApiTest, InsertIntoListNegativeInsertsIndexingFromEnd) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr one(PyLong_FromLong(1));
  PyList_Append(list, one);
  PyObjectPtr two(PyLong_FromLong(2));
  PyList_Append(list, two);

  PyObjectPtr val(PyLong_FromLong(666));
  ASSERT_EQ(PyList_Insert(list, -1, val), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyList_GetItem(list, 0), one);
  EXPECT_EQ(PyList_GetItem(list, 1), val);
  ASSERT_EQ(PyList_GetItem(list, 2), two);
}

TEST_F(ListExtensionApiTest, InsertIntoListWayNegativeInsertsAtBeginning) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr one(PyLong_FromLong(1));
  PyList_Append(list, one);
  PyObjectPtr two(PyLong_FromLong(2));
  PyList_Append(list, two);

  PyObjectPtr val(PyLong_FromLong(666));
  ASSERT_EQ(PyList_Insert(list, -100, val), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyList_GetItem(list, 0), val);
  EXPECT_EQ(PyList_GetItem(list, 1), one);
  ASSERT_EQ(PyList_GetItem(list, 2), two);
}

TEST_F(ListExtensionApiTest, ReverseWithNullRaisesSystemError) {
  ASSERT_EQ(PyList_Reverse(nullptr), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ListExtensionApiTest, ReverseWithNonListRaisesSystemError) {
  ASSERT_EQ(PyList_Reverse(Py_None), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ListExtensionApiTest, ReverseWithZeroLengthListSucceeds) {
  PyObjectPtr list(PyList_New(0));
  ASSERT_EQ(PyList_Reverse(list), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ListExtensionApiTest, ReverseWithNonZeroLengthListSucceeds) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr val0(PyLong_FromLong(0));
  PyList_Append(list, val0);
  PyObjectPtr val1(PyLong_FromLong(1));
  PyList_Append(list, val1);
  PyObjectPtr val2(PyLong_FromLong(2));
  PyList_Append(list, val2);
  PyObjectPtr val3(PyLong_FromLong(3));
  PyList_Append(list, val3);
  PyObjectPtr val4(PyLong_FromLong(4));
  PyList_Append(list, val4);

  ASSERT_EQ(PyList_Reverse(list), 0);
  ASSERT_EQ(PyList_Size(list), 5);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyList_GetItem(list, 0), val4);
  EXPECT_EQ(PyList_GetItem(list, 1), val3);
  EXPECT_EQ(PyList_GetItem(list, 2), val2);
  EXPECT_EQ(PyList_GetItem(list, 3), val1);
  EXPECT_EQ(PyList_GetItem(list, 4), val0);
}

TEST_F(ListExtensionApiTest, SortWithNullListRaisesSystemError) {
  ASSERT_EQ(PyList_Sort(nullptr), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ListExtensionApiTest, SortWithNonListRaisesSystemError) {
  ASSERT_EQ(PyList_Sort(Py_None), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ListExtensionApiTest, SortSortsList) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr five(PyLong_FromLong(5));
  PyList_Append(list, five);
  PyObjectPtr four(PyLong_FromLong(4));
  PyList_Append(list, four);
  PyObjectPtr three(PyLong_FromLong(3));
  PyList_Append(list, three);
  PyObjectPtr two(PyLong_FromLong(2));
  PyList_Append(list, two);
  PyObjectPtr one(PyLong_FromLong(1));
  PyList_Append(list, one);
  ASSERT_EQ(PyList_Size(list), 5);
  ASSERT_EQ(PyList_Sort(list), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyList_GetItem(list, 0), one);
  EXPECT_EQ(PyList_GetItem(list, 1), two);
  EXPECT_EQ(PyList_GetItem(list, 2), three);
  EXPECT_EQ(PyList_GetItem(list, 3), four);
  EXPECT_EQ(PyList_GetItem(list, 4), five);
}

TEST_F(ListExtensionApiTest, SortWithNonComparableElementsRaisesTypeError) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr three(PyLong_FromLong(3));
  PyList_Append(list, three);
  PyObjectPtr two(PyLong_FromLong(2));
  PyList_Append(list, two);
  PyObjectPtr one(PyLong_FromLong(1));
  PyList_Append(list, one);
  PyObjectPtr bar(PyUnicode_FromString("bar"));
  PyList_Append(list, bar);
  ASSERT_EQ(PyList_Size(list), 4);
  ASSERT_EQ(PyList_Sort(list), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

}  // namespace py
