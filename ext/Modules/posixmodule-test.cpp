#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using PosixExtensionApiTest = ExtensionApi;

TEST_F(PosixExtensionApiTest, FsPathWithNonPathReturnsNull) {
  PyObjectPtr result(PyOS_FSPath(Py_None));
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
  EXPECT_EQ(result, nullptr);
}

TEST_F(PosixExtensionApiTest, FsPathWithStrReturnsSameStr) {
  PyObjectPtr str(PyUnicode_FromString("foo"));
  PyObjectPtr result(PyOS_FSPath(str));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, str);
}

TEST_F(PosixExtensionApiTest, FsPathWithBytesReturnsSameBytes) {
  PyObjectPtr bytes(PyBytes_FromString("foo"));
  PyObjectPtr result(PyOS_FSPath(bytes));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, bytes);
}

TEST_F(PosixExtensionApiTest, FsPathWithNonCallableFsPathRaisesTypeError) {
  PyRun_SimpleString(R"(
class Foo():
  __fspath__ = None
foo = Foo()
  )");
  PyObjectPtr foo(moduleGet("__main__", "foo"));
  PyObjectPtr result(PyOS_FSPath(foo));
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
  EXPECT_EQ(result, nullptr);
}

TEST_F(PosixExtensionApiTest, FsPathWithNonStrOrBytesResultRaisesTypeError) {
  PyRun_SimpleString(R"(
class Foo():
  def __fspath__(self):
    return 1
foo = Foo()
  )");
  PyObjectPtr foo(moduleGet("__main__", "foo"));
  PyObjectPtr result(PyOS_FSPath(foo));
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
  EXPECT_EQ(result, nullptr);
}

TEST_F(PosixExtensionApiTest, FsPathReturnsPath) {
  PyRun_SimpleString(R"(
class Foo():
  def __fspath__(self):
    return "/some/path"
foo = Foo()
  )");
  PyObjectPtr foo(moduleGet("__main__", "foo"));
  PyObjectPtr result(PyOS_FSPath(foo));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "/some/path"));
}

}  // namespace testing
}  // namespace py
