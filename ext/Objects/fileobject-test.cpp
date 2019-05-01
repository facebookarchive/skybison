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

TEST_F(FileObjectExtensionApiTest, AsFileDescriptorWithIntSubclassReturnsInt) {
  PyRun_SimpleString(R"(
class Subclass(int): pass
obj = Subclass(5)
)");
  PyObjectPtr obj(moduleGet("__main__", "obj"));
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

TEST_F(FileObjectExtensionApiTest,
       AsFileDescriptorWithIntSubclassFilenoReturnsInt) {
  PyRun_SimpleString(R"(
class Subclass(int): pass
class C:
  def fileno(self):
    return Subclass(5)
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyObject_AsFileDescriptor(c), 5);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(FileObjectExtensionApiTest, WriteObjectWritesRepr) {
  PyObjectPtr file(moduleGet("sys", "stdout"));
  PyObjectPtr obj(PyUnicode_FromString("hello there"));
  CaptureStdStreams streams;
  ASSERT_EQ(PyFile_WriteObject(obj, file, 0), 0);
  EXPECT_EQ(streams.out(), "'hello there'");
}

TEST_F(FileObjectExtensionApiTest, WriteObjectWithFlagWritesStr) {
  PyObjectPtr file(moduleGet("sys", "stdout"));
  PyObjectPtr obj(PyUnicode_FromString("hello there"));
  PyObjectPtr obj2(PyLong_FromLong(12345));
  CaptureStdStreams streams;
  ASSERT_EQ(PyFile_WriteObject(obj, file, Py_PRINT_RAW), 0);
  ASSERT_EQ(PyFile_WriteObject(obj2, file, Py_PRINT_RAW), 0);
  EXPECT_EQ(streams.out(), "hello there12345");
}

TEST_F(FileObjectExtensionApiTest, WriteObjectWithNullObjectWritesNull) {
  PyObjectPtr file(moduleGet("sys", "stderr"));
  CaptureStdStreams streams;
  ASSERT_EQ(PyFile_WriteObject(nullptr, file, 0), 0);
  EXPECT_EQ(streams.err(), "<NULL>");
}

TEST_F(FileObjectExtensionApiTest, WriteObjectWithNullFileRaisesTypeError) {
  ASSERT_EQ(PyFile_WriteObject(nullptr, nullptr, 0), -1);
  EXPECT_EQ(PyErr_ExceptionMatches(PyExc_TypeError), 1);
}

TEST_F(FileObjectExtensionApiTest, WriteStringWritesString) {
  PyObjectPtr file(moduleGet("sys", "stdout"));
  CaptureStdStreams streams;
  ASSERT_EQ(
      PyFile_WriteString("The quick brown fox jumps over the lazy dog.", file),
      0);
  EXPECT_EQ(streams.out(), "The quick brown fox jumps over the lazy dog.");
}

TEST_F(FileObjectExtensionApiTest, WriteStringWithSetExceptionFails) {
  PyObjectPtr file(moduleGet("sys", "stdout"));
  PyErr_SetObject(PyExc_TypeError, Py_None);
  CaptureStdStreams streams;
  ASSERT_EQ(PyFile_WriteString("hello there", file), -1);
  ASSERT_EQ(streams.err(), "");
  ASSERT_EQ(streams.out(), "");
  EXPECT_NE(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyErr_ExceptionMatches(PyExc_TypeError), 1);
}

}  // namespace python
