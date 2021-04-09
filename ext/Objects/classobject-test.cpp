#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using ClassExtensionApiTest = ExtensionApi;

TEST_F(ClassExtensionApiTest, InstanceMethodNewReturnsInstanceMethod) {
  PyRun_SimpleString(R"(
def foo():
  pass
)");
  PyObjectPtr foo(mainModuleGet("foo"));
  PyObjectPtr method(PyInstanceMethod_New(foo));
  EXPECT_NE(method, nullptr);
  EXPECT_TRUE(PyInstanceMethod_Check(method.get()));
  EXPECT_NE(method, foo);
}

TEST_F(ClassExtensionApiTest, InstanceMethodGETFUNCTIONReturnsFunction) {
  PyRun_SimpleString(R"(
def foo():
  pass
)");
  PyObjectPtr foo(mainModuleGet("foo"));
  PyObjectPtr method(PyInstanceMethod_New(foo));
  EXPECT_NE(method, nullptr);
  EXPECT_EQ(PyInstanceMethod_GET_FUNCTION(method.get()), foo);
}

TEST_F(ClassExtensionApiTest, CallWithInstanceMethodCallsFunction) {
  PyRun_SimpleString(R"(
def foo():
  return 123
)");
  PyObjectPtr foo(mainModuleGet("foo"));
  PyObjectPtr method(PyInstanceMethod_New(foo));
  EXPECT_NE(method, nullptr);
  PyObjectPtr result(PyObject_CallFunctionObjArgs(method, nullptr));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isLongEqualsLong(result, 123));
}

TEST_F(ClassExtensionApiTest,
       InstanceMethodDunderGetWithNoneObjectReturnsFunction) {
  PyRun_SimpleString(R"(
def foo():
  return 123
)");
  PyObjectPtr foo(mainModuleGet("foo"));
  PyObjectPtr method(PyInstanceMethod_New(foo));
  EXPECT_NE(method, nullptr);
  PyObjectPtr none((Borrowed(Py_None)));
  PyObjectPtr non_none(PyLong_FromLong(123));
  PyObjectPtr result(
      PyObject_CallMethod(method, "__get__", "OO", none.get(), non_none.get()));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_NE(result, nullptr);
  EXPECT_EQ(result, foo);
}

TEST_F(ClassExtensionApiTest,
       InstanceMethodDunderGetWithNonNoneObjectReturnsMethod) {
  PyRun_SimpleString(R"(
def foo():
  return 123
)");
  PyObjectPtr foo(mainModuleGet("foo"));
  PyObjectPtr method(PyInstanceMethod_New(foo));
  EXPECT_NE(method, nullptr);
  PyObjectPtr instance(PyLong_FromLong(456));
  PyObjectPtr none((Borrowed(Py_None)));
  PyObjectPtr result(
      PyObject_CallMethod(method, "__get__", "OO", instance.get(), none.get()));
  EXPECT_NE(result, nullptr);
  EXPECT_TRUE(PyMethod_Check(result.get()));
}

TEST_F(ClassExtensionApiTest, FunctionReturnsFunction) {
  PyRun_SimpleString(R"(
def foo():
  pass
)");
  PyObjectPtr foo(mainModuleGet("foo"));
  PyObjectPtr bar(PyLong_FromLong(42));
  PyObjectPtr method(PyMethod_New(foo, bar));
  EXPECT_EQ(PyMethod_Function(method), foo);
}

TEST_F(ClassExtensionApiTest, GetFunctionReturnsFunction) {
  PyRun_SimpleString(R"(
def foo():
  pass
)");
  PyObjectPtr foo(mainModuleGet("foo"));
  PyObjectPtr bar(PyLong_FromLong(42));
  PyObjectPtr method(PyMethod_New(foo, bar));
  EXPECT_EQ(PyMethod_GET_FUNCTION(method.get()), foo);
}

TEST_F(ClassExtensionApiTest, NewWithNullSelfRaisesRuntimeError) {
  PyRun_SimpleString(R"(
def foo(x):
  return x
)");
  PyObjectPtr foo(mainModuleGet("foo"));
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
  PyObjectPtr foo(mainModuleGet("foo"));
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
  PyObjectPtr foo(mainModuleGet("foo"));
  PyObjectPtr integer(PyLong_FromLong(123));
  PyObjectPtr method(PyMethod_New(foo, integer));
  EXPECT_EQ(PyMethod_Check(method.get()), 1);
}

TEST_F(ClassExtensionApiTest, SelfReturnsSelf) {
  PyRun_SimpleString(R"(
def foo():
  pass
)");
  PyObjectPtr foo(mainModuleGet("foo"));
  PyObjectPtr bar(PyLong_FromLong(42));
  PyObjectPtr method(PyMethod_New(foo, bar));
  EXPECT_EQ(PyMethod_Self(method), bar);
}

TEST_F(ClassExtensionApiTest, GetSelfReturnsSelf) {
  PyRun_SimpleString(R"(
def foo():
  pass
)");
  PyObjectPtr foo(mainModuleGet("foo"));
  PyObjectPtr bar(PyLong_FromLong(42));
  PyObjectPtr method(PyMethod_New(foo, bar));
  EXPECT_EQ(PyMethod_GET_SELF(method.get()), bar);
}

}  // namespace testing
}  // namespace py
