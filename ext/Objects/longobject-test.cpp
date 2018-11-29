#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using LongExtensionApiTest = ExtensionApi;

TEST_F(LongExtensionApiTest, PyLongCheckOnInt) {
  PyObjectPtr pylong(PyLong_FromLong(10));
  EXPECT_TRUE(PyLong_Check(pylong));
  EXPECT_TRUE(PyLong_CheckExact(pylong));

  pylong = PyLong_FromLongLong(10);
  EXPECT_TRUE(PyLong_Check(pylong));
  EXPECT_TRUE(PyLong_CheckExact(pylong));

  pylong = PyLong_FromUnsignedLong(10);
  EXPECT_TRUE(PyLong_Check(pylong));
  EXPECT_TRUE(PyLong_CheckExact(pylong));

  pylong = PyLong_FromUnsignedLongLong(10);
  EXPECT_TRUE(PyLong_Check(pylong));
  EXPECT_TRUE(PyLong_CheckExact(pylong));

  pylong = PyLong_FromSsize_t(10);
  EXPECT_TRUE(PyLong_Check(pylong));
  EXPECT_TRUE(PyLong_CheckExact(pylong));
}

TEST_F(LongExtensionApiTest, PyLongCheckOnType) {
  PyObjectPtr pylong(PyLong_FromLong(10));
  PyObjectPtr type(reinterpret_cast<PyObject*>(Py_TYPE(pylong)));
  EXPECT_FALSE(PyLong_Check(type));
  EXPECT_FALSE(PyLong_CheckExact(type));
}

TEST_F(LongExtensionApiTest, AsLongWithNullReturnsNegative) {
  long res = PyLong_AsLong(nullptr);
  EXPECT_EQ(res, -1);

  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(LongExtensionApiTest, AsLongWithNonIntegerReturnsNegative) {
  long res = PyLong_AsLong(Py_None);
  EXPECT_EQ(res, -1);
  // TODO(eelizondo): Add exception checks once PyLong_AsLong is fully
  // implemented
}

TEST_F(LongExtensionApiTest, FromLongReturnsLong) {
  const int val = 10;
  PyObjectPtr pylong(PyLong_FromLong(val));
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  EXPECT_EQ(PyLong_AsLong(pylong), val);
  EXPECT_EQ(PyLong_AsLongLong(pylong), val);
  EXPECT_EQ(PyLong_AsSsize_t(pylong), val);

  auto const val2 = std::numeric_limits<long>::min();
  PyObjectPtr pylong2(PyLong_FromLong(val2));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsLong(pylong2), val2);

  auto const val3 = std::numeric_limits<long>::max();
  PyObjectPtr pylong3(PyLong_FromLong(val3));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsLong(pylong3), val3);
}

TEST_F(LongExtensionApiTest, FromUnsignedReturnsLong) {
  auto const ulmax = std::numeric_limits<unsigned long>::max();
  PyObjectPtr pylong(PyLong_FromUnsignedLong(ulmax));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsUnsignedLong(pylong), ulmax);
  EXPECT_EQ(PyLong_AsUnsignedLongLong(pylong), ulmax);
  EXPECT_EQ(PyLong_AsSize_t(pylong), ulmax);

  auto const ullmax = std::numeric_limits<unsigned long long>::max();
  PyObjectPtr pylong2(PyLong_FromUnsignedLongLong(ullmax));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsUnsignedLongLong(pylong2), ullmax);

  auto const uval = 1234UL;
  PyObjectPtr pylong3(PyLong_FromUnsignedLong(uval));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsUnsignedLong(pylong3), uval);
}

static PyObject* lshift(long num, long shift) {
  PyObject* num_obj = PyLong_FromLong(num);
  PyObject* shift_obj = PyLong_FromLong(shift);
  PyObject* result = PyNumber_Lshift(num_obj, shift_obj);
  Py_DECREF(num_obj);
  Py_DECREF(shift_obj);
  return result;
}

TEST_F(LongExtensionApiTest, Overflow) {
  PyObjectPtr pylong(lshift(1, 100));

  EXPECT_EQ(PyLong_AsUnsignedLong(pylong), -1UL);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_OverflowError));
  PyErr_Clear();

  EXPECT_EQ(PyLong_AsLong(pylong), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_OverflowError));
  PyErr_Clear();

  EXPECT_EQ(PyLong_AsSsize_t(pylong), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_OverflowError));
  PyErr_Clear();

  pylong = PyLong_FromLong(-123);
  EXPECT_EQ(PyLong_AsUnsignedLongLong(pylong), -1ULL);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_OverflowError));
}

TEST_F(LongExtensionApiTest, AsLongAndOverflow) {
  auto const ulmax = std::numeric_limits<unsigned long>::max();
  auto const lmax = std::numeric_limits<long>::max();

  PyObjectPtr pylong(PyLong_FromUnsignedLong(ulmax));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  int overflow = 0;
  EXPECT_EQ(PyLong_AsLongAndOverflow(pylong, &overflow), -1);
  EXPECT_EQ(overflow, 1);
  overflow = 0;
  EXPECT_EQ(PyLong_AsLongLongAndOverflow(pylong, &overflow), -1);
  EXPECT_EQ(overflow, 1);

  pylong = PyLong_FromLong(lmax);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  overflow = 1;
  EXPECT_EQ(PyLong_AsLongAndOverflow(pylong, &overflow), lmax);
  EXPECT_EQ(overflow, 0);
  overflow = 1;
  EXPECT_EQ(PyLong_AsLongLongAndOverflow(pylong, &overflow), lmax);
  EXPECT_EQ(overflow, 0);

  pylong = lshift(-1, 100);
  overflow = 0;
  EXPECT_EQ(PyLong_AsLongAndOverflow(pylong, &overflow), -1);
  EXPECT_EQ(overflow, -1);
  overflow = 0;
  EXPECT_EQ(PyLong_AsLongLongAndOverflow(pylong, &overflow), -1);
  EXPECT_EQ(overflow, -1);
}

TEST_F(LongExtensionApiTest, AsUnsignedLongMaskWithMax) {
  auto const ulmax = std::numeric_limits<unsigned long>::max();
  PyObjectPtr pylong(PyLong_FromUnsignedLong(ulmax));
  EXPECT_EQ(PyLong_AsUnsignedLongMask(pylong), ulmax);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsUnsignedLongLongMask(pylong), ulmax);
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  auto const ullmax = std::numeric_limits<unsigned long long>::max();
  pylong = PyLong_FromUnsignedLongLong(ullmax);
  EXPECT_EQ(PyLong_AsUnsignedLongLongMask(pylong), ullmax);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(LongExtensionApiTest, AsUnsignedLongMaskWithLargeInt) {
  PyObjectPtr largeint(lshift(1, 100));
  PyObjectPtr pylong(PyNumber_Or(largeint, PyObjectPtr(PyLong_FromLong(123))));
  EXPECT_EQ(PyLong_AsUnsignedLongMask(pylong), 123UL);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsUnsignedLongLongMask(pylong), 123ULL);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(LongExtensionApiTest, AsUnsignedLongMaskWithNegative) {
  PyObjectPtr pylong(PyLong_FromLong(-17));
  EXPECT_EQ(PyLong_AsUnsignedLongMask(pylong), -17UL);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsUnsignedLongLongMask(pylong), -17ULL);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

}  // namespace python
