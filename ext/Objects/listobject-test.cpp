#include "gtest/gtest.h"

#include "Python.h"
#include "runtime/runtime.h"
#include "runtime/test-utils.h"

namespace python {

TEST(ExtensionTest, ListObjectInalidNew) {
  Py_ssize_t invalid_length = -1;
  PyObject* pyresult = PyList_New(invalid_length);
  EXPECT_EQ(pyresult, nullptr);
}

TEST(ExtensionTest, ListObjectValidNew) {
  Runtime runtime;
  HandleScope scope;

  Py_ssize_t length = 5;
  PyObject* pyresult = PyList_New(length);
  Handle<Object> result_obj(
      &scope, ApiHandle::fromPyObject(pyresult)->asObject());
  ASSERT_TRUE(result_obj->isList());
  Handle<List> result(&scope, *result_obj);
  EXPECT_EQ(length, result->capacity());
}

TEST(ExtensionTest, ListObjectZeroNew) {
  Runtime runtime;
  HandleScope scope;

  Py_ssize_t length = 0;
  PyObject* pyresult = PyList_New(length);
  Handle<Object> result_obj(
      &scope, ApiHandle::fromPyObject(pyresult)->asObject());
  ASSERT_TRUE(result_obj->isList());
  Handle<List> result(&scope, *result_obj);
  EXPECT_EQ(length, result->capacity());
}

} // namespace python
