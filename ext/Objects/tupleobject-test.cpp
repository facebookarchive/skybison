#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using TupleExtensionApiTest = ExtensionApi;

TEST_F(TupleExtensionApiTest, NewAndSize) {
  Py_ssize_t length = 5;
  PyObject* pytuple = PyTuple_New(length);
  Py_ssize_t result = PyTuple_Size(pytuple);
  EXPECT_EQ(result, length);
}

TEST_F(TupleExtensionApiTest, SetItemWithNonTupleReturnsNegative) {
  int result = PyTuple_SetItem(Py_True, 0, Py_None);
  EXPECT_EQ(result, -1);

  const char* expected_message = "bad argument to internal function";
  EXPECT_TRUE(testing::exceptionValueMatches(expected_message));
}

TEST_F(TupleExtensionApiTest, SetItemWithInvalidIndexReturnsNegative) {
  PyObject* pytuple = PyTuple_New(1);
  int result = PyTuple_SetItem(pytuple, 2, Py_None);
  EXPECT_EQ(result, -1);

  const char* expected_message = "tuple assignment index out of range";
  EXPECT_TRUE(testing::exceptionValueMatches(expected_message));
}

TEST_F(TupleExtensionApiTest, SetItemReturnsZero) {
  PyObject* pytuple = PyTuple_New(1);
  int result = PyTuple_SetItem(pytuple, 0, Py_None);
  EXPECT_EQ(result, 0);
}

TEST_F(TupleExtensionApiTest, GetItemFromNonTupleReturnsNull) {
  PyObject* pytuple = PyTuple_GetItem(Py_None, 0);
  EXPECT_EQ(nullptr, pytuple);
}

TEST_F(TupleExtensionApiTest, GetItemOutOfBoundsReturnsMinusOne) {
  Py_ssize_t length = 5;
  PyObject* pytuple = PyTuple_New(length);

  // Get item out of bounds
  PyObject* pyresult = PyTuple_GetItem(pytuple, -1);
  EXPECT_EQ(nullptr, pyresult);

  pyresult = PyTuple_GetItem(pytuple, length);
  EXPECT_EQ(nullptr, pyresult);
}

TEST_F(TupleExtensionApiTest, GetItemReturnsSameItem) {
  Py_ssize_t length = 5;
  Py_ssize_t pos = 3;
  Py_ssize_t int_value = 10;
  PyObject* pytuple = PyTuple_New(length);
  PyObject* pyitem = PyLong_FromLong(int_value);
  ASSERT_EQ(PyTuple_SetItem(pytuple, pos, pyitem), 0);

  // Get item
  PyObject* pyresult = PyTuple_GetItem(pytuple, pos);
  EXPECT_NE(nullptr, pyresult);
  EXPECT_EQ(PyLong_AsLong(pyresult), int_value);
}

TEST_F(TupleExtensionApiTest, GetItemReturnsBorrowedReference) {
  Py_ssize_t length = 5;
  Py_ssize_t pos = 3;
  PyObject* pytuple = PyTuple_New(length);
  PyObject* pyitem = testing::createUniqueObject();
  EXPECT_EQ(Py_REFCNT(pyitem), 1);
  ASSERT_EQ(PyTuple_SetItem(pytuple, pos, pyitem), 0);
  // PyTuple_SetItem "steals" a reference to the item.  Verify that the
  // reference count did not change.
  EXPECT_EQ(Py_REFCNT(pyitem), 1);

  PyObject* pyresult = PyTuple_GetItem(pytuple, pos);
  // PyTuple_GetItem "borrows" a reference for the return value.  Verify the
  // reference count did not change.
  EXPECT_EQ(Py_REFCNT(pyresult), 1);
}

TEST_F(TupleExtensionApiTest, PackZeroReturnsEmptyTuple) {
  PyObject* pytuple = PyTuple_Pack(0);
  Py_ssize_t result = PyTuple_Size(pytuple);
  EXPECT_EQ(result, 0);
}

TEST_F(TupleExtensionApiTest, PackOneValue) {
  Py_ssize_t length = 1;
  const int int_value = 5;
  PyObject* pylong = PyLong_FromLong(int_value);
  PyObject* pytuple = PyTuple_Pack(length, pylong);

  PyObject* pyresult = PyTuple_GetItem(pytuple, 0);
  EXPECT_EQ(PyLong_AsLong(pyresult), int_value);
}

TEST_F(TupleExtensionApiTest, PackTwoValues) {
  Py_ssize_t length = 2;
  const int int_value1 = 5;
  const int int_value2 = 12;
  PyObject* pylong1 = PyLong_FromLong(int_value1);
  PyObject* pylong2 = PyLong_FromLong(int_value2);
  PyObject* pytuple = PyTuple_Pack(length, pylong1, pylong2);

  PyObject* pyresult1 = PyTuple_GetItem(pytuple, 0);
  PyObject* pyresult2 = PyTuple_GetItem(pytuple, 1);
  EXPECT_EQ(PyLong_AsLong(pyresult1), int_value1);
  EXPECT_EQ(PyLong_AsLong(pyresult2), int_value2);
}

TEST_F(TupleExtensionApiTest, ClearFreeListReturnsZero) {
  EXPECT_EQ(PyTuple_ClearFreeList(), 0);
}

}  // namespace python
