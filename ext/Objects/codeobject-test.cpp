#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using CodeExtensionApiTest = ExtensionApi;

TEST_F(CodeExtensionApiTest, ConstantKeyWithNoneReturnsTwoTuple) {
  PyObjectPtr result(_PyCode_ConstantKey(Py_None));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyTuple_Check(result));
  ASSERT_EQ(PyTuple_Size(result), 2);
  EXPECT_EQ(PyTuple_GetItem(result, 0),
            reinterpret_cast<PyObject*>(Py_TYPE(Py_None)));
  EXPECT_EQ(PyTuple_GetItem(result, 1), Py_None);
}

TEST_F(CodeExtensionApiTest, ConstantKeyWithEllipsisReturnsTwoTuple) {
  PyObjectPtr result(_PyCode_ConstantKey(Py_Ellipsis));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyTuple_Check(result));
  ASSERT_EQ(PyTuple_Size(result), 2);
  EXPECT_EQ(PyTuple_GetItem(result, 0),
            reinterpret_cast<PyObject*>(Py_TYPE(Py_Ellipsis)));
  EXPECT_EQ(PyTuple_GetItem(result, 1), Py_Ellipsis);
}

TEST_F(CodeExtensionApiTest, ConstantKeyWithIntReturnsTwoTuple) {
  PyObjectPtr obj(PyLong_FromLong(5));
  PyObjectPtr result(_PyCode_ConstantKey(obj));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyTuple_Check(result));
  ASSERT_EQ(PyTuple_Size(result), 2);
  EXPECT_EQ(PyTuple_GetItem(result, 0),
            reinterpret_cast<PyObject*>(Py_TYPE(obj)));
  EXPECT_EQ(PyTuple_GetItem(result, 1), obj);
}

TEST_F(CodeExtensionApiTest, ConstantKeyWithTrueReturnsTwoTuple) {
  PyObjectPtr result(_PyCode_ConstantKey(Py_True));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyTuple_Check(result));
  ASSERT_EQ(PyTuple_Size(result), 2);
  EXPECT_EQ(PyTuple_GetItem(result, 0),
            reinterpret_cast<PyObject*>(Py_TYPE(Py_True)));
  EXPECT_EQ(PyTuple_GetItem(result, 1), Py_True);
}

TEST_F(CodeExtensionApiTest, ConstantKeyWithFalseReturnsTwoTuple) {
  PyObjectPtr result(_PyCode_ConstantKey(Py_False));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyTuple_Check(result));
  ASSERT_EQ(PyTuple_Size(result), 2);
  EXPECT_EQ(PyTuple_GetItem(result, 0),
            reinterpret_cast<PyObject*>(Py_TYPE(Py_False)));
  EXPECT_EQ(PyTuple_GetItem(result, 1), Py_False);
}

TEST_F(CodeExtensionApiTest, ConstantKeyWithBytesReturnsTwoTuple) {
  PyObjectPtr obj(PyBytes_FromString("hello"));
  PyObjectPtr result(_PyCode_ConstantKey(obj));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyTuple_Check(result));
  ASSERT_EQ(PyTuple_Size(result), 2);
  EXPECT_EQ(PyTuple_GetItem(result, 0),
            reinterpret_cast<PyObject*>(Py_TYPE(obj)));
  EXPECT_EQ(PyTuple_GetItem(result, 1), obj);
}

TEST_F(CodeExtensionApiTest, ConstantKeyWithStrReturnsTwoTuple) {
  PyObjectPtr obj(PyUnicode_FromString("hello"));
  PyObjectPtr result(_PyCode_ConstantKey(obj));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyTuple_Check(result));
  ASSERT_EQ(PyTuple_Size(result), 2);
  EXPECT_EQ(PyTuple_GetItem(result, 0),
            reinterpret_cast<PyObject*>(Py_TYPE(obj)));
  EXPECT_EQ(PyTuple_GetItem(result, 1), obj);
}

TEST_F(CodeExtensionApiTest, ConstantKeyWithCodeReturnsTwoTuple) {
  PyObjectPtr empty_tuple(PyTuple_New(0));
  PyObjectPtr empty_bytes(PyBytes_FromString(""));
  PyObjectPtr empty_str(PyUnicode_FromString(""));
  PyObjectPtr obj(reinterpret_cast<PyObject*>(PyCode_New(
      0, 0, 0, 0, 0, empty_bytes, empty_tuple, empty_tuple, empty_tuple,
      empty_tuple, empty_tuple, empty_str, empty_str, 0, empty_bytes)));
  PyObjectPtr result(_PyCode_ConstantKey(obj));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyTuple_Check(result));
  ASSERT_EQ(PyTuple_Size(result), 2);
  EXPECT_EQ(PyTuple_GetItem(result, 0),
            reinterpret_cast<PyObject*>(Py_TYPE(obj)));
  EXPECT_EQ(PyTuple_GetItem(result, 1), obj);
}

