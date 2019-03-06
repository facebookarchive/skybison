#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using AbstractExtensionApiTest = ExtensionApi;

TEST_F(AbstractExtensionApiTest, PyBufferFillInfoSimpleFillsInfo) {
  Py_buffer buffer;
  char buf[13];
  PyObjectPtr pyobj(Py_None);
  long prev_refcount = Py_REFCNT(pyobj);
  int readonly = 1;
  int result = PyBuffer_FillInfo(&buffer, pyobj, buf, sizeof(buf), readonly,
                                 PyBUF_SIMPLE);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(Py_REFCNT(pyobj), prev_refcount + 1);
  EXPECT_EQ(buffer.obj, pyobj);
  EXPECT_EQ(buffer.buf, buf);
  EXPECT_EQ(buffer.len, (Py_ssize_t)sizeof(buf));
  EXPECT_EQ(buffer.readonly, 1);
  EXPECT_EQ(buffer.itemsize, 1);
  EXPECT_EQ(buffer.format, nullptr);
  EXPECT_EQ(buffer.ndim, 1);
  EXPECT_EQ(buffer.shape, nullptr);
  EXPECT_EQ(buffer.strides, nullptr);
  EXPECT_EQ(buffer.suboffsets, nullptr);
  EXPECT_EQ(buffer.internal, nullptr);
}

TEST_F(AbstractExtensionApiTest,
       PyBufferFillInfoWithWritableFormatNdStridesFillsInfo) {
  Py_buffer buffer;
  char buf[7];
  PyObjectPtr pyobj(Py_None);
  long prev_refcount = Py_REFCNT(pyobj);
  int readonly = 0;
  int result = PyBuffer_FillInfo(
      &buffer, pyobj, buf, sizeof(buf), readonly,
      PyBUF_WRITABLE | PyBUF_FORMAT | PyBUF_ND | PyBUF_STRIDES);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(Py_REFCNT(pyobj), prev_refcount + 1);
  EXPECT_EQ(buffer.obj, pyobj);
  EXPECT_EQ(buffer.buf, buf);
  EXPECT_EQ(buffer.len, (Py_ssize_t)sizeof(buf));
  EXPECT_EQ(buffer.readonly, 0);
  EXPECT_EQ(buffer.itemsize, 1);
  EXPECT_EQ(strcmp(buffer.format, "B"), 0);
  EXPECT_EQ(buffer.ndim, 1);
  EXPECT_EQ(buffer.shape, &buffer.len);
  EXPECT_EQ(buffer.strides, &buffer.itemsize);
  EXPECT_EQ(buffer.suboffsets, nullptr);
  EXPECT_EQ(buffer.internal, nullptr);
}

TEST_F(AbstractExtensionApiTest, PyBufferFillInfoWithNullptrRaisesBufferError) {
  int result = PyBuffer_FillInfo(nullptr, Py_None, nullptr, 0, 1, PyBUF_SIMPLE);
  EXPECT_EQ(result, -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_BufferError));
}

TEST_F(AbstractExtensionApiTest,
       PyBufferFillInfoWithWritableFlagAndReadonlyRaisesBufferError) {
  Py_buffer buffer;
  int result =
      PyBuffer_FillInfo(&buffer, Py_None, nullptr, 0, 1, PyBUF_WRITABLE);
  EXPECT_EQ(result, -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_BufferError));
}

