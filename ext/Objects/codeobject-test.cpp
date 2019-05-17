#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using CodeExtensionApiTest = ExtensionApi;

TEST_F(CodeExtensionApiTest, GetNumFreeReturnsNumberOfFreevars) {
  PyObjectPtr freevars(PyTuple_New(3));
  PyTuple_SetItem(freevars, 0, PyUnicode_FromString("foo"));
  PyTuple_SetItem(freevars, 1, PyUnicode_FromString("bar"));
  PyTuple_SetItem(freevars, 2, PyUnicode_FromString("baz"));
  PyObjectPtr empty_tuple(PyTuple_New(0));
  PyObjectPtr empty_bytes(PyBytes_FromString(""));
  PyObjectPtr empty_str(PyUnicode_FromString(""));
  PyCodeObject* code = PyCode_New(
      0, 0, 0, 0, 0, empty_bytes, empty_tuple, empty_tuple, empty_tuple,
      freevars, empty_tuple, empty_str, empty_str, 0, empty_bytes);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(code, nullptr);
  EXPECT_EQ(PyCode_GetNumFree(code), 3);
  Py_DECREF(code);
}

TEST_F(CodeExtensionApiTest, NewWithValidArgsReturnsCodeObject) {
  int argcount = 3;
  int kwargcount = 0;
  PyObjectPtr varnames(PyTuple_New(argcount + kwargcount));
  PyTuple_SetItem(varnames, 0, PyUnicode_FromString("foo"));
  PyTuple_SetItem(varnames, 1, PyUnicode_FromString("bar"));
  PyTuple_SetItem(varnames, 2, PyUnicode_FromString("baz"));
  PyObjectPtr cellvars(PyTuple_New(2));
  PyTuple_SetItem(cellvars, 0, PyUnicode_FromString("foobar"));
  PyTuple_SetItem(cellvars, 1, PyUnicode_FromString("foobaz"));
  PyObjectPtr empty_tuple(PyTuple_New(0));
  PyObjectPtr empty_bytes(PyBytes_FromString(""));
  PyObjectPtr empty_str(PyUnicode_FromString(""));
  PyObjectPtr result(reinterpret_cast<PyObject*>(PyCode_New(
      argcount, kwargcount, 0, 0, 0, empty_bytes, empty_tuple, empty_tuple,
      varnames, empty_tuple, cellvars, empty_str, empty_str, 0, empty_bytes)));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(result, nullptr);
  EXPECT_TRUE(PyCode_Check(result));
}