TEST_F(CodeExtensionApiTest, ConstantKeyWithFloatReturnsTwoTuple) {
  PyObjectPtr obj(PyFloat_FromDouble(1.0));
  PyObjectPtr result(_PyCode_ConstantKey(obj));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyTuple_Check(result));
  ASSERT_EQ(PyTuple_Size(result), 2);
  EXPECT_EQ(PyTuple_GetItem(result, 0),
            reinterpret_cast<PyObject*>(Py_TYPE(obj)));
  EXPECT_EQ(PyTuple_GetItem(result, 1), obj);
}

TEST_F(CodeExtensionApiTest,
       ConstantKeyWithFloatNegativeZeroReturnsThreeTuple) {
  PyObjectPtr obj(PyFloat_FromDouble(-0.0));
  PyObjectPtr result(_PyCode_ConstantKey(obj));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyTuple_Check(result));
  ASSERT_EQ(PyTuple_Size(result), 3);
  EXPECT_EQ(PyTuple_GetItem(result, 0),
            reinterpret_cast<PyObject*>(Py_TYPE(obj)));
  EXPECT_EQ(PyTuple_GetItem(result, 1), obj);
  EXPECT_EQ(PyTuple_GetItem(result, 2), Py_None);
}

TEST_F(CodeExtensionApiTest,
       ConstantKeyWithComplexBothNegativeZeroReturnsThreeTuple) {
  PyObjectPtr obj(PyComplex_FromDoubles(-0.0, -0.0));
  PyObjectPtr result(_PyCode_ConstantKey(obj));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyTuple_Check(result));
  ASSERT_EQ(PyTuple_Size(result), 3);
  EXPECT_EQ(PyTuple_GetItem(result, 0),
            reinterpret_cast<PyObject*>(Py_TYPE(obj)));
  EXPECT_EQ(PyTuple_GetItem(result, 1), obj);
  EXPECT_EQ(PyTuple_GetItem(result, 2), Py_True);
}

TEST_F(CodeExtensionApiTest,
       ConstantKeyWithComplexImagNegativeZeroReturnsThreeTuple) {
  PyObjectPtr obj(PyComplex_FromDoubles(1.0, -0.0));
  PyObjectPtr result(_PyCode_ConstantKey(obj));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyTuple_Check(result));
  ASSERT_EQ(PyTuple_Size(result), 3);
  EXPECT_EQ(PyTuple_GetItem(result, 0),
            reinterpret_cast<PyObject*>(Py_TYPE(obj)));
  EXPECT_EQ(PyTuple_GetItem(result, 1), obj);
  EXPECT_EQ(PyTuple_GetItem(result, 2), Py_False);
}

TEST_F(CodeExtensionApiTest,
       ConstantKeyWithComplexRealNegativeZeroReturnsThreeTuple) {
  PyObjectPtr obj(PyComplex_FromDoubles(-0.0, 1.0));
  PyObjectPtr result(_PyCode_ConstantKey(obj));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyTuple_Check(result));
  ASSERT_EQ(PyTuple_Size(result), 3);
  EXPECT_EQ(PyTuple_GetItem(result, 0),
            reinterpret_cast<PyObject*>(Py_TYPE(obj)));
  EXPECT_EQ(PyTuple_GetItem(result, 1), obj);
  EXPECT_EQ(PyTuple_GetItem(result, 2), Py_None);
}

TEST_F(CodeExtensionApiTest,
       ConstantKeyWithComplexNeitherNegativeZeroReturnsTwoTuple) {
  PyObjectPtr obj(PyComplex_FromDoubles(1.0, 1.0));
  PyObjectPtr result(_PyCode_ConstantKey(obj));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyTuple_Check(result));
  ASSERT_EQ(PyTuple_Size(result), 2);
  EXPECT_EQ(PyTuple_GetItem(result, 0),
            reinterpret_cast<PyObject*>(Py_TYPE(obj)));
  EXPECT_EQ(PyTuple_GetItem(result, 1), obj);
}