TEST_F(AbstractExtensionApiTest, PyMappingCheckWithoutGetItemReturnsFalse) {
  PyRun_SimpleString(R"(
class ClassWithoutDunderGetItem: pass

obj = ClassWithoutDunderGetItem()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_FALSE(PyMapping_Check(obj));
}

TEST_F(AbstractExtensionApiTest,
       PyMappingCheckWithoutGetItemOnClassReturnsFalse) {
  PyRun_SimpleString(R"(
class ClassWithoutDunderGetItem: pass

obj = ClassWithoutDunderGetItem()
obj.__getitem__ = lambda self, key: 1
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_FALSE(PyMapping_Check(obj));
}

TEST_F(AbstractExtensionApiTest, PyMappingCheckWithNumericReturnsFalse) {
  PyObjectPtr num(PyLong_FromLong(4));
  EXPECT_FALSE(PyMapping_Check(num));
}

TEST_F(AbstractExtensionApiTest, PyMappingCheckWithSetReturnsFalse) {
  PyObjectPtr set(PySet_New(nullptr));
  EXPECT_FALSE(PyMapping_Check(set));
}

TEST_F(AbstractExtensionApiTest, PyMappingCheckWithBooleanReturnsFalse) {
  PyObjectPtr py_false(Py_False);
  EXPECT_FALSE(PyMapping_Check(py_false));
}

TEST_F(AbstractExtensionApiTest, PyMappingCheckWithGetItemMethodReturnsTrue) {
  PyRun_SimpleString(R"(
class ClassWithDunderGetItemMethod:
  def __getitem__(self, key):
    return None

obj = ClassWithDunderGetItemMethod()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_TRUE(PyMapping_Check(obj));
}

TEST_F(AbstractExtensionApiTest, PyMappingCheckWithGetItemAttrReturnsTrue) {
  PyRun_SimpleString(R"(
class ClassWithDunderGetItemAttr:
  __getitem__ = 42

obj = ClassWithDunderGetItemAttr()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_TRUE(PyMapping_Check(obj));
}

TEST_F(AbstractExtensionApiTest, PyMappingCheckWithStringReturnsTrue) {
  PyObjectPtr str(PyUnicode_FromString("foo"));
  EXPECT_TRUE(PyMapping_Check(str));
}

TEST_F(AbstractExtensionApiTest, PyMappingCheckWithListReturnsTrue) {
  PyObjectPtr list(PyList_New(3));
  EXPECT_TRUE(PyMapping_Check(list));
}

TEST_F(AbstractExtensionApiTest, PyMappingCheckWithDictReturnsTrue) {
  PyObjectPtr dict(PyDict_New());
  EXPECT_TRUE(PyMapping_Check(dict));
}

TEST_F(AbstractExtensionApiTest, PyMappingLengthOnNullRaisesSystemError) {
  EXPECT_EQ(PyMapping_Length(nullptr), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PyMappingLengthWithNonMappingReturnsLen) {
  PyRun_SimpleString(R"(
class Foo:
  def __len__(self):
    return 1
obj = Foo()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_EQ(PyMapping_Length(obj), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyMappingSizeOnNullRaisesSystemError) {
  EXPECT_EQ(PyMapping_Size(nullptr), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

// Number Protocol

TEST_F(AbstractExtensionApiTest, PyNumberAbsoluteWithNullRaisesSystemError) {
  ASSERT_EQ(PyNumber_Absolute(nullptr), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       PyNumberAbsoluteWithNoDunderAbsRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  pass
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  ASSERT_EQ(PyNumber_Absolute(c), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberAbsoluteCallsDunderAbs) {
  PyObjectPtr negative(PyLong_FromLong(-10));
  PyObjectPtr positive(PyLong_FromLong(10));
  EXPECT_EQ(PyNumber_Absolute(negative), positive);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyNumberAddWithNoDunderAddRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  pass
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  ASSERT_EQ(PyNumber_Add(c, c), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberAddCallsDunderAdd) {
  PyRun_SimpleString(R"(
class ClassWithDunderAdd:
  def __add__(self, other):
    return "hello";

x = ClassWithDunderAdd()
  )");
  PyObjectPtr x(moduleGet("__main__", "x"));
  PyObjectPtr y(PyLong_FromLong(7));
  PyObjectPtr result(PyNumber_Add(x, y));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "hello"));
}

TEST_F(AbstractExtensionApiTest, PyNumberAddWithIntsReturnsSum) {
  PyObjectPtr x(PyLong_FromLong(7));
  PyObjectPtr y(PyLong_FromLong(10));
  PyObjectPtr result(PyNumber_Add(x, y));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 17);
}

TEST_F(AbstractExtensionApiTest, PyNumberAddWithUnicodeReturnsConcat) {
  PyObjectPtr x(PyUnicode_FromString("foo"));
  PyObjectPtr y(PyUnicode_FromString("bar"));
  PyObjectPtr result(PyNumber_Add(x, y));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "foobar"));
}

TEST_F(AbstractExtensionApiTest, PyNumberAndWithNonIntRaisesTypeError) {
  PyObjectPtr x(PyUnicode_FromString("foo"));
  PyObjectPtr y(PyLong_FromLong(2));
  ASSERT_EQ(PyNumber_And(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberAndWithIntsReturnsBitwiseAnd) {
  PyObjectPtr x(PyLong_FromLong(5));  // 0b0101
  PyObjectPtr y(PyLong_FromLong(3));  // 0b0011
  PyObjectPtr result(PyNumber_And(x, y));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 1);  // 0b0001
}

TEST_F(AbstractExtensionApiTest, PyNumberAsSsizeTWithNullRaisesSystemError) {
  ASSERT_EQ(PyNumber_AsSsize_t(nullptr, PyExc_TypeError), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PyNumberAsSsizeTWithStringRaisesTypeError) {
  PyObjectPtr str(PyUnicode_FromString("foo"));
  ASSERT_EQ(PyNumber_AsSsize_t(str, nullptr), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberAsSsizeTWithIntReturnsInt) {
  PyObjectPtr num(PyLong_FromLong(10));
  Py_ssize_t result = PyNumber_AsSsize_t(num, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, 10);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberAsSsizeTWithNegativeOneReturnsNegativeOne) {
  PyObjectPtr num(PyLong_FromLong(-1));
  Py_ssize_t result = PyNumber_AsSsize_t(num, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, -1);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberAsSsizeTWithOverflowAndNullClearsError) {
  const unsigned char bytes[] = {0x01, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00};
  PyObjectPtr num(_PyLong_FromByteArray(bytes, sizeof(bytes), false, true));
  Py_ssize_t result = PyNumber_AsSsize_t(num, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, 0x7fffffffffffffff);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberAsSsizeTWithUnderflowAndNullClearsError) {
  const unsigned char bytes[] = {0xff, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00};
  PyObjectPtr num(_PyLong_FromByteArray(bytes, sizeof(bytes), false, true));
  Py_ssize_t result = PyNumber_AsSsize_t(num, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, -0x8000000000000000);
}

TEST_F(AbstractExtensionApiTest, PyNumberAsSsizeTWithOverflowSetsGivenError) {
  const unsigned char bytes[] = {0x01, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00};
  PyObjectPtr num(_PyLong_FromByteArray(bytes, sizeof(bytes), false, true));
  ASSERT_EQ(PyNumber_AsSsize_t(num, PyExc_ModuleNotFoundError), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ModuleNotFoundError));
}

TEST_F(AbstractExtensionApiTest, PyNumberCheckWithFloatReturnsTrue) {
  PyObjectPtr float_num(PyFloat_FromDouble(1.1));
  EXPECT_EQ(PyNumber_Check(float_num), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyNumberCheckWithIntReturnsTrue) {
  PyObjectPtr int_num(PyLong_FromLong(1));
  EXPECT_EQ(PyNumber_Check(int_num), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyNumberCheckWithFloatSubclassReturnsTrue) {
  PyRun_SimpleString(R"(
class SubFloat(float):
  pass
sub = SubFloat()
  )");
  PyObjectPtr sub(moduleGet("__main__", "sub"));
  EXPECT_EQ(PyNumber_Check(sub), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyNumberCheckWithDunderIntClassReturnsTrue) {
  PyRun_SimpleString(R"(
class DunderIntClass():
  def __int__(self):
    return 5
i = DunderIntClass()
  )");
  PyObjectPtr i(moduleGet("__main__", "i"));
  EXPECT_EQ(PyNumber_Check(i), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyNumberCheckWithDunderFloatClassReturnsTrue) {
  PyRun_SimpleString(R"(
class DunderFloatClass():
  def __float__(self):
    return 5.0
f = DunderFloatClass()
  )");
  PyObjectPtr f(moduleGet("__main__", "f"));
  EXPECT_EQ(PyNumber_Check(f), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyNumberCheckWithNonNumberReturnsFalse) {
  PyObjectPtr str(PyUnicode_FromString(""));
  EXPECT_EQ(PyNumber_Check(str), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyNumberCheckWithNullReturnsFalse) {
  EXPECT_EQ(PyNumber_Check(nullptr), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyNumberFloorDivideWithNonIntRaisesTypeError) {
  PyObjectPtr x(PyUnicode_FromString("foo"));
  PyObjectPtr y(PyLong_FromLong(2));
  ASSERT_EQ(PyNumber_FloorDivide(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberFloorDivideWithIntsReturnsInt) {
  PyObjectPtr x(PyLong_FromLong(42));
  PyObjectPtr y(PyLong_FromLong(5));
  PyObjectPtr result(PyNumber_FloorDivide(x, y));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 8);
}

TEST_F(AbstractExtensionApiTest, PyNumberIndexOnIntReturnsSelf) {
  PyObjectPtr pylong(PyLong_FromLong(666));
  PyObjectPtr index(PyNumber_Index(pylong));
  EXPECT_EQ(index, pylong);
}

TEST_F(AbstractExtensionApiTest, PyNumberIndexCallsDunderIndex) {
  PyRun_SimpleString(R"(
class IntLikeClass:
  def __index__(self):
    return 42;

i = IntLikeClass();
  )");
  PyObjectPtr i(moduleGet("__main__", "i"));
  PyObjectPtr index(PyNumber_Index(i));
  ASSERT_TRUE(PyLong_CheckExact(index));
  EXPECT_EQ(PyLong_AsLong(index), 42);
}

TEST_F(AbstractExtensionApiTest, PyNumberIndexOnNullRaisesSystemError) {
  ASSERT_EQ(PyNumber_Index(nullptr), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PyNumberIndexOnNonIntRaisesTypeError) {
  PyObjectPtr str(PyUnicode_FromString("not an int"));
  ASSERT_EQ(PyNumber_Index(str), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest,
       PyNumberIndexWithMistypedDunderIndexRaisesTypeError) {
  PyRun_SimpleString(R"(
class IntLikeClass:
  def __index__(self):
    return "not an int";

i = IntLikeClass();
  )");
  PyObjectPtr i(moduleGet("__main__", "i"));
  ASSERT_EQ(PyNumber_Index(i), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest,
       PyNumberInPlaceAddWithNonNumberRaisesTypeError) {
  PyObjectPtr x(PyUnicode_FromString("foo"));
  PyObjectPtr y(PyLong_FromLong(2));
  ASSERT_EQ(PyNumber_InPlaceAdd(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberInPlaceAddWithIntsReturnsInt) {
  PyObjectPtr x(PyLong_FromLong(4));
  PyObjectPtr y(PyLong_FromLong(2));
  PyObjectPtr result(PyNumber_InPlaceAdd(x, y));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 6);
  EXPECT_EQ(PyLong_AsLong(x), 4);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberInPlaceAndWithNonNumberRaisesTypeError) {
  PyObjectPtr x(PyUnicode_FromString("foo"));
  PyObjectPtr y(PyLong_FromLong(2));
  ASSERT_EQ(PyNumber_InPlaceAnd(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberInPlaceAndWithIntsReturnsInt) {
  PyObjectPtr x(PyLong_FromLong(5));  // 0b0101
  PyObjectPtr y(PyLong_FromLong(3));  // 0b0011
  PyObjectPtr result(PyNumber_InPlaceAnd(x, y));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 1);  // 0b0001
  EXPECT_EQ(PyLong_AsLong(x), 5);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberInPlaceFloorDivideWithNonNumberRaisesTypeError) {
  PyObjectPtr x(PyUnicode_FromString("foo"));
  PyObjectPtr y(PyLong_FromLong(2));
  ASSERT_EQ(PyNumber_InPlaceFloorDivide(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberInPlaceFloorDivideWithIntsReturnsInt) {
  PyObjectPtr x(PyLong_FromLong(42));
  PyObjectPtr y(PyLong_FromLong(5));
  PyObjectPtr result(PyNumber_InPlaceFloorDivide(x, y));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 8);
  EXPECT_EQ(PyLong_AsLong(x), 42);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberInPlaceLshiftWithNonNumberRaisesTypeError) {
  PyObjectPtr x(PyUnicode_FromString("foo"));
  PyObjectPtr y(PyLong_FromLong(2));
  ASSERT_EQ(PyNumber_InPlaceLshift(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberInPlaceLshiftIntsReturnsInt) {
  PyObjectPtr x(PyLong_FromLong(5));
  PyObjectPtr y(PyLong_FromLong(3));
  PyObjectPtr result(PyNumber_InPlaceLshift(x, y));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 40);
  EXPECT_EQ(PyLong_AsLong(x), 5);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberInPlaceMatrixMultiplyWithNonNumberRaisesTypeError) {
  PyObjectPtr x(PyUnicode_FromString("foo"));
  PyObjectPtr y(PyLong_FromLong(2));
  ASSERT_EQ(PyNumber_InPlaceMatrixMultiply(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest,
       PyNumberInPlaceMatrixMultiplyCallsDunderImatmul) {
  PyRun_SimpleString(R"(
class C:
  def __init__(self):
    self.called = False
  def __imatmul__(self, other):
    self.called = True
    return 1
x = C()
)");
  PyObjectPtr x(moduleGet("__main__", "x"));
  PyObjectPtr y(PyLong_FromLong(3));
  PyObjectPtr called1(PyObject_GetAttrString(x, "called"));
  EXPECT_EQ(called1, Py_False);
  PyObjectPtr result(PyNumber_InPlaceMatrixMultiply(x, y));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 1);
  PyObjectPtr called2(PyObject_GetAttrString(x, "called"));
  EXPECT_EQ(called2, Py_True);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberInPlaceMultiplyWithNonNumberRaisesTypeError) {
  PyObjectPtr x(PyLong_FromLong(2));
  ASSERT_EQ(PyNumber_InPlaceMultiply(x, Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberInPlaceMultiplyWithIntsReturnsInt) {
  PyObjectPtr x(PyLong_FromLong(3));
  PyObjectPtr y(PyLong_FromLong(2));
  PyObjectPtr result(PyNumber_InPlaceMultiply(x, y));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 6);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberInPlaceOrWithNonNumberRaisesTypeError) {
  PyObjectPtr x(PyUnicode_FromString("foo"));
  PyObjectPtr y(PyLong_FromLong(2));
  ASSERT_EQ(PyNumber_InPlaceOr(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberInPlaceOrWithIntsReturnsInt) {
  PyObjectPtr x(PyLong_FromLong(5));  // 0b0101
  PyObjectPtr y(PyLong_FromLong(3));  // 0b0011
  PyObjectPtr result(PyNumber_InPlaceOr(x, y));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 7);  // 0b0111
  EXPECT_EQ(PyLong_AsLong(x), 5);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberInPlacePowerWithNonNumberRaisesTypeError) {
  PyObjectPtr x(PyUnicode_FromString("foo"));
  PyObjectPtr y(PyLong_FromLong(2));
  ASSERT_EQ(PyNumber_InPlacePower(x, y, Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberInPlacePowerCallsDunderIpow) {
  PyRun_SimpleString(R"(
class C:
  def __init__(self):
    self.called = False
  def __ipow__(self, other):
    self.called = True
    return 1
x = C()
)");
  PyObjectPtr x(moduleGet("__main__", "x"));
  PyObjectPtr y(PyLong_FromLong(3));
  PyObjectPtr called1(PyObject_GetAttrString(x, "called"));
  EXPECT_EQ(called1, Py_False);
  PyObjectPtr result(PyNumber_InPlacePower(x, y, Py_None));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 1);
  PyObjectPtr called2(PyObject_GetAttrString(x, "called"));
  EXPECT_EQ(called2, Py_True);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberInPlaceRemainderWithNonNumberRaisesTypeError) {
  PyObjectPtr x(PyList_New(0));
  PyObjectPtr y(PyLong_FromLong(2));
  ASSERT_EQ(PyNumber_InPlaceRemainder(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberInPlaceRemainderWithIntsReturnsInt) {
  PyObjectPtr x(PyLong_FromLong(42));
  PyObjectPtr y(PyLong_FromLong(5));
  PyObjectPtr result(PyNumber_InPlaceRemainder(x, y));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 2);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberInPlaceRshiftWithNonNumberRaisesTypeError) {
  PyObjectPtr x(PyUnicode_FromString("foo"));
  PyObjectPtr y(PyLong_FromLong(2));
  ASSERT_EQ(PyNumber_InPlaceRshift(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberInPlaceRshiftIntsReturnsInt) {
  PyObjectPtr x(PyLong_FromLong(42));
  PyObjectPtr y(PyLong_FromLong(3));
  PyObjectPtr result(PyNumber_InPlaceRshift(x, y));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 5);
  EXPECT_EQ(PyLong_AsLong(x), 42);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberInPlaceSubtractWithNonNumberRaisesTypeError) {
  PyObjectPtr x(PyUnicode_FromString("foo"));
  PyObjectPtr y(PyLong_FromLong(2));
  ASSERT_EQ(PyNumber_InPlaceSubtract(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberInPlaceSubtractIntsReturnsInt) {
  PyObjectPtr x(PyLong_FromLong(5));
  PyObjectPtr y(PyLong_FromLong(3));
  PyObjectPtr result(PyNumber_InPlaceSubtract(x, y));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 2);
  EXPECT_EQ(PyLong_AsLong(x), 5);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberInPlaceTrueDivideWithNonNumberRaisesTypeError) {
  PyObjectPtr x(PyUnicode_FromString("foo"));
  PyObjectPtr y(PyLong_FromLong(2));
  ASSERT_EQ(PyNumber_InPlaceTrueDivide(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest,
       PyNumberInPlaceTrueDivideWithFloatsReturnsFloat) {
  PyObjectPtr x(PyLong_FromLong(42.0));
  PyObjectPtr y(PyLong_FromLong(5));
  PyObjectPtr result(PyNumber_InPlaceTrueDivide(x, y));
  ASSERT_TRUE(PyFloat_CheckExact(result));
  EXPECT_EQ(PyFloat_AsDouble(result), 8.4);
  EXPECT_EQ(PyFloat_AsDouble(x), 42.0);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberInPlaceXorWithNonNumberRaisesTypeError) {
  PyObjectPtr x(PyUnicode_FromString("foo"));
  PyObjectPtr y(PyLong_FromLong(2));
  ASSERT_EQ(PyNumber_InPlaceXor(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberInPlaceXorWithIntsReturnsInt) {
  PyObjectPtr x(PyLong_FromLong(5));  // 0b0101
  PyObjectPtr y(PyLong_FromLong(3));  // 0b0011
  PyObjectPtr result(PyNumber_InPlaceXor(x, y));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 6);  // 0b0110
  EXPECT_EQ(PyLong_AsLong(x), 5);
}

TEST_F(AbstractExtensionApiTest, PyNumberInvertWithIntReturnsInt) {
  PyObjectPtr num(PyLong_FromLong(7));
  PyObjectPtr result(PyNumber_Invert(num));
  EXPECT_EQ(PyLong_AsLong(result), -8);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberInvertWithCustomClassCallsDunderInvert) {
  PyRun_SimpleString(R"(
class C:
  def __invert__(self):
    return "custom invert"
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyNumber_Invert(c));
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(result, "custom invert"), 0);
}

TEST_F(AbstractExtensionApiTest, PyNumberInvertWithNullRaisesSystemError) {
  ASSERT_EQ(PyNumber_Invert(nullptr), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PyNumberInvertWithNonNumberRaisesTypeError) {
  ASSERT_EQ(PyNumber_Positive(Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberInvertPropagatesException) {
  PyRun_SimpleString(R"(
class C:
  def __invert__(self):
    raise UserWarning()
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyNumber_Invert(c));
  ASSERT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_UserWarning));
}

TEST_F(AbstractExtensionApiTest, PyNumberLongWithNullRaisesSystemError) {
  EXPECT_EQ(PyNumber_Long(nullptr), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PyNumberLongWithIntReturnsInt) {
  PyObjectPtr intobj(PyLong_FromLong(7));
  Py_ssize_t refcnt = Py_REFCNT(intobj);
  PyObjectPtr result(PyNumber_Long(intobj));
  EXPECT_EQ(result, intobj);
  EXPECT_EQ(Py_REFCNT(result), refcnt + 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberLongDunderLongReturnsNonIntRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  def __int__(self):
    return "foo"
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyNumber_Long(c), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberLongCallsDunderTrunc) {
  PyRun_SimpleString(R"(
class C:
  def __trunc__(self):
    return 7
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyNumber_Long(c));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyLong_AsLong(result), 7);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyNumberLongCallsDunderTruncAndDunderInt) {
  PyRun_SimpleString(R"(
class D:
  def __int__(self):
    return 8

class C:
  def __trunc__(self):
    return D()

c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyNumber_Long(c));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyLong_AsLong(result), 8);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyNumberLongWithStringReturnsInt) {
  PyObjectPtr str(PyUnicode_FromString("7"));
  PyObjectPtr result(PyNumber_Long(str));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyLong_AsLong(result), 7);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberLongWithUnsupportedTypeRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  pass
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyNumber_Long(c), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberLshiftWithNonIntSelfRaisesTypeError) {
  PyObjectPtr x(PyFloat_FromDouble(5.0));
  PyObjectPtr y(PyLong_FromLong(2));
  ASSERT_EQ(PyNumber_Lshift(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberLshiftWithNonIntOtherRaisesTypeError) {
  PyObjectPtr x(PyLong_FromLong(5));
  PyObjectPtr y(PyFloat_FromDouble(2.0));
  ASSERT_EQ(PyNumber_Lshift(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberLshiftWithIntsShiftBitsLeft) {
  PyObjectPtr x(PyLong_FromLong(0x13));
  PyObjectPtr y(PyLong_FromLong(2));
  PyObjectPtr result(PyNumber_Lshift(x, y));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 0x4C);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberMatrixMultiplyWithoutDunderMatmulRaisesTypeError) {
  PyObjectPtr x(PyFloat_FromDouble(5.0));
  PyObjectPtr y(PyLong_FromLong(2));
  ASSERT_EQ(PyNumber_MatrixMultiply(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberMatrixMultiplyCallsDunderMatmul) {
  PyRun_SimpleString(R"(
class C:
  def __matmul__(self, other):
    return other
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr x(PyLong_FromLong(42));
  PyObjectPtr result(PyNumber_MatrixMultiply(c, x));
  ASSERT_TRUE(PyLong_CheckExact(result));
  ASSERT_EQ(PyLong_AsLong(result), 42);
}

TEST_F(AbstractExtensionApiTest, PyNumberMultiplyWithIntsReturnsInt) {
  PyObjectPtr x(PyLong_FromLong(5));
  PyObjectPtr y(PyLong_FromLong(2));
  PyObjectPtr result(PyNumber_Multiply(x, y));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 10);
}

TEST_F(AbstractExtensionApiTest, PyNumberMultiplyWithFloatReturnsFloat) {
  PyObjectPtr x(PyFloat_FromDouble(5.0));
  PyObjectPtr y(PyLong_FromLong(2));
  PyObjectPtr result(PyNumber_Multiply(x, y));
  ASSERT_TRUE(PyFloat_CheckExact(result));
  ASSERT_EQ(PyFloat_AsDouble(result), 10.0);
}

TEST_F(AbstractExtensionApiTest, PyNumberMultiplyCallsDunderMul) {
  PyRun_SimpleString(R"(
class C:
  def __mul__(self, other):
    return other
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr x(PyLong_FromLong(42));
  PyObjectPtr result(PyNumber_Multiply(c, x));
  ASSERT_TRUE(PyLong_CheckExact(result));
  ASSERT_EQ(PyLong_AsLong(result), 42);
}

TEST_F(AbstractExtensionApiTest, PyNumberNegativeWithIntReturnsInt) {
  PyObjectPtr num(PyLong_FromLong(-22));
  PyObjectPtr result(PyNumber_Negative(num));
  EXPECT_EQ(PyLong_AsLong(result), 22);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberNegativeWithCustomClassCallsDunderNeg) {
  PyRun_SimpleString(R"(
class C:
  def __neg__(self):
    return "custom neg"
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyNumber_Negative(c));
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(result, "custom neg"), 0);
}

TEST_F(AbstractExtensionApiTest, PyNumberNegativeWithNullRaisesSystemError) {
  EXPECT_EQ(PyNumber_Negative(nullptr), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PyNumberNegativeWithNonNumberRaisesTypeError) {
  EXPECT_EQ(PyNumber_Negative(Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberNegativePropagatesException) {
  PyRun_SimpleString(R"(
class C:
  def __neg__(self):
    raise UserWarning()
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyNumber_Negative(c));
  EXPECT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_UserWarning));
}

TEST_F(AbstractExtensionApiTest, PyNumberOrWithNonIntRaisesTypeError) {
  PyObjectPtr x(PyLong_FromLong(10));
  PyObjectPtr y(PyFloat_FromDouble(2.0));
  ASSERT_EQ(PyNumber_Or(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberOrWithIntsReturnsBitwiseOr) {
  PyObjectPtr x(PyLong_FromLong(5));  // 0b0101
  PyObjectPtr y(PyLong_FromLong(3));  // 0b0011
  PyObjectPtr result(PyNumber_Or(x, y));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 7);  // 0b0111
}

TEST_F(AbstractExtensionApiTest, PyNumberPositiveWithIntReturnsInt) {
  PyObjectPtr num(PyLong_FromLong(-13));
  PyObjectPtr result(PyNumber_Positive(num));
  EXPECT_EQ(PyLong_AsLong(result), -13);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberPositiveWithCustomClassCallsDunderPos) {
  PyRun_SimpleString(R"(
class C:
  def __pos__(self):
    return "custom pos"
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyNumber_Positive(c));
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(result, "custom pos"), 0);
}

TEST_F(AbstractExtensionApiTest, PyNumberPositiveWithNullRaisesSystemError) {
  ASSERT_EQ(PyNumber_Positive(nullptr), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PyNumberPositiveWithNonNumberRaisesTypeError) {
  ASSERT_EQ(PyNumber_Positive(Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberPositivePropagatesException) {
  PyRun_SimpleString(R"(
class C:
  def __pos__(self):
    raise UserWarning()
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyNumber_Positive(c));
  EXPECT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_UserWarning));
}

TEST_F(AbstractExtensionApiTest, PyNumberPowerWithNonNumberRaisesTypeError) {
  PyObjectPtr x(PyUnicode_FromString("foo"));
  PyObjectPtr y(PyLong_FromLong(2));
  ASSERT_EQ(PyNumber_Power(x, y, Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberPowerWithFloatReturnsFloat) {
  PyObjectPtr x(PyFloat_FromDouble(2.0));
  PyObjectPtr y(PyLong_FromLong(3));
  PyObjectPtr result(PyNumber_Power(x, y, Py_None));
  ASSERT_TRUE(PyFloat_CheckExact(result));
  EXPECT_EQ(PyFloat_AsDouble(result), 8.0);
}

TEST_F(AbstractExtensionApiTest, PyNumberRemainderWithNonIntRaisesTypeError) {
  PyObjectPtr x(PyLong_FromLong(10));
  PyObjectPtr y(PyUnicode_FromString("foo"));
  ASSERT_EQ(PyNumber_Remainder(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberRemainderWithIntsReturnsInt) {
  PyObjectPtr x(PyLong_FromLong(10));
  PyObjectPtr y(PyLong_FromLong(3));
  PyObjectPtr result(PyNumber_Remainder(x, y));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 1);
}

TEST_F(AbstractExtensionApiTest, PyNumberRshiftWithNonIntSelfRaisesTypeError) {
  PyObjectPtr x(PyFloat_FromDouble(5.0));
  PyObjectPtr y(PyLong_FromLong(2));
  ASSERT_EQ(PyNumber_Rshift(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberRshiftWithNonIntOtherRaisesTypeError) {
  PyObjectPtr x(PyLong_FromLong(5));
  PyObjectPtr y(PyFloat_FromDouble(2.0));
  ASSERT_EQ(PyNumber_Rshift(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberRshiftWithIntsShiftBitsRight) {
  PyObjectPtr x(PyLong_FromLong(0x4C));
  PyObjectPtr y(PyLong_FromLong(2));
  PyObjectPtr result(PyNumber_Rshift(x, y));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 0x13);
}

TEST_F(AbstractExtensionApiTest,
       PyNumberSubtractWithoutDunderSubtractRaisesTypeError) {
  PyObjectPtr x(PyUnicode_FromString("foo"));
  PyObjectPtr y(PyLong_FromLong(2));
  ASSERT_EQ(PyNumber_Subtract(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberSubtractCallsDunderSub) {
  PyRun_SimpleString(R"(
class C:
  def __sub__(self, other):
    return other
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr x(PyLong_FromLong(42));
  PyObjectPtr result(PyNumber_Subtract(c, x));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 42);
}

TEST_F(AbstractExtensionApiTest, PyNumberSubtractWithFloatReturnsFloat) {
  PyObjectPtr x(PyLong_FromLong(10));
  PyObjectPtr y(PyFloat_FromDouble(2.0));
  PyObjectPtr result(PyNumber_Subtract(x, y));
  ASSERT_TRUE(PyFloat_CheckExact(result));
  EXPECT_EQ(PyFloat_AsDouble(result), 8.0);

  PyObjectPtr result2(PyNumber_Subtract(y, x));
  ASSERT_TRUE(PyFloat_CheckExact(result2));
  EXPECT_EQ(PyFloat_AsDouble(result2), -8.0);
}

TEST_F(AbstractExtensionApiTest, PyNumberSubtractWithIntsReturnsInt) {
  PyObjectPtr x(PyLong_FromLong(10));
  PyObjectPtr y(PyLong_FromLong(2));
  PyObjectPtr result(PyNumber_Subtract(x, y));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 8);
}

TEST_F(AbstractExtensionApiTest, PyNumberTrueDivideWithNonIntRaisesTypeError) {
  PyObjectPtr x(PyUnicode_FromString("foo"));
  PyObjectPtr y(PyLong_FromLong(2));
  ASSERT_EQ(PyNumber_TrueDivide(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberTrueDivideCallsDunderTruediv) {
  PyRun_SimpleString(R"(
class C:
  def __truediv__(self, other):
    return other
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr x(PyLong_FromLong(42));
  PyObjectPtr result(PyNumber_TrueDivide(c, x));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 42);
}

TEST_F(AbstractExtensionApiTest, PyNumberTrueDivideWithIntsReturnsFloat) {
  PyObjectPtr x(PyLong_FromLong(42));
  PyObjectPtr y(PyLong_FromLong(5));
  PyObjectPtr result(PyNumber_TrueDivide(x, y));
  ASSERT_TRUE(PyFloat_CheckExact(result));
  EXPECT_EQ(PyFloat_AsDouble(result), 8.4);
}

TEST_F(AbstractExtensionApiTest, PyNumberTrueDivideWithFloatReturnsFloat) {
  PyObjectPtr a(PyFloat_FromDouble(42.0));
  PyObjectPtr b(PyLong_FromLong(5));
  PyObjectPtr result1(PyNumber_TrueDivide(a, b));
  ASSERT_TRUE(PyFloat_CheckExact(result1));
  EXPECT_EQ(PyFloat_AsDouble(result1), 8.4);

  PyObjectPtr x(PyLong_FromLong(42));
  PyObjectPtr y(PyFloat_FromDouble(5.0));
  PyObjectPtr result2(PyNumber_TrueDivide(x, y));
  ASSERT_TRUE(PyFloat_CheckExact(result2));
  EXPECT_EQ(PyFloat_AsDouble(result2), 8.4);
}

TEST_F(AbstractExtensionApiTest, PyNumberXorWithNonIntRaisesTypeError) {
  PyObjectPtr x(PyFloat_FromDouble(5.0));
  PyObjectPtr y(PyLong_FromLong(3));
  ASSERT_EQ(PyNumber_Xor(x, y), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyNumberXorWithIntsReturnsBitwiseOr) {
  PyObjectPtr v(PyLong_FromLong(5));  // 0b0101
  PyObjectPtr w(PyLong_FromLong(3));  // 0b0011
  PyObjectPtr result(PyNumber_Xor(v, w));
  ASSERT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 6);  // 0b0110
}

TEST_F(AbstractExtensionApiTest, PyObjectCallWithArgsCalls) {
  PyRun_SimpleString(R"(
def func(*args, **kwargs):
  return f"{args!r}{kwargs!r}"
)");
  PyObjectPtr func(moduleGet("__main__", "func"));
  PyObjectPtr args(PyTuple_Pack(3, PyUnicode_FromString("one"),
                                PyLong_FromLong(2), PyLong_FromLong(3)));
  PyObjectPtr result(PyObject_Call(func, args, nullptr));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "('one', 2, 3){}"));
}

TEST_F(AbstractExtensionApiTest, PyObjectCallWithArgsAndKwargsCalls) {
  PyRun_SimpleString(R"(
def func(*args, **kwargs):
  return f"{args!r}{kwargs!r}"
)");
  PyObjectPtr func(moduleGet("__main__", "func"));
  PyObjectPtr args(PyTuple_Pack(3, PyLong_FromLong(1), PyLong_FromLong(2),
                                PyUnicode_FromString("three")));
  PyObjectPtr kwargs(PyDict_New());
  PyDict_SetItem(kwargs, PyUnicode_FromString("kwarg"), PyLong_FromLong(4));
  PyObjectPtr result(PyObject_Call(func, args, kwargs));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "(1, 2, 'three'){'kwarg': 4}"));
}

TEST_F(AbstractExtensionApiTest, PyObjectCallPropagatesException) {
  PyRun_SimpleString(R"(
def func(*args, **kwargs):
  raise UserWarning()
)");
  PyObjectPtr func(moduleGet("__main__", "func"));
  PyObjectPtr args(PyTuple_New(0));
  PyObjectPtr result(PyObject_Call(func, args, nullptr));
  EXPECT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_UserWarning));
}

TEST_F(AbstractExtensionApiTest, PyObjectCallFunctionCalls) {
  PyRun_SimpleString(R"(
def func(*args):
  return f"{args!r}"
)");
  PyObjectPtr func(moduleGet("__main__", "func"));
  PyObjectPtr result(
      PyObject_CallFunction(func, "(iI)s#i", 3, 7, "aaaa", 3, 99));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "((3, 7), 'aaa', 99)"));
}

TEST_F(AbstractExtensionApiTest,
       PyObjectCallFunctionWithNonCallableRaisesTypeError) {
  PyObjectPtr result(PyObject_CallFunction(Py_None, nullptr));
  EXPECT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyObjectCallFunctionPropagatesException) {
  PyRun_SimpleString(R"(
def func():
  raise UserWarning()
)");
  PyObjectPtr func(moduleGet("__main__", "func"));
  PyObjectPtr result(PyObject_CallFunction(func, nullptr));
  EXPECT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_UserWarning));
}

TEST_F(AbstractExtensionApiTest, PyObjectCallFunctionSizeTCalls) {
  PyRun_SimpleString(R"(
def func(*args):
  return f"{args!r}"
)");
  PyObjectPtr func(moduleGet("__main__", "func"));
  PyObjectPtr result(
      PyObject_CallFunction(func, "is#i", 7, "bbb", Py_ssize_t{2}, 14));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "(7, 'bb', 14)"));
}

TEST_F(AbstractExtensionApiTest, PyObjectCallMethodCalls) {
  PyRun_SimpleString(R"(
class C:
  x = 42
  def func(self, *args):
    return f"{self.x}{args!r}"
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyObject_CallMethod(c, "func", "s#(i)", "ccc", 1, 7));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "42('c', (7,))"));
}

TEST_F(AbstractExtensionApiTest,
       PyObjectCallMethodWithNonExistentMemberRaisesAttributeError) {
  PyObjectPtr result(PyObject_CallMethod(Py_None, "foo", nullptr));
  EXPECT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_AttributeError));
}

TEST_F(AbstractExtensionApiTest, PyObjectCallMethodPropagatesException) {
  PyRun_SimpleString(R"(
class C:
  def func(self):
    raise UserWarning()
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyObject_CallMethod(c, "func", nullptr));
  EXPECT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_UserWarning));
}

TEST_F(AbstractExtensionApiTest, PyObjectCallMethodObjArgsCalls) {
  PyRun_SimpleString(R"(
class C:
  x = 23
  def func(self, *args):
    return f"{self.x}{args!r}"
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr name(PyUnicode_FromString("func"));
  PyObjectPtr arg0(PyLong_FromLong(-13));
  PyObjectPtr arg1(PyUnicode_FromString("zzz"));
  PyObjectPtr result(
      PyObject_CallMethodObjArgs(c, name, static_cast<PyObject*>(arg0),
                                 static_cast<PyObject*>(arg1), nullptr));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "23(-13, 'zzz')"));
}

TEST_F(AbstractExtensionApiTest,
       PyObjectCallMethodObjArgsWithNullObjectRaisesSystemError) {
  PyObjectPtr name(PyUnicode_FromString("func"));
  PyObjectPtr result(PyObject_CallMethodObjArgs(nullptr, name));
  ASSERT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       PyObjectCallMethodObjArgsWithNullMethodNameRaisesSystemError) {
  PyObjectPtr result(PyObject_CallMethodObjArgs(Py_None, nullptr));
  ASSERT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PyObjectCallMethodSizeTCalls) {
  PyRun_SimpleString(R"(
class C:
  x = -5
  def func(self, *args):
    return f"{self.x}{args!r}"
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(_PyObject_CallMethod_SizeT(c, "func", "is#i", 9, "ddd",
                                                Py_ssize_t{2}, 8));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "-5(9, 'dd', 8)"));
}

TEST_F(AbstractExtensionApiTest, PyObject_CallObjectCalls) {
  PyRun_SimpleString(R"(
class C:
  x = 9
  def __call__(self, *args):
    return f"{self.x}{args!r}"
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr args(PyTuple_Pack(
      3, PyLong_FromLong(1), PyUnicode_FromString("two"), PyLong_FromLong(3)));
  PyObjectPtr result(PyObject_CallObject(c, args));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "9(1, 'two', 3)"));
}

TEST_F(AbstractExtensionApiTest, PyObject_CallObjectWithArgsNullptrCalls) {
  PyRun_SimpleString(R"(
def func(*args, **kwargs):
  return f"{args!r}{kwargs!r}"
)");
  PyObjectPtr func(moduleGet("__main__", "func"));
  PyObjectPtr result(PyObject_CallObject(func, nullptr));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "(){}"));
}

TEST_F(AbstractExtensionApiTest, PyObjectCheckBufferWithBytesReturnsTrue) {
  PyObject* obj(PyBytes_FromString("foo"));
  EXPECT_TRUE(PyObject_CheckBuffer(obj));
  Py_DECREF(obj);
}

TEST_F(AbstractExtensionApiTest, PyObjectCheckBufferWithNonBytesReturnsFalse) {
  PyObject* obj(PyLong_FromLong(2));
  EXPECT_FALSE(PyObject_CheckBuffer(obj));
  Py_DECREF(obj);
}

TEST_F(AbstractExtensionApiTest,
       PyObjectLengthWithoutDunderLenRaisesTypeError) {
  PyObjectPtr num(PyLong_FromLong(3));
  ASSERT_EQ(PyObject_Length(num), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyObjectLengthOnNullRaisesSystemError) {
  EXPECT_EQ(PyObject_Length(nullptr), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PyObjectLengthWithNonIntLenRaisesTypeError) {
  PyRun_SimpleString(R"(
class Foo:
  def __len__(self):
    return "foo"
obj = Foo()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  ASSERT_EQ(PyObject_Length(obj), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyObjectLengthWithoutIndexRaisesTypeError) {
  PyRun_SimpleString(R"(
class Foo: pass
class Bar:
  def __len__(self): return Foo()
obj = Bar()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  ASSERT_EQ(PyObject_Length(obj), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyObjectLengthWithNonIntIndexRaisesTypeError) {
  PyRun_SimpleString(R"(
class Foo:
  def __index__(self): return None
class Bar:
  def __len__(self): return Foo()
obj = Bar()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  ASSERT_EQ(PyObject_Length(obj), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyObjectLengthWithIndexReturnsValue) {
  PyRun_SimpleString(R"(
class Foo:
  def __index__(self): return 1
class Bar:
  def __len__(self): return Foo()
obj = Bar()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_EQ(PyObject_Length(obj), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest,
       PyObjectLengthWithNegativeLenRaisesValueError) {
  PyRun_SimpleString(R"(
class Foo:
  def __len__(self):
    return -5
obj = Foo()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  ASSERT_EQ(PyObject_Length(obj), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(AbstractExtensionApiTest,
       PyObjectLengthWithOverflowRaisesOverflowError) {
  PyRun_SimpleString(R"(
class Foo:
  def __len__(self):
    return 0x8000000000000000
obj = Foo()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  ASSERT_EQ(PyObject_Length(obj), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_OverflowError));
}

TEST_F(AbstractExtensionApiTest,
       PyObjectLengthWithUnderflowRaisesOverflowError) {
  PyRun_SimpleString(R"(
class Foo:
  def __len__(self):
    return -0x8000000000000001
obj = Foo()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  ASSERT_EQ(PyObject_Length(obj), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_OverflowError));
}

TEST_F(AbstractExtensionApiTest, PyObjectLengthWithEmptyDictReturnsZero) {
  PyObjectPtr dict(PyDict_New());
  EXPECT_EQ(PyObject_Length(dict), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyObjectLengthWithEmptyListReturnsZero) {
  PyObjectPtr list(PyList_New(0));
  EXPECT_EQ(PyObject_Length(list), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyObjectLengthWithEmptyStringReturnsZero) {
  PyObjectPtr str(PyUnicode_FromString(""));
  EXPECT_EQ(PyObject_Length(str), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyObjectLengthWithNonEmptyDictReturnsValue) {
  PyObjectPtr dict(PyDict_New());

  {
    PyObjectPtr value(PyLong_FromLong(3));

    PyObjectPtr key1(PyLong_FromLong(1));
    PyDict_SetItem(dict, key1, value);

    PyObjectPtr key2(PyLong_FromLong(2));
    PyDict_SetItem(dict, key2, value);
  }

  EXPECT_EQ(PyObject_Length(dict), 2);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyObjectLengthWithNonEmptyListReturnsValue) {
  PyObjectPtr list(PyList_New(3));
  EXPECT_EQ(PyObject_Length(list), 3);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyObjectLengthWithNonEmptyStringReturnsValue) {
  PyObjectPtr str(PyUnicode_FromString("foo"));
  EXPECT_EQ(PyObject_Length(str), 3);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyObjectSizeOnNullRaisesSystemError) {
  EXPECT_EQ(PyObject_Size(nullptr), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PyObjectTypeOnNullRaisesSystemError) {
  EXPECT_EQ(PyObject_Type(nullptr), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PyObjectTypeReturnsType) {
  PyObjectPtr num(PyLong_FromLong(4));
  PyObjectPtr type(PyObject_Type(num));
  EXPECT_TRUE(PyType_Check(type));
}

TEST_F(AbstractExtensionApiTest, PySequenceCheckWithoutGetItemReturnsFalse) {
  PyRun_SimpleString(R"(
class ClassWithoutDunderGetItem: pass

obj = ClassWithoutDunderGetItem()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_FALSE(PySequence_Check(obj));
}

TEST_F(AbstractExtensionApiTest,
       PySequenceCheckWithoutGetItemOnClassReturnsFalse) {
  PyRun_SimpleString(R"(
class ClassWithoutDunderGetItem: pass

obj = ClassWithoutDunderGetItem()
obj.__getitem__ = lambda self, key : 1
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_FALSE(PySequence_Check(obj));
}

TEST_F(AbstractExtensionApiTest, PySequenceCheckWithNumericReturnsFalse) {
  PyObjectPtr num(PyLong_FromLong(3));
  EXPECT_FALSE(PySequence_Check(num));
}

TEST_F(AbstractExtensionApiTest, PySequenceCheckWithSetReturnsFalse) {
  PyObjectPtr set(PySet_New(nullptr));
  EXPECT_FALSE(PySequence_Check(set));
}

TEST_F(AbstractExtensionApiTest, PySequenceCheckWithDictReturnsFalse) {
  PyObjectPtr dict(PyDict_New());
  EXPECT_FALSE(PySequence_Check(dict));
}

TEST_F(AbstractExtensionApiTest, PySequenceCheckWithDictSubclassReturnsFalse) {
  PyRun_SimpleString(R"(
class Subclass(dict): pass

obj = Subclass()
)");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_FALSE(PySequence_Check(obj));
}

TEST_F(AbstractExtensionApiTest, PySequenceCheckWithNoneReturnsFalse) {
  PyObjectPtr py_none(Py_None);
  EXPECT_FALSE(PySequence_Check(py_none));
}

TEST_F(AbstractExtensionApiTest, PySequenceCheckWithGetItemMethodReturnsTrue) {
  PyRun_SimpleString(R"(
class ClassWithDunderGetItemMethod:
  def __getitem__(self, key):
    return None

obj = ClassWithDunderGetItemMethod()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_TRUE(PySequence_Check(obj));
}

TEST_F(AbstractExtensionApiTest, PySequenceCheckWithGetItemAttrReturnsTrue) {
  PyRun_SimpleString(R"(
class ClassWithDunderGetItemAttr:
  __getitem__ = 42

obj = ClassWithDunderGetItemAttr()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_TRUE(PySequence_Check(obj));
}

TEST_F(AbstractExtensionApiTest, PySequenceCheckWithStringReturnsTrue) {
  PyObjectPtr str(PyUnicode_FromString("foo"));
  EXPECT_TRUE(PySequence_Check(str));
}

TEST_F(AbstractExtensionApiTest, PySequenceCheckWithListReturnsTrue) {
  PyObjectPtr list(PyList_New(3));
  EXPECT_TRUE(PySequence_Check(list));
}

TEST_F(AbstractExtensionApiTest, PySequenceLengthOnNull) {
  EXPECT_EQ(PySequence_Length(nullptr), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PySequenceLengthWithNonSequenceReturnsValue) {
  PyRun_SimpleString(R"(
class Foo:
  def __len__(self):
    return 1
obj = Foo()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_EQ(PySequence_Length(obj), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

// PySequence_Length fails on `dict` in CPython, but succeeds on subclasses
TEST_F(AbstractExtensionApiTest,
       PySequenceLengthWithEmptyDictSubclassReturnsZero) {
  PyRun_SimpleString(R"(
class Foo(dict):
  pass
obj = Foo()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_EQ(PySequence_Length(obj), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest,
       PySequenceLengthWithNonEmptyDictSubclassReturnsValue) {
  PyRun_SimpleString(R"(
class Foo(dict):
  pass
obj = Foo()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));

  ASSERT_EQ(PyDict_SetItem(obj, one, two), 0);
  ASSERT_EQ(PyDict_SetItem(obj, two, one), 0);

  EXPECT_EQ(PySequence_Length(obj), 2);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PySequenceSizeOnNull) {
  EXPECT_EQ(PySequence_Size(nullptr), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PyObjCallFunctionObjArgsWithNullReturnsNull) {
  testing::PyObjectPtr test(PyObject_CallFunctionObjArgs(nullptr, nullptr));
  EXPECT_EQ(nullptr, test);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       PyObjCallFunctionObjArgsWithNonFunctionReturnsNull) {
  testing::PyObjectPtr non_func(PyTuple_New(0));
  testing::PyObjectPtr test(PyObject_CallFunctionObjArgs(non_func, nullptr));
  EXPECT_EQ(test, nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, CallFunctionObjArgsWithNoArgsReturnsValue) {
  PyRun_SimpleString(R"(
def func():
  return 5
  )");

  testing::PyObjectPtr func(testing::moduleGet("__main__", "func"));
  testing::PyObjectPtr func_result(PyObject_CallFunctionObjArgs(func, nullptr));
  EXPECT_EQ(PyLong_AsLong(func_result), 5);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest,
       CallFunctionObjArgsWithCallableClassReturnsValue) {
  PyRun_SimpleString(R"(
class Foo():
  def __call__(self):
    return 5
f = Foo()
  )");

  testing::PyObjectPtr f(testing::moduleGet("__main__", "f"));
  testing::PyObjectPtr f_result(PyObject_CallFunctionObjArgs(f, nullptr));
  EXPECT_EQ(PyLong_AsLong(f_result), 5);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest,
       CallFunctionObjArgsWithManyArgumentsReturnsValue) {
  PyRun_SimpleString(R"(
def func(a, b, c, d, e, f):
  return a + b + c + d + e + f
  )");

  testing::PyObjectPtr func(testing::moduleGet("__main__", "func"));
  PyObject* one = PyLong_FromLong(1);
  PyObject* two = PyLong_FromLong(2);
  testing::PyObjectPtr func_result(PyObject_CallFunctionObjArgs(
      func, one, one, two, two, one, two, nullptr));
  Py_DECREF(one);
  Py_DECREF(two);
  EXPECT_EQ(PyLong_AsLong(func_result), 9);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyIndexCheckWithIntReturnsTrue) {
  PyObjectPtr int_num(PyLong_FromLong(1));
  EXPECT_TRUE(PyIndex_Check(int_num.get()));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyIndexCheckWithFloatReturnsFalse) {
  PyObjectPtr float_num(PyFloat_FromDouble(1.1));
  EXPECT_FALSE(PyIndex_Check(float_num.get()));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest,
       PyIndexCheckWithUncallableDunderIndexReturnsTrue) {
  PyRun_SimpleString(R"(
class C:
  __index__ = None
idx = C()
  )");
  PyObjectPtr idx(moduleGet("__main__", "idx"));
  EXPECT_TRUE(PyIndex_Check(idx.get()));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyIndexCheckWithDunderIndexReturnsTrue) {
  PyRun_SimpleString(R"(
class C:
  def __index__(self):
    return 1
idx = C()
  )");
  PyObjectPtr idx(moduleGet("__main__", "idx"));
  EXPECT_TRUE(PyIndex_Check(idx.get()));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, GetIterWithNoDunderIterRaises) {
  PyRun_SimpleString(R"(
class C:
  pass
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  ASSERT_EQ(PyObject_GetIter(c), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, GetIterWithNonCallableDunderIterRaises) {
  PyRun_SimpleString(R"(
class C:
  __iter__ = 4
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  ASSERT_EQ(PyObject_GetIter(c), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, GetIterWithDunderIterReturningNonIterRaises) {
  PyRun_SimpleString(R"(
class C:
  def __iter__(self):
    return 4
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  ASSERT_EQ(PyObject_GetIter(c), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, GetIterPropagatesException) {
  PyRun_SimpleString(R"(
class C:
  def __iter__(self):
    raise ValueError("hi")
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  ASSERT_EQ(PyObject_GetIter(c), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(AbstractExtensionApiTest, PyIterNextReturnsNext) {
  PyObjectPtr tuple(PyTuple_Pack(3, PyLong_FromLong(1), PyLong_FromLong(2),
                                 PyLong_FromLong(3)));
  PyObjectPtr iter(PyObject_GetIter(tuple));
  ASSERT_NE(iter, nullptr);
  PyObjectPtr next(PyIter_Next(iter));
  ASSERT_NE(next, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsLong(next), 1);
  next = PyIter_Next(iter);
  ASSERT_NE(next, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsLong(next), 2);
  next = PyIter_Next(iter);
  ASSERT_NE(next, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsLong(next), 3);
  next = PyIter_Next(iter);
  ASSERT_EQ(next, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyIterNextOnNonIterRaises) {
  ASSERT_EQ(PyObject_GetIter(Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyIterNextPropagatesException) {
  PyRun_SimpleString(R"(
class C:
  def __iter__(self):
    return self
  def __next__(self):
    raise ValueError("hi")

c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr iter(PyObject_GetIter(c));
  ASSERT_NE(iter, nullptr);
  PyObjectPtr next(PyIter_Next(iter));
  ASSERT_EQ(next, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(AbstractExtensionApiTest, PySequenceConcatWithNullLeftRaises) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr result(PySequence_Concat(nullptr, tuple));
  ASSERT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PySequenceConcatWithNullRightRaises) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr result(PySequence_Concat(tuple, nullptr));
  ASSERT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PySequenceConcatCallsDunderAdd) {
  PyObjectPtr left(PyTuple_Pack(2, PyLong_FromLong(1), PyLong_FromLong(2)));
  PyObjectPtr right(PyTuple_Pack(2, PyLong_FromLong(3), PyLong_FromLong(4)));
  PyObjectPtr result(PySequence_Concat(left, right));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyTuple_CheckExact(result));
  ASSERT_EQ(PyTuple_Size(result), 4);
  EXPECT_EQ(PyTuple_GetItem(result, 0), PyTuple_GetItem(left, 0));
  EXPECT_EQ(PyTuple_GetItem(result, 1), PyTuple_GetItem(left, 1));
  EXPECT_EQ(PyTuple_GetItem(result, 2), PyTuple_GetItem(right, 0));
  EXPECT_EQ(PyTuple_GetItem(result, 3), PyTuple_GetItem(right, 1));
}

TEST_F(AbstractExtensionApiTest, PySequenceRepeatWithNullSeqRaises) {
  PyObjectPtr result(PySequence_Repeat(nullptr, 5));
  ASSERT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PySequenceRepeatCallsDunderMul) {
  PyObjectPtr seq(PyTuple_Pack(2, PyLong_FromLong(1), PyLong_FromLong(2)));
  PyObjectPtr result(PySequence_Repeat(seq, 2));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_EQ(PyTuple_Size(result), 4);
  EXPECT_EQ(PyTuple_GetItem(result, 0), PyTuple_GetItem(seq, 0));
  EXPECT_EQ(PyTuple_GetItem(result, 1), PyTuple_GetItem(seq, 1));
  EXPECT_EQ(PyTuple_GetItem(result, 2), PyTuple_GetItem(seq, 0));
  EXPECT_EQ(PyTuple_GetItem(result, 3), PyTuple_GetItem(seq, 1));
}

TEST_F(AbstractExtensionApiTest, PySequenceCountWithNullSeqRaises) {
  PyObjectPtr obj(PyLong_FromLong(1));
  EXPECT_EQ(PySequence_Count(nullptr, obj), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PySequenceCountWithNullObjRaises) {
  PyObjectPtr tuple(PyTuple_Pack(2, PyLong_FromLong(1), PyLong_FromLong(2)));
  EXPECT_EQ(PySequence_Count(tuple, nullptr), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PySequenceCountCountsOccurrences) {
  PyObjectPtr obj(PyLong_FromLong(2));
  PyObjectPtr tuple(PyTuple_Pack(3, PyLong_FromLong(1), PyLong_FromLong(2),
                                 PyLong_FromLong(2)));
  EXPECT_EQ(PySequence_Count(tuple, obj), 2);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PySequenceGetItemCallsDunderGetItem) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    return 7
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PySequence_GetItem(c, 0));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, PyLong_FromLong(7));
}

TEST_F(AbstractExtensionApiTest, PySequenceSetItemWithNullValCallsDelItem) {
  PyRun_SimpleString(R"(
sideeffect = 0
class C:
  def __delitem__(self, key):
    global sideeffect
    sideeffect = 10
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  ASSERT_EQ(PySequence_SetItem(c, 0, nullptr), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr sideeffect(moduleGet("__main__", "sideeffect"));
  EXPECT_EQ(PyLong_AsLong(sideeffect), 10);
}

TEST_F(AbstractExtensionApiTest, PySequenceSetItemCallsDunderSetItem) {
  PyRun_SimpleString(R"(
sideeffect = 0
class C:
  def __setitem__(self, key, val):
    global sideeffect
    sideeffect = 10
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr val(PyLong_FromLong(4));
  ASSERT_EQ(PySequence_SetItem(c, 0, val), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr sideeffect(moduleGet("__main__", "sideeffect"));
  EXPECT_EQ(PyLong_AsLong(sideeffect), 10);
}

TEST_F(AbstractExtensionApiTest, PySequenceDelItemCallsDunderDelItem) {
  PyRun_SimpleString(R"(
sideeffect = 0
class C:
  def __delitem__(self, key):
    global sideeffect
    sideeffect = 10
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  ASSERT_EQ(PySequence_DelItem(c, 0), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr sideeffect(moduleGet("__main__", "sideeffect"));
  EXPECT_EQ(PyLong_AsLong(sideeffect), 10);
}

TEST_F(AbstractExtensionApiTest, PySequenceContainsCallsDunderContains) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    pass
  def __contains__(self, key):
    return True
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr key(PyLong_FromLong(3));
  ASSERT_EQ(PySequence_Contains(c, key), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PySequenceContainsPropagatesException) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    pass
  def __contains__(self, key):
    raise ValueError
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr key(PyLong_FromLong(3));
  ASSERT_EQ(PySequence_Contains(c, key), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(AbstractExtensionApiTest, PySequenceContainsFallsBackToIterSearch) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    pass
  def __iter__(self):
    return [1,2,3].__iter__()
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr key(PyLong_FromLong(4));
  ASSERT_EQ(PySequence_Contains(c, key), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PySequenceIndexWithNullObjRaises) {
  PyObjectPtr tuple(PyTuple_Pack(2, PyLong_FromLong(1), PyLong_FromLong(2)));
  EXPECT_EQ(PySequence_Index(tuple, nullptr), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PySequenceIndexFindsFirstOccurrence) {
  PyObjectPtr obj(PyLong_FromLong(2));
  PyObjectPtr tuple(PyTuple_Pack(3, PyLong_FromLong(1), PyLong_FromLong(2),
                                 PyLong_FromLong(2)));
  EXPECT_EQ(PySequence_Index(tuple, obj), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PySequenceFastWithNullObjRaisesSystemError) {
  EXPECT_EQ(PySequence_Fast(nullptr, "msg"), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PySequenceFastWithTupleReturnsSameObject) {
  PyObjectPtr tuple(PyTuple_New(3));
  PyObjectPtr result(PySequence_Fast(tuple, "msg"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(tuple, result);
}

TEST_F(AbstractExtensionApiTest, PySequenceFastWithListReturnsSameObject) {
  PyObjectPtr list(PyList_New(3));
  PyObjectPtr result(PySequence_Fast(list, "msg"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(list, result);
}

TEST_F(AbstractExtensionApiTest, PySequenceFastWithNonIterableRaisesTypeError) {
  ASSERT_EQ(PySequence_Fast(Py_None, "msg"), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PySequenceFastWithIterableReturnsList) {
  PyRun_SimpleString(R"(
class C:
  def __iter__(self):
    return (1, 2, 3).__iter__()
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PySequence_Fast(c, "msg"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyList_CheckExact(result));
}

TEST_F(AbstractExtensionApiTest, PySequenceListWithNullSeqRaisesSystemError) {
  ASSERT_EQ(PySequence_List(nullptr), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PySequenceListWithNonIterableRaisesTypeError) {
  ASSERT_EQ(PySequence_List(Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PySequenceListReturnsList) {
  PyRun_SimpleString(R"(
class C:
  def __iter__(self):
    return (1, 2, 3).__iter__()
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PySequence_List(c));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyList_CheckExact(result));
  EXPECT_EQ(PyList_Size(result), 3);
}

TEST_F(AbstractExtensionApiTest,
       PySequenceGetSliceWithNullSeqRaisesSystemError) {
  EXPECT_EQ(PySequence_GetSlice(nullptr, 1, 2), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       PySequenceGetSliceWithNonIterableRaisesTypeError) {
  EXPECT_EQ(PySequence_GetSlice(Py_None, 1, 2), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PySequenceGetSliceCallsDunderGetItem) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    return 7
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PySequence_GetSlice(c, 1, 2));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyLong_Check(result));
  EXPECT_EQ(PyLong_AsLong(result), 7);
}

TEST_F(AbstractExtensionApiTest,
       PySequenceSetSliceWithNullSeqRaisesSystemError) {
  PyObjectPtr obj(PyList_New(0));
  EXPECT_EQ(PySequence_SetSlice(nullptr, 1, 2, obj), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       PySequenceSetSliceWithNonIterableRaisesTypeError) {
  PyObjectPtr obj(PyList_New(0));
  EXPECT_EQ(PySequence_SetSlice(Py_None, 1, 2, obj), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PySequenceSetSliceCallsDunderSetItem) {
  PyRun_SimpleString(R"(
sideeffect = 0
class C:
  def __setitem__(self, key, value):
    global sideeffect
    sideeffect = 10
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr obj(PyList_New(0));
  ASSERT_EQ(PySequence_SetSlice(c, 1, 2, obj), 0);
  PyObjectPtr sideeffect(moduleGet("__main__", "sideeffect"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsLong(sideeffect), 10);
}

TEST_F(AbstractExtensionApiTest,
       PySequenceSetSliceWithNullObjCallsDunderDelItem) {
  PyRun_SimpleString(R"(
sideeffect = 0
class C:
  def __delitem__(self, key):
    global sideeffect
    sideeffect = 10
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PySequence_SetSlice(c, 1, 2, nullptr), 0);
  PyObjectPtr sideeffect(moduleGet("__main__", "sideeffect"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsLong(sideeffect), 10);
}

TEST_F(AbstractExtensionApiTest,
       PySequenceDelSliceWithNullSeqRaisesSystemError) {
  PyObjectPtr obj(PyList_New(0));
  EXPECT_EQ(PySequence_DelSlice(nullptr, 1, 2), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       PySequenceDelSliceWithNonIterableRaisesTypeError) {
  PyObjectPtr obj(PyList_New(0));
  EXPECT_EQ(PySequence_DelSlice(Py_None, 1, 2), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PySequenceDelSliceCallsDunderDelItem) {
  PyRun_SimpleString(R"(
sideeffect = 0
class C:
  def __delitem__(self, key):
    global sideeffect
    sideeffect = 10
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PySequence_DelSlice(c, 1, 2), 0);
  PyObjectPtr sideeffect(moduleGet("__main__", "sideeffect"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsLong(sideeffect), 10);
}

TEST_F(AbstractExtensionApiTest, PySequenceTupleWithNullSeqRaisesSystemError) {
  EXPECT_EQ(PySequence_Tuple(nullptr), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       PySequenceTupleWithNonIterableRaisesTypeError) {
  EXPECT_EQ(PySequence_Tuple(Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PySequenceTupleReturnsTuple) {
  PyRun_SimpleString(R"(
class C:
  def __iter__(self):
    return [1, 2, 3].__iter__()
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PySequence_Tuple(c));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyTuple_CheckExact(result));
  EXPECT_EQ(PyTuple_Size(result), 3);
}

TEST_F(AbstractExtensionApiTest, ObjectGetItemWithNullObjRaisesSystemError) {
  PyObjectPtr obj(PyLong_FromLong(1));
  EXPECT_EQ(PyObject_GetItem(nullptr, obj), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, ObjectGetItemWithNullKeyRaisesSystemError) {
  PyObjectPtr obj(PyLong_FromLong(1));
  EXPECT_EQ(PyObject_GetItem(obj, nullptr), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       ObjectGetItemWithNoDunderGetItemRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  pass
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr key(PyLong_FromLong(7));
  EXPECT_EQ(PyObject_GetItem(c, key), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest,
       ObjectGetItemWithUncallableDunderGetItemRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  __getitem__ = 4
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr key(PyLong_FromLong(7));
  EXPECT_EQ(PyObject_GetItem(c, key), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, ObjectGetItemCallsDunderGetItem) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    return key
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr key(PyLong_FromLong(7));
  EXPECT_EQ(PyObject_GetItem(c, key), key);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, ObjectGetItemPropagatesException) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    raise IndexError
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr key(PyLong_FromLong(7));
  EXPECT_EQ(PyObject_GetItem(c, key), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_IndexError));
}

TEST_F(AbstractExtensionApiTest,
       MappingGetItemStringWithNullObjRaisesSystemError) {
  EXPECT_EQ(PyMapping_GetItemString(nullptr, "hello"), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       MappingGetItemStringWithNullKeyRaisesSystemError) {
  PyObjectPtr obj(PyLong_FromLong(1));
  EXPECT_EQ(PyMapping_GetItemString(obj, nullptr), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       MappingGetItemStringWithNoDunderGetItemRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  pass
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyMapping_GetItemString(c, "hello"), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest,
       MappingGetItemStringWithUncallableDunderGetItemRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  __getitem__ = 4
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyMapping_GetItemString(c, "hello"), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, MappingGetItemStringCallsDunderGetItem) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    return key
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  const char* key = "hello";
  PyObjectPtr result(PyMapping_GetItemString(c, key));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(result, key));
}

TEST_F(AbstractExtensionApiTest, MappingGetItemStringPropagatesException) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    raise IndexError
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyMapping_GetItemString(c, "hello"), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_IndexError));
}

TEST_F(AbstractExtensionApiTest, MappingHasKeyWithNullObjReturnsFalse) {
  PyObjectPtr obj(PyLong_FromLong(7));
  EXPECT_EQ(PyMapping_HasKey(nullptr, obj), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, MappingHasKeyWithNullKeyReturnsFalse) {
  PyObjectPtr obj(PyLong_FromLong(7));
  EXPECT_EQ(PyMapping_HasKey(obj, nullptr), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, MappingHasKeyCallsDunderGetItem) {
  PyRun_SimpleString(R"(
sideeffect = 0
class C:
  def __getitem__(self, key):
    global sideeffect
    sideeffect = 10
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr key(PyLong_FromLong(7));
  ASSERT_EQ(PyMapping_HasKey(c, key), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr sideeffect(moduleGet("__main__", "sideeffect"));
  EXPECT_EQ(PyLong_AsLong(sideeffect), 10);
}

TEST_F(AbstractExtensionApiTest,
       MappingHasKeyReturnsFalseWhenExceptionIsRaised) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    raise IndexError
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr key(PyLong_FromLong(7));
  EXPECT_EQ(PyMapping_HasKey(c, key), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, MappingHasKeyStringWithNullObjReturnsFalse) {
  EXPECT_EQ(PyMapping_HasKeyString(nullptr, "hello"), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, MappingHasKeyStringWithNullKeyReturnsFalse) {
  PyObjectPtr obj(PyLong_FromLong(7));
  EXPECT_EQ(PyMapping_HasKeyString(obj, nullptr), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, MappingHasKeyStringCallsDunderGetItem) {
  PyRun_SimpleString(R"(
sideeffect = 0
class C:
  def __getitem__(self, key):
    global sideeffect
    sideeffect = 10
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  ASSERT_EQ(PyMapping_HasKeyString(c, "hello"), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr sideeffect(moduleGet("__main__", "sideeffect"));
  EXPECT_EQ(PyLong_AsLong(sideeffect), 10);
}

TEST_F(AbstractExtensionApiTest,
       MappingHasKeyStringReturnsFalseWhenExceptionIsRaised) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    raise IndexError
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyMapping_HasKeyString(c, "hello"), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, MappingKeysWithNoKeysRaisesAttributeError) {
  PyRun_SimpleString(R"(
class C:
  pass
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyMapping_Keys(c), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_AttributeError));
}

TEST_F(AbstractExtensionApiTest, MappingKeysCallsReturnsListOfKeys) {
  PyRun_SimpleString(R"(
class C:
  def keys(self):
    return ["hello", "world"]
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyMapping_Keys(c));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyList_Check(result));
  ASSERT_EQ(PyList_Size(result), 2);
  EXPECT_TRUE(isUnicodeEqualsCStr(PyList_GetItem(result, 0), "hello"));
  EXPECT_TRUE(isUnicodeEqualsCStr(PyList_GetItem(result, 1), "world"));
}

TEST_F(AbstractExtensionApiTest, MappingKeysCallsReturnsListOfKeysSequence) {
  PyRun_SimpleString(R"(
class C:
  def keys(self):
    return ("hello", "world").__iter__()
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyMapping_Keys(c));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyList_Check(result));
  ASSERT_EQ(PyList_Size(result), 2);
  EXPECT_TRUE(isUnicodeEqualsCStr(PyList_GetItem(result, 0), "hello"));
  EXPECT_TRUE(isUnicodeEqualsCStr(PyList_GetItem(result, 1), "world"));
}

TEST_F(AbstractExtensionApiTest, MappingKeysWithDictSubclassCallsKeys) {
  PyRun_SimpleString(R"(
class C(dict):
  def keys(self):
    return ("hello", "world").__iter__()
c = C()
c["a"] = 1
c["b"] = 2
c["c"] = 3
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyMapping_Keys(c));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyList_Check(result));
  ASSERT_EQ(PyList_Size(result), 2);
  EXPECT_TRUE(isUnicodeEqualsCStr(PyList_GetItem(result, 0), "hello"));
  EXPECT_TRUE(isUnicodeEqualsCStr(PyList_GetItem(result, 1), "world"));
}

TEST_F(AbstractExtensionApiTest, MappingItemsWithNoItemsRaisesAttributeError) {
  PyRun_SimpleString(R"(
class C:
  pass
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyMapping_Items(c), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_AttributeError));
}

TEST_F(AbstractExtensionApiTest, MappingItemsCallsReturnsListOfItems) {
  PyRun_SimpleString(R"(
class C:
  def items(self):
    return ["hello", "world"]
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyMapping_Items(c));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyList_Check(result));
  ASSERT_EQ(PyList_Size(result), 2);
  EXPECT_TRUE(isUnicodeEqualsCStr(PyList_GetItem(result, 0), "hello"));
  EXPECT_TRUE(isUnicodeEqualsCStr(PyList_GetItem(result, 1), "world"));
}

TEST_F(AbstractExtensionApiTest, MappingItemsCallsReturnsListOfItemsSequence) {
  PyRun_SimpleString(R"(
class C:
  def items(self):
    return ("hello", "world").__iter__()
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyMapping_Items(c));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyList_Check(result));
  ASSERT_EQ(PyList_Size(result), 2);
  EXPECT_TRUE(isUnicodeEqualsCStr(PyList_GetItem(result, 0), "hello"));
  EXPECT_TRUE(isUnicodeEqualsCStr(PyList_GetItem(result, 1), "world"));
}

TEST_F(AbstractExtensionApiTest, MappingItemsWithDictSubclassCallsItems) {
  PyRun_SimpleString(R"(
class C(dict):
  def items(self):
    return ("hello", "world").__iter__()
c = C()
c["a"] = 1
c["b"] = 2
c["c"] = 3
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyMapping_Items(c));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyList_Check(result));
  ASSERT_EQ(PyList_Size(result), 2);
  EXPECT_TRUE(isUnicodeEqualsCStr(PyList_GetItem(result, 0), "hello"));
  EXPECT_TRUE(isUnicodeEqualsCStr(PyList_GetItem(result, 1), "world"));
}

TEST_F(AbstractExtensionApiTest,
       MappingValuesWithNoValuesRaisesAttributeError) {
  PyRun_SimpleString(R"(
class C:
  pass
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyMapping_Values(c), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_AttributeError));
}

TEST_F(AbstractExtensionApiTest, MappingValuesCallsReturnsListOfValues) {
  PyRun_SimpleString(R"(
class C:
  def values(self):
    return ["hello", "world"]
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyMapping_Values(c));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyList_Check(result));
  ASSERT_EQ(PyList_Size(result), 2);
  EXPECT_TRUE(isUnicodeEqualsCStr(PyList_GetItem(result, 0), "hello"));
  EXPECT_TRUE(isUnicodeEqualsCStr(PyList_GetItem(result, 1), "world"));
}

TEST_F(AbstractExtensionApiTest,
       MappingValuesCallsReturnsListOfValuesSequence) {
  PyRun_SimpleString(R"(
class C:
  def values(self):
    return ("hello", "world").__iter__()
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyMapping_Values(c));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyList_Check(result));
  ASSERT_EQ(PyList_Size(result), 2);
  EXPECT_TRUE(isUnicodeEqualsCStr(PyList_GetItem(result, 0), "hello"));
  EXPECT_TRUE(isUnicodeEqualsCStr(PyList_GetItem(result, 1), "world"));
}

TEST_F(AbstractExtensionApiTest, MappingValuesWithDictSubclassCallsValues) {
  PyRun_SimpleString(R"(
class C(dict):
  def values(self):
    return ("hello", "world").__iter__()
c = C()
c["a"] = 1
c["b"] = 2
c["c"] = 3
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyMapping_Values(c));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyList_Check(result));
  ASSERT_EQ(PyList_Size(result), 2);
  EXPECT_TRUE(isUnicodeEqualsCStr(PyList_GetItem(result, 0), "hello"));
  EXPECT_TRUE(isUnicodeEqualsCStr(PyList_GetItem(result, 1), "world"));
}

TEST_F(AbstractExtensionApiTest, ObjectSetItemWithNullObjRaisesSystemError) {
  PyObjectPtr obj(PyLong_FromLong(1));
  EXPECT_EQ(PyObject_SetItem(nullptr, obj, obj), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, ObjectSetItemWithNullKeyRaisesSystemError) {
  PyObjectPtr obj(PyLong_FromLong(1));
  EXPECT_EQ(PyObject_SetItem(obj, nullptr, obj), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, ObjectSetItemWithNullValueRaisesSystemError) {
  PyObjectPtr obj(PyLong_FromLong(1));
  EXPECT_EQ(PyObject_SetItem(obj, obj, nullptr), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       ObjectSetItemWithNoDunderSetItemRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  pass
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr obj(PyLong_FromLong(1));
  EXPECT_EQ(PyObject_SetItem(c, obj, obj), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest,
       ObjectSetItemWithUncallableDunderSetItemRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  __setitem__ = 4
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr obj(PyLong_FromLong(1));
  EXPECT_EQ(PyObject_SetItem(c, obj, obj), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, ObjectSetItemCallsDunderSetItem) {
  PyRun_SimpleString(R"(
sideeffect = 0
class C:
  def __setitem__(self, key, val):
    global sideeffect
    sideeffect = 10
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr obj(PyLong_FromLong(1));
  ASSERT_EQ(PyObject_SetItem(c, obj, obj), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr sideeffect(moduleGet("__main__", "sideeffect"));
  EXPECT_EQ(PyLong_AsLong(sideeffect), 10);
}

TEST_F(AbstractExtensionApiTest, ObjectSetItemPropagatesException) {
  PyRun_SimpleString(R"(
class C:
  def __setitem__(self, key, value):
    raise IndexError
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr obj(PyLong_FromLong(7));
  EXPECT_EQ(PyObject_SetItem(c, obj, obj), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_IndexError));
}

TEST_F(AbstractExtensionApiTest,
       MappingSetItemStringWithNullObjRaisesSystemError) {
  PyObjectPtr obj(PyLong_FromLong(1));
  EXPECT_EQ(PyMapping_SetItemString(nullptr, "hello", obj), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       MappingSetItemStringWithNullKeyRaisesSystemError) {
  PyObjectPtr obj(PyLong_FromLong(1));
  EXPECT_EQ(PyMapping_SetItemString(obj, nullptr, obj), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       MappingSetItemStringWithNullValueRaisesSystemError) {
  PyObjectPtr obj(PyLong_FromLong(1));
  EXPECT_EQ(PyMapping_SetItemString(obj, "hello", nullptr), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       MappingSetItemStringWithNoDunderSetItemRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  pass
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr obj(PyLong_FromLong(1));
  EXPECT_EQ(PyMapping_SetItemString(c, "hello", obj), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest,
       MappingSetItemStringWithUncallableDunderSetItemRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  __setitem__ = 4
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr obj(PyLong_FromLong(1));
  EXPECT_EQ(PyMapping_SetItemString(c, "hello", obj), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, MappingSetItemStringCallsDunderSetItem) {
  PyRun_SimpleString(R"(
sideeffect = 0
class C:
  def __setitem__(self, key, val):
    global sideeffect
    sideeffect = 10
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr obj(PyLong_FromLong(1));
  ASSERT_EQ(PyMapping_SetItemString(c, "hello", obj), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr sideeffect(moduleGet("__main__", "sideeffect"));
  EXPECT_EQ(PyLong_AsLong(sideeffect), 10);
}

TEST_F(AbstractExtensionApiTest, MappingSetItemStringPropagatesException) {
  PyRun_SimpleString(R"(
class C:
  def __setitem__(self, key, value):
    raise IndexError
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr obj(PyLong_FromLong(7));
  EXPECT_EQ(PyMapping_SetItemString(c, "hello", obj), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_IndexError));
}

TEST_F(AbstractExtensionApiTest,
       ObjectFormatWithNonStrFormatSpecRaisesTypeErrorPyro) {
  EXPECT_EQ(PyObject_Format(Py_None, Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, ObjectFormatCallsDunderFormat) {
  PyRun_SimpleString(R"(
sideeffect = 0
class C:
  def __format__(self, format_spec):
    global sideeffect
    sideeffect = 10
    return "foo"
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyObject_Format(c, nullptr));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "foo"));
  PyObjectPtr sideeffect(moduleGet("__main__", "sideeffect"));
  EXPECT_EQ(PyLong_AsLong(sideeffect), 10);
}

TEST_F(AbstractExtensionApiTest,
       ObjectFormatWithDunderFormatReturningNonStrRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  def __format__(self, format_spec):
    return 7
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyObject_Format(c, nullptr));
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

}  // namespace python
