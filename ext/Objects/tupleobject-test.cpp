#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using TupleExtensionApiTest = ExtensionApi;

TEST_F(TupleExtensionApiTest, CheckWithTupleSubclassReturnsTrue) {
  PyRun_SimpleString(R"(
class Foo(tuple): pass
obj = Foo((1, 2));
)");
  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_TRUE(PyTuple_Check(obj));
}

TEST_F(TupleExtensionApiTest, CheckExactWithTupleSubclassReturnsFalse) {
  PyRun_SimpleString(R"(
class Foo(tuple): pass
obj = Foo((1, 2));
)");
  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_FALSE(PyTuple_CheckExact(obj));
}

TEST_F(TupleExtensionApiTest, NewAndSize) {
  Py_ssize_t length = 5;
  PyObject* pytuple = PyTuple_New(length);
  Py_ssize_t result = PyTuple_Size(pytuple);
  EXPECT_EQ(result, length);
}

TEST_F(TupleExtensionApiTest, SetItemWithNonTupleReturnsNegative) {
  int result = PyTuple_SetItem(Py_True, 0, Py_None);
  EXPECT_EQ(result, -1);

  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(TupleExtensionApiTest, SetItemWithInvalidIndexReturnsNegative) {
  PyObject* pytuple = PyTuple_New(1);
  int result = PyTuple_SetItem(pytuple, 2, Py_None);
  EXPECT_EQ(result, -1);

  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_IndexError));
}

TEST_F(TupleExtensionApiTest, SetItemReturnsZero) {
  PyObject* pytuple = PyTuple_New(1);
  int result = PyTuple_SetItem(pytuple, 0, Py_None);
  EXPECT_EQ(result, 0);
}

TEST_F(TupleExtensionApiTest, SetItemWithTupleSubclassRaisesSystemError) {
  PyRun_SimpleString(R"(
class Foo(tuple): pass
obj = Foo((1, 2));
)");
  PyObjectPtr obj(moduleGet("__main__", "obj"));
  ASSERT_EQ(PyTuple_SetItem(obj, 0, Py_None), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
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
  long refcnt = Py_REFCNT(pyitem);
  ASSERT_EQ(PyTuple_SetItem(pytuple, pos, pyitem), 0);
  // PyTuple_SetItem "steals" a reference to the item.  Verify that the
  // reference count did not change.
  EXPECT_EQ(Py_REFCNT(pyitem), refcnt);

  PyObject* pyresult = PyTuple_GetItem(pytuple, pos);
  // PyTuple_GetItem "borrows" a reference for the return value.  Verify the
  // reference count did not change.
  EXPECT_EQ(Py_REFCNT(pyresult), refcnt);
}

TEST_F(TupleExtensionApiTest, GetItemWithTupleSubclassReturnsValue) {
  PyRun_SimpleString(R"(
class Foo(tuple): pass
obj = Foo((1, 2));
)");
  PyObjectPtr obj(moduleGet("__main__", "obj"));
  PyObjectPtr first(PyTuple_GetItem(obj, 0));
  PyObjectPtr second(PyTuple_GetItem(obj, 1));
  EXPECT_EQ(PyLong_AsLong(first), 1);
  EXPECT_EQ(PyLong_AsLong(second), 2);
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

TEST_F(TupleExtensionApiTest, ClearFreeListReturnsZeroPyro) {
  EXPECT_EQ(PyTuple_ClearFreeList(), 0);
}

}  // namespace python
