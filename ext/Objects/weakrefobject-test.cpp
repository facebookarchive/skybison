#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using WeakRefExtensionApiTest = ExtensionApi;

TEST_F(WeakRefExtensionApiTest, NewProxyWithCallbackReturnsProxy) {
  PyRun_SimpleString(R"(
class C:
  def bar(self):
    return "C.bar"

def foo():
  pass

obj = C()
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  PyObjectPtr foo(mainModuleGet("foo"));
  PyObjectPtr proxy(PyWeakref_NewProxy(obj, foo));
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  PyObjectPtr result(PyObject_CallMethod(proxy, "bar", nullptr));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "C.bar"));
}

TEST_F(WeakRefExtensionApiTest, NewProxyWithNullCallbackReturnsProxy) {
  PyRun_SimpleString(R"(
class C:
  def bar(self):
    return "C.bar"

obj = C()
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  PyObjectPtr proxy(PyWeakref_NewProxy(obj, nullptr));
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  PyObjectPtr result(PyObject_CallMethod(proxy, "bar", nullptr));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "C.bar"));
}

TEST_F(WeakRefExtensionApiTest, NewWeakRefWithCallbackReturnsWeakRef) {
  PyRun_SimpleString(R"(
class C:
  pass
obj = C()
def foo():
  pass
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  PyObjectPtr foo(mainModuleGet("foo"));
  PyObjectPtr ref(PyWeakref_NewRef(obj, foo));
  EXPECT_TRUE(PyWeakref_Check(ref));
}

TEST_F(WeakRefExtensionApiTest, NewRefWithNullCallbackReturnsWeakRef) {
  PyRun_SimpleString(R"(
class C:
  pass
obj = C()
def foo():
  pass
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  PyObjectPtr ref(PyWeakref_NewRef(obj, nullptr));
  EXPECT_TRUE(PyWeakref_Check(ref));
}

TEST_F(WeakRefExtensionApiTest, GetObjectWithNullRaisesSystemError) {
  EXPECT_EQ(PyWeakref_GetObject(nullptr), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(WeakRefExtensionApiTest, GetObjectWithNonWeakrefRaisesSystemError) {
  PyRun_SimpleString(R"(
class C:
  pass
obj = C()
def foo():
  pass
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  EXPECT_EQ(PyWeakref_GetObject(obj), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(WeakRefExtensionApiTest, GetObjectReturnsReferent) {
  PyRun_SimpleString(R"(
class C:
  pass
obj = C()
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  PyObjectPtr ref(PyWeakref_NewRef(obj, nullptr));
  PyObject* referent = PyWeakref_GetObject(ref);
  EXPECT_EQ(referent, obj);
}

}  // namespace testing
}  // namespace py
