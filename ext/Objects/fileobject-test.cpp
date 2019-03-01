#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using FileObjectExtensionApiTest = ExtensionApi;

using namespace testing;

TEST_F(FileObjectExtensionApiTest, AsFileDescriptorWithSmallIntReturnsInt) {
  PyObjectPtr obj(PyLong_FromLong(5));
  EXPECT_EQ(PyObject_AsFileDescriptor(obj), 5);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(FileObjectExtensionApiTest,
       AsFileDescriptorWithLargeIntRaisesOverflowError) {
  PyRun_SimpleString(R"(
big = 1 << 70
)");
  PyObjectPtr big(moduleGet("__main__", "big"));
  EXPECT_EQ(PyObject_AsFileDescriptor(big), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_OverflowError));
}

TEST_F(FileObjectExtensionApiTest, AsFileDescriptorCallsFilenoMethod) {
  PyRun_SimpleString(R"(
class C:
  def fileno(self):
    return 7
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyObject_AsFileDescriptor(c), 7);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(FileObjectExtensionApiTest,
       AsFileDescriptorWithNegativeSmallIntRaisesValueError) {
  PyObjectPtr obj(PyLong_FromLong(-7));
  EXPECT_EQ(PyObject_AsFileDescriptor(obj), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(FileObjectExtensionApiTest,
       AsFileDescriptorWithNegativeFilenoRaisesValueError) {
  PyRun_SimpleString(R"(
class C:
  def fileno(self):
    return -7
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyObject_AsFileDescriptor(c), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(FileObjectExtensionApiTest, AsFileDescriptorWithNonIntRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  def fileno(self):
    return "foobar"
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyObject_AsFileDescriptor(c), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

}  // namespace python