TEST_F(CodeExtensionApiTest, NewWithNegativeArgcountRaisesSystemError) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr str(PyUnicode_FromString("foobar"));
  PyObjectPtr bytes(PyBytes_FromString("foobar"));
  EXPECT_EQ(PyCode_New(-1, 0, 0, 0, 0, bytes, tuple, tuple, tuple, tuple, tuple,
                       str, str, 0, bytes),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(CodeExtensionApiTest, NewWithNegativeKwonlyargcountRaisesSystemError) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr str(PyUnicode_FromString("foobar"));
  PyObjectPtr bytes(PyBytes_FromString("foobar"));
  EXPECT_EQ(PyCode_New(0, -1, 0, 0, 0, bytes, tuple, tuple, tuple, tuple, tuple,
                       str, str, 0, bytes),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(CodeExtensionApiTest, NewWithNegativeNlocalsRaisesSystemError) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr str(PyUnicode_FromString("foobar"));
  PyObjectPtr bytes(PyBytes_FromString("foobar"));
  EXPECT_EQ(PyCode_New(0, 0, -1, 0, 0, bytes, tuple, tuple, tuple, tuple, tuple,
                       str, str, 0, bytes),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(CodeExtensionApiTest, NewWithNullCodeRaisesSystemError) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr str(PyUnicode_FromString("foobar"));
  PyObjectPtr bytes(PyBytes_FromString("foobar"));
  EXPECT_EQ(PyCode_New(0, 0, 0, 0, 0, nullptr, tuple, tuple, tuple, tuple,
                       tuple, str, str, 0, bytes),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(CodeExtensionApiTest, NewWithNonBufferCodeRaisesSystemError) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr str(PyUnicode_FromString("foobar"));
  PyObjectPtr bytes(PyBytes_FromString("foobar"));
  EXPECT_EQ(PyCode_New(0, 0, 0, 0, 0, Py_None, tuple, tuple, tuple, tuple,
                       tuple, str, str, 0, bytes),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(CodeExtensionApiTest, NewWithNullConstsRaisesSystemError) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr str(PyUnicode_FromString("foobar"));
  PyObjectPtr bytes(PyBytes_FromString("foobar"));
  EXPECT_EQ(PyCode_New(0, 0, 0, 0, 0, bytes, nullptr, tuple, tuple, tuple,
                       tuple, str, str, 0, bytes),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(CodeExtensionApiTest, NewWithNonTupleConstsRaisesSystemError) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr str(PyUnicode_FromString("foobar"));
  PyObjectPtr bytes(PyBytes_FromString("foobar"));
  EXPECT_EQ(PyCode_New(0, 0, 0, 0, 0, bytes, Py_None, tuple, tuple, tuple,
                       tuple, str, str, 0, bytes),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(CodeExtensionApiTest, NewWithNullNamesRaisesSystemError) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr str(PyUnicode_FromString("foobar"));
  PyObjectPtr bytes(PyBytes_FromString("foobar"));
  EXPECT_EQ(PyCode_New(0, 0, 0, 0, 0, bytes, tuple, nullptr, tuple, tuple,
                       tuple, str, str, 0, bytes),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(CodeExtensionApiTest, NewWithNonTupleNamesRaisesSystemError) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr str(PyUnicode_FromString("foobar"));
  PyObjectPtr bytes(PyBytes_FromString("foobar"));
  EXPECT_EQ(PyCode_New(0, 0, 0, 0, 0, bytes, tuple, Py_None, tuple, tuple,
                       tuple, str, str, 0, bytes),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(CodeExtensionApiTest, NewWithNullVarnamesRaisesSystemError) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr str(PyUnicode_FromString("foobar"));
  PyObjectPtr bytes(PyBytes_FromString("foobar"));
  EXPECT_EQ(PyCode_New(0, 0, 0, 0, 0, bytes, tuple, tuple, nullptr, tuple,
                       tuple, str, str, 0, bytes),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(CodeExtensionApiTest, NewWithNonTupleVarnamesRaisesSystemError) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr str(PyUnicode_FromString("foobar"));
  PyObjectPtr bytes(PyBytes_FromString("foobar"));
  EXPECT_EQ(PyCode_New(0, 0, 0, 0, 0, bytes, tuple, tuple, Py_None, tuple,
                       tuple, str, str, 0, bytes),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(CodeExtensionApiTest, NewWithNullFreevarsRaisesSystemError) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr str(PyUnicode_FromString("foobar"));
  PyObjectPtr bytes(PyBytes_FromString("foobar"));
  EXPECT_EQ(PyCode_New(0, 0, 0, 0, 0, bytes, tuple, tuple, tuple, nullptr,
                       tuple, str, str, 0, bytes),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(CodeExtensionApiTest, NewWithNonTupleFreevarsRaisesSystemError) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr str(PyUnicode_FromString("foobar"));
  PyObjectPtr bytes(PyBytes_FromString("foobar"));
  EXPECT_EQ(PyCode_New(0, 0, 0, 0, 0, bytes, tuple, tuple, tuple, Py_None,
                       tuple, str, str, 0, bytes),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(CodeExtensionApiTest, NewWithNullCellvarsRaisesSystemError) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr str(PyUnicode_FromString("foobar"));
  PyObjectPtr bytes(PyBytes_FromString("foobar"));
  EXPECT_EQ(PyCode_New(0, 0, 0, 0, 0, bytes, tuple, tuple, tuple, tuple,
                       nullptr, str, str, 0, bytes),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(CodeExtensionApiTest, NewWithNonTupleCellvarsRaisesSystemError) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr str(PyUnicode_FromString("foobar"));
  PyObjectPtr bytes(PyBytes_FromString("foobar"));
  EXPECT_EQ(PyCode_New(0, 0, 0, 0, 0, bytes, tuple, tuple, tuple, tuple,
                       Py_None, str, str, 0, bytes),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(CodeExtensionApiTest, NewWithNullFilenameRaisesSystemError) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr str(PyUnicode_FromString("foobar"));
  PyObjectPtr bytes(PyBytes_FromString("foobar"));
  EXPECT_EQ(PyCode_New(0, 0, 0, 0, 0, bytes, tuple, tuple, tuple, tuple, tuple,
                       nullptr, str, 0, bytes),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(CodeExtensionApiTest, NewWithNonStrFilenameRaisesSystemError) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr str(PyUnicode_FromString("foobar"));
  PyObjectPtr bytes(PyBytes_FromString("foobar"));
  EXPECT_EQ(PyCode_New(0, 0, 0, 0, 0, bytes, tuple, tuple, tuple, tuple, tuple,
                       Py_None, str, 0, bytes),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(CodeExtensionApiTest, NewWithNullNameRaisesSystemError) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr str(PyUnicode_FromString("foobar"));
  PyObjectPtr bytes(PyBytes_FromString("foobar"));
  EXPECT_EQ(PyCode_New(0, 0, 0, 0, 0, bytes, tuple, tuple, tuple, tuple, tuple,
                       str, nullptr, 0, bytes),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(CodeExtensionApiTest, NewWithNonStrNameRaisesSystemError) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr str(PyUnicode_FromString("foobar"));
  PyObjectPtr bytes(PyBytes_FromString("foobar"));
  EXPECT_EQ(PyCode_New(0, 0, 0, 0, 0, bytes, tuple, tuple, tuple, tuple, tuple,
                       str, Py_None, 0, bytes),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(CodeExtensionApiTest, NewWithNullLnotabRaisesSystemError) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr str(PyUnicode_FromString("foobar"));
  PyObjectPtr bytes(PyBytes_FromString("foobar"));
  EXPECT_EQ(PyCode_New(0, 0, 0, 0, 0, bytes, tuple, tuple, tuple, tuple, tuple,
                       str, str, 0, nullptr),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(CodeExtensionApiTest, NewWithNonBytesLnotabRaisesSystemError) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr str(PyUnicode_FromString("foobar"));
  PyObjectPtr bytes(PyBytes_FromString("foobar"));
  EXPECT_EQ(PyCode_New(0, 0, 0, 0, 0, bytes, tuple, tuple, tuple, tuple, tuple,
                       str, str, 0, Py_None),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

}  // namespace python
