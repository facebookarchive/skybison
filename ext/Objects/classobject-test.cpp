#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using ClassExtensionApiTest = ExtensionApi;

TEST_F(ClassExtensionApiTest, FunctionReturnsFunction) {
  PyRun_SimpleString(R"(
def foo():
  pass
)");
  PyObjectPtr foo(moduleGet("__main__", "foo"));
  PyObjectPtr bar(PyLong_FromLong(42));
  PyObjectPtr method(PyMethod_New(foo, bar));
  EXPECT_EQ(PyMethod_Function(method), foo);
}

TEST_F(ClassExtensionApiTest, GetFunctionReturnsFunction) {
  PyRun_SimpleString(R"(
def foo():
  pass
)");
  PyObjectPtr foo(moduleGet("__main__", "foo"));
  PyObjectPtr bar(PyLong_FromLong(42));
  PyObjectPtr method(PyMethod_New(foo, bar));
  EXPECT_EQ(PyMethod_GET_FUNCTION(method.get()), foo);
}

TEST_F(ClassExtensionApiTest, NewWithNullSelfRaisesRuntimeError) {
  PyRun_SimpleString(R"(
def foo(x):
  return x
)");
  PyObjectPtr foo(moduleGet("__main__", "foo"));
  PyObject* method = PyMethod_New(foo, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_EQ(method, nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ClassExtensionApiTest, NewWithNoArgsReturnsBoundObject) {
  PyRun_SimpleString(R"(
def foo(x):
  return x
)");
  PyObjectPtr foo(moduleGet("__main__", "foo"));
  PyObjectPtr integer(PyLong_FromLong(123));
  PyObjectPtr method(PyMethod_New(foo, integer));
  PyObjectPtr args(PyTuple_New(0));
  PyObjectPtr result(PyObject_CallObject(method, args));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result, integer);
}

TEST_F(ClassExtensionApiTest, CheckWithNonMethodReturnsFalse) {
  PyObjectPtr pylong(PyLong_FromLong(10));
  EXPECT_EQ(PyMethod_Check(pylong.get()), 0);
}

TEST_F(ClassExtensionApiTest, CheckWithMethodReturnsTrue) {
  PyRun_SimpleString(R"(
def foo(x):
  return x
)");
  PyObjectPtr foo(moduleGet("__main__", "foo"));
  PyObjectPtr integer(PyLong_FromLong(123));
  PyObjectPtr method(PyMethod_New(foo, integer));
  EXPECT_EQ(PyMethod_Check(method.get()), 1);
}

TEST_F(ClassExtensionApiTest, SelfReturnsSelf) {
  PyRun_SimpleString(R"(
def foo():
  pass
)");
  PyObjectPtr foo(moduleGet("__main__", "foo"));
  PyObjectPtr bar(PyLong_FromLong(42));
  PyObjectPtr method(PyMethod_New(foo, bar));
  EXPECT_EQ(PyMethod_Self(method), bar);
}

TEST_F(ClassExtensionApiTest, GetSelfReturnsSelf) {
  PyRun_SimpleString(R"(
def foo():
  pass
)");
  PyObjectPtr foo(moduleGet("__main__", "foo"));
  PyObjectPtr bar(PyLong_FromLong(42));
  PyObjectPtr method(PyMethod_New(foo, bar));
  EXPECT_EQ(PyMethod_GET_SELF(method.get()), bar);
}

}  // namespace testing
}  // namespace py
