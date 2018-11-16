#include "gtest/gtest.h"

#include "Python.h"
#include "runtime/runtime.h"
#include "runtime/test-utils.h"

namespace python {

TEST(TupleObject, New) {
  Runtime runtime;
  HandleScope scope;

  word length = 5;
  PyObject* pyresult = PyTuple_New(length);
  Handle<Object> result_obj(&scope,
                            ApiHandle::fromPyObject(pyresult)->asObject());
  ASSERT_TRUE(result_obj->isObjectArray());

  Handle<ObjectArray> result(&scope, *result_obj);
  EXPECT_EQ(length, result->length());
}

TEST(TupleObject, Size) {
  Runtime runtime;
  HandleScope scope;

  word length = 5;
  Handle<ObjectArray> tuple(&scope, runtime.newObjectArray(length));
  word pylength = PyTuple_Size(ApiHandle::fromObject(*tuple)->asPyObject());
  EXPECT_EQ(length, pylength);
}

TEST(TupleObject, GetItemFromNonTupleReturnsNull) {
  Runtime runtime;
  HandleScope scope;

  // Get item from non tuple
  Handle<Integer> not_tuple(&scope, runtime.newInteger(7));
  PyObject* pyresult =
      PyTuple_GetItem(ApiHandle::fromObject(*not_tuple)->asPyObject(), 0);
  EXPECT_EQ(nullptr, pyresult);
}

TEST(TupleObject, GetItemOutOfBoundsReturnsMinusOne) {
  Runtime runtime;
  HandleScope scope;

  word pos = 3;
  word length = 5;
  word int_value = 10;
  Handle<ObjectArray> tuple(&scope, runtime.newObjectArray(length));
  Handle<Integer> item(&scope, runtime.newInteger(int_value));
  tuple->atPut(pos, *item);

  // Get item out of bounds
  PyObject* pyresult =
      PyTuple_GetItem(ApiHandle::fromObject(*tuple)->asPyObject(), -1);
  EXPECT_EQ(nullptr, pyresult);
  pyresult = PyTuple_GetItem(ApiHandle::fromObject(*tuple)->asPyObject(), length);
  EXPECT_EQ(nullptr, pyresult);
}

TEST(TupleObject, GetItemReturnsSameItem) {
  Runtime runtime;
  HandleScope scope;

  word pos = 3;
  word length = 5;
  word int_value = 10;
  Handle<ObjectArray> tuple(&scope, runtime.newObjectArray(length));
  Handle<Integer> item(&scope, runtime.newInteger(int_value));
  tuple->atPut(pos, *item);

  // Get item
  PyObject* pyresult =
      PyTuple_GetItem(ApiHandle::fromObject(*tuple)->asPyObject(), pos);
  EXPECT_NE(nullptr, pyresult);
  Handle<Object> result_obj(&scope,
                            ApiHandle::fromPyObject(pyresult)->asObject());
  ASSERT_TRUE(result_obj->isInteger());
}

TEST(TupleObject, GetItemReturnsBorrowedReference) {
  Runtime runtime;
  HandleScope scope;

  word pos = 3;
  word length = 5;
  word int_value = 10;
  Handle<ObjectArray> tuple(&scope, runtime.newObjectArray(length));
  Handle<Integer> item(&scope, runtime.newInteger(int_value));
  tuple->atPut(pos, *item);

  // Verify borrowed handle
  PyObject* pyresult =
      PyTuple_GetItem(ApiHandle::fromObject(*tuple)->asPyObject(), pos);
  Handle<Object> result_obj(&scope,
                            ApiHandle::fromPyObject(pyresult)->asObject());
  ApiHandle* result_handle = ApiHandle::fromObject(*result_obj);
  EXPECT_TRUE(result_handle->isBorrowed());
  Handle<Integer> result(&scope, result_handle->asObject());
  EXPECT_EQ(int_value, result->asWord());
}

TEST(TupleObject, PackZeroReturnsEmptyTuple) {
  Runtime runtime;
  HandleScope scope;

  PyObject* pytuple = PyTuple_Pack(0);
  Handle<Object> result_obj(&scope,
                            ApiHandle::fromPyObject(pytuple)->asObject());
  ASSERT_TRUE(result_obj->isObjectArray());

  Handle<ObjectArray> result(&scope, *result_obj);
  EXPECT_EQ(0, result->length());
}

TEST(TupleObject, PackOneValue) {
  Runtime runtime;
  HandleScope scope;

  word length1 = 1;
  PyObject* pyint = PyLong_FromLong(5);
  PyObject* pytuple1 = PyTuple_Pack(length1, pyint);
  Handle<Object> result_obj1(&scope,
                             ApiHandle::fromPyObject(pytuple1)->asObject());
  ASSERT_TRUE(result_obj1->isObjectArray());

  Handle<ObjectArray> result1(&scope, *result_obj1);
  EXPECT_EQ(length1, result1->length());
  EXPECT_EQ(pyint, ApiHandle::fromObject(result1->at(0))->asPyObject());
}

TEST(TupleObject, PackTwoValues) {
  Runtime runtime;
  HandleScope scope;

  word length2 = 2;
  PyObject* pyint = PyLong_FromLong(5);
  PyObject* pybool = PyBool_FromLong(1);
  PyObject* pytuple2 = PyTuple_Pack(length2, pybool, pyint);
  Handle<Object> result_obj2(&scope,
                             ApiHandle::fromPyObject(pytuple2)->asObject());
  ASSERT_TRUE(result_obj2->isObjectArray());

  Handle<ObjectArray> result2(&scope, *result_obj2);
  EXPECT_EQ(length2, result2->length());
  EXPECT_EQ(pybool, ApiHandle::fromObject(result2->at(0))->asPyObject());
  EXPECT_EQ(pyint, ApiHandle::fromObject(result2->at(1))->asPyObject());
}

}  // namespace python
