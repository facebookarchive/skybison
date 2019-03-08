#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using WeakRefExtensionApiTest = ExtensionApi;

TEST_F(WeakRefExtensionApiTest, NewWeakRefWithCallableReturnsWeakRef) {
  PyRun_SimpleString(R"(
class C:
  pass
obj = C()
def foo():
  pass
)");
  PyObjectPtr obj(moduleGet("__main__", "obj"));
  PyObjectPtr foo(moduleGet("__main__", "foo"));
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
  PyObjectPtr obj(moduleGet("__main__", "obj"));
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
  PyObjectPtr obj(moduleGet("__main__", "obj"));
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
  PyObjectPtr obj(moduleGet("__main__", "obj"));
  PyObjectPtr ref(PyWeakref_NewRef(obj, nullptr));
  PyObject* referent = PyWeakref_GetObject(ref);
  EXPECT_EQ(referent, obj);
}

}  // namespace python
