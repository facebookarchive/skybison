#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {

using namespace testing;

using ClassExtensionApiTest = ExtensionApi;

TEST_F(ClassExtensionApiTest, ClearFreeListReturnsZeroPyro) {
  EXPECT_EQ(PyMethod_ClearFreeList(), 0);
}

TEST_F(ClassExtensionApiTest, MethodNewWithNullSelfRaisesRuntimeError) {
  PyRun_SimpleString(R"(
def foo(x):
  return x
)");
  PyObjectPtr foo_func(moduleGet("__main__", "foo"));
  PyObject* func = PyMethod_New(foo_func, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_EQ(func, nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ClassExtensionApiTest, MethodCallWithNoArgsReturnsBoundObject) {
  PyRun_SimpleString(R"(
def foo(x):
  return x
)");
  PyObjectPtr foo_func(moduleGet("__main__", "foo"));
  PyObjectPtr integer(PyLong_FromLong(123));
  PyObjectPtr func(PyMethod_New(foo_func, integer));
  PyObjectPtr args(PyTuple_New(0));
  PyObjectPtr result(PyObject_CallObject(func, args));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result, integer);
}

}  // namespace py