TEST_F(CodeExtensionApiTest, ConstantKeyWithTupleReturnsTwoTuple) {
  PyObjectPtr val0(PyLong_FromLong(0));
  Py_INCREF(val0);
  PyObjectPtr val1(PyLong_FromLong(1));
  Py_INCREF(val1);
  PyObjectPtr val2(PyLong_FromLong(2));
  Py_INCREF(val2);
  PyObjectPtr obj(PyTuple_New(3));
  PyTuple_SetItem(obj, 0, val0);
  PyTuple_SetItem(obj, 1, val1);
  PyTuple_SetItem(obj, 2, val2);
  PyObjectPtr result(_PyCode_ConstantKey(obj));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyTuple_Check(result));
  ASSERT_EQ(PyTuple_Size(result), 2);
  EXPECT_EQ(PyTuple_GetItem(result, 1), obj);
  PyObject* new_tuple = PyTuple_GetItem(result, 0);
  EXPECT_EQ(PyTuple_Size(new_tuple), PyTuple_Size(obj));
  EXPECT_TRUE(PyTuple_Check(PyTuple_GetItem(new_tuple, 0)));
  EXPECT_TRUE(PyTuple_Check(PyTuple_GetItem(new_tuple, 1)));
  EXPECT_TRUE(PyTuple_Check(PyTuple_GetItem(new_tuple, 2)));
}

TEST_F(CodeExtensionApiTest, ConstantKeyWithFrozenSetReturnsTwoTuple) {
  PyObjectPtr zero(PyLong_FromLong(0));
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));
  PyObjectPtr iterable(PyTuple_Pack(3, zero.get(), one.get(), two.get()));
  PyObjectPtr obj(PyFrozenSet_New(iterable));
  PyObjectPtr result(_PyCode_ConstantKey(obj));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyTuple_Check(result));
  ASSERT_EQ(PyTuple_Size(result), 2);
  EXPECT_EQ(PyTuple_GetItem(result, 1), obj);
  PyObject* new_frozenset = PyTuple_GetItem(result, 0);
  ASSERT_TRUE(PyFrozenSet_Check(new_frozenset));
  EXPECT_EQ(PySet_Size(new_frozenset), PySet_Size(obj));
}

TEST_F(CodeExtensionApiTest, ConstantKeyWithOtherObjectReturnsTwoTupleWithId) {
  PyObjectPtr obj(PyList_New(0));
  PyObjectPtr result(_PyCode_ConstantKey(obj));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyTuple_Check(result));
  ASSERT_EQ(PyTuple_Size(result), 2);
  PyObject* obj_id = PyTuple_GetItem(result, 0);
  ASSERT_TRUE(PyLong_Check(obj_id));
  EXPECT_EQ(PyLong_AsVoidPtr(obj_id), obj.get());
  EXPECT_EQ(PyTuple_GetItem(result, 1), obj);
}

TEST_F(CodeExtensionApiTest, GetFreevarsReturnsFreevars) {
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
  PyObjectPtr result(PyCode_GetFreevars(reinterpret_cast<PyObject*>(code)));
  EXPECT_EQ(result, freevars);
  Py_DECREF(code);
}

TEST_F(CodeExtensionApiTest, GetNameReturnsName) {
  PyObjectPtr empty_tuple(PyTuple_New(0));
  PyObjectPtr empty_bytes(PyBytes_FromString(""));
  PyObjectPtr empty_str(PyUnicode_FromString(""));
  PyObjectPtr name(PyUnicode_FromString("foobar"));
  PyCodeObject* code = PyCode_New(0, 0, 0, 0, 0, empty_bytes, empty_tuple,
                                  empty_tuple, empty_tuple, empty_tuple,
                                  empty_tuple, empty_str, name, 0, empty_bytes);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(code, nullptr);
  PyObjectPtr result(PyCode_GetName(reinterpret_cast<PyObject*>(code)));
  EXPECT_EQ(result, name);
  Py_DECREF(code);
}

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

TEST_F(CodeExtensionApiTest, NewEmptyReturnsCodeObject) {
  PyCodeObject* code = PyCode_NewEmpty("my filename", "my funcname", 123);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(code, nullptr);
  EXPECT_TRUE(PyCode_Check(code));
  EXPECT_EQ(PyCode_GetNumFree(code), 0);
  Py_DECREF(code);
}

TEST_F(CodeExtensionApiTest, NewWithValidArgsReturnsCodeObject) {
  int argcount = 3;
  int kwargcount = 0;
  int nlocals = 3;
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
  PyObjectPtr result(reinterpret_cast<PyObject*>(
      PyCode_New(argcount, kwargcount, nlocals, 0, 0, empty_bytes, empty_tuple,
                 empty_tuple, varnames, empty_tuple, cellvars, empty_str,
                 empty_str, 0, empty_bytes)));
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

}  // namespace testing
}  // namespace py
