#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using SliceExtensionApiTest = ExtensionApi;

TEST_F(SliceExtensionApiTest, NewReturnsSlice) {
  PyObjectPtr slice(PySlice_New(nullptr, nullptr, nullptr));
  EXPECT_TRUE(PySlice_Check(slice));
}

TEST_F(SliceExtensionApiTest, AdjustIndicesReturnsSliceLength) {
  Py_ssize_t length = 10;
  Py_ssize_t start = 2;
  Py_ssize_t stop = -1;
  Py_ssize_t step = 3;
  Py_ssize_t slice_length = PySlice_AdjustIndices(length, &start, &stop, step);
  EXPECT_EQ(slice_length, 3);
  EXPECT_EQ(start, 2);
  EXPECT_EQ(stop, 9);
}

TEST_F(SliceExtensionApiTest, AdjustIndicesWithNegativeShifts) {
  Py_ssize_t length = 5;
  Py_ssize_t start = -4;
  Py_ssize_t stop = -1;
  Py_ssize_t step = 1;
  Py_ssize_t slice_length = PySlice_AdjustIndices(length, &start, &stop, step);
  EXPECT_EQ(slice_length, 3);
  EXPECT_EQ(start, 1);
  EXPECT_EQ(stop, 4);
}

TEST_F(SliceExtensionApiTest,
       AdjustIndicesWithLargeNegativesAndPositiveStepSetsZero) {
  Py_ssize_t length = 5;
  Py_ssize_t start = -40;
  Py_ssize_t stop = -10;
  Py_ssize_t step = 2;
  Py_ssize_t slice_length = PySlice_AdjustIndices(length, &start, &stop, step);
  EXPECT_EQ(slice_length, 0);
  EXPECT_EQ(start, 0);
  EXPECT_EQ(stop, 0);
}

TEST_F(SliceExtensionApiTest,
       AdjustIndicesWithLargeNegativesAndNegativeStepSetsNegativeOne) {
  Py_ssize_t length = 5;
  Py_ssize_t start = -40;
  Py_ssize_t stop = -100;
  Py_ssize_t step = -2;
  Py_ssize_t slice_length = PySlice_AdjustIndices(length, &start, &stop, step);
  EXPECT_EQ(slice_length, 0);
  EXPECT_EQ(start, -1);
  EXPECT_EQ(stop, -1);
}

TEST_F(SliceExtensionApiTest, AdjustIndicesWithLargeIndicesSetsLength) {
  Py_ssize_t length = 5;
  Py_ssize_t start = 8;
  Py_ssize_t stop = 10;
  Py_ssize_t step = 1;
  Py_ssize_t slice_length = PySlice_AdjustIndices(length, &start, &stop, step);
  EXPECT_EQ(slice_length, 0);
  EXPECT_EQ(start, 5);
  EXPECT_EQ(stop, 5);
}

TEST_F(SliceExtensionApiTest,
       AdjustIndicesWithLargeIndicesAndNegativeStepSetsOffsetLength) {
  Py_ssize_t length = 5;
  Py_ssize_t start = 8;
  Py_ssize_t stop = 10;
  Py_ssize_t step = -1;
  Py_ssize_t slice_length = PySlice_AdjustIndices(length, &start, &stop, step);
  EXPECT_EQ(slice_length, 0);
  EXPECT_EQ(start, 4);
  EXPECT_EQ(stop, 4);
}

TEST_F(SliceExtensionApiTest, GetIndicesExWithUnpackErrorRaisesValueError) {
  PyObjectPtr zero(PyLong_FromLong(0));
  PyObjectPtr slice(PySlice_New(nullptr, nullptr, zero));
  Py_ssize_t length = 0;
  Py_ssize_t start, stop, step, slicelength;
  int result =
      PySlice_GetIndicesEx(slice, length, &start, &stop, &step, &slicelength);
  ASSERT_EQ(result, -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(SliceExtensionApiTest, GetIndicesExWithUnpackSuccessSetsValues) {
  PyObjectPtr two(PyLong_FromLong(2));
  PyObjectPtr slice(PySlice_New(nullptr, nullptr, two));
  Py_ssize_t length = 10;
  Py_ssize_t start, stop, step, slicelength;
  int result =
      PySlice_GetIndicesEx(slice, length, &start, &stop, &step, &slicelength);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(start, 0);
  EXPECT_EQ(stop, 10);
  EXPECT_EQ(step, 2);
  EXPECT_EQ(slicelength, 5);
}

TEST_F(SliceExtensionApiTest, UnpackWithNonSliceRaisesSystemErrorPyro) {
  PyObjectPtr num(PyLong_FromLong(0));
  Py_ssize_t start, stop, step;
  ASSERT_EQ(PySlice_Unpack(num, &start, &stop, &step), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(SliceExtensionApiTest, UnpackWithNonIndexStartRaisesTypeError) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr slice(PySlice_New(list, nullptr, nullptr));
  Py_ssize_t start, stop, step;
  ASSERT_EQ(PySlice_Unpack(slice, &start, &stop, &step), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(SliceExtensionApiTest, UnpackWithNonIndexStopRaisesTypeError) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr slice(PySlice_New(nullptr, list, nullptr));
  Py_ssize_t start, stop, step;
  ASSERT_EQ(PySlice_Unpack(slice, &start, &stop, &step), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(SliceExtensionApiTest, UnpackWithNonIndexStepRaisesTypeError) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr slice(PySlice_New(nullptr, nullptr, list));
  Py_ssize_t start, stop, step;
  ASSERT_EQ(PySlice_Unpack(slice, &start, &stop, &step), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(SliceExtensionApiTest, UnpackWithZeroStepRaisesTypeError) {
  PyObjectPtr zero(PyLong_FromLong(0));
  PyObjectPtr slice(PySlice_New(nullptr, nullptr, zero));
  Py_ssize_t start, stop, step;
  ASSERT_EQ(PySlice_Unpack(slice, &start, &stop, &step), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(SliceExtensionApiTest, UnpackWithNoneSetsDefaults) {
  PyObjectPtr slice(PySlice_New(nullptr, nullptr, nullptr));
  Py_ssize_t start, stop, step;
  ASSERT_EQ(PySlice_Unpack(slice, &start, &stop, &step), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(start, 0);
  EXPECT_GT(stop, 1 << 30);  // Arbitrary large value less than
                             // PY_SSIZE_T_MAX and SmallInt::kMaxValue
  EXPECT_EQ(step, 1);
}

TEST_F(SliceExtensionApiTest, UnpackWithNonIntCallsDunderIndex) {
  PyRun_SimpleString(R"(
class Foo:
  def __init__(self): self.bar = 0
  def __index__(self):
    self.bar += 1
    return self.bar
foo = Foo()
)");
  PyObjectPtr foo(mainModuleGet("foo"));
  PyObjectPtr slice(PySlice_New(foo, foo, foo));
  Py_ssize_t start, stop, step;
  ASSERT_EQ(PySlice_Unpack(slice, &start, &stop, &step), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(start, 2);
  EXPECT_EQ(stop, 3);
  EXPECT_EQ(step, 1);
}

TEST_F(SliceExtensionApiTest, UnpackWithIndicesSetsValues) {
  PyObjectPtr idx1(PyLong_FromLong(1024));
  PyObjectPtr idx2(PyLong_FromLong(-42));
  PyObjectPtr idx3(PyLong_FromLong(10));
  PyObjectPtr slice(PySlice_New(idx1, idx2, idx3));
  Py_ssize_t start, stop, step;
  ASSERT_EQ(PySlice_Unpack(slice, &start, &stop, &step), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(start, 1024);
  EXPECT_GE(stop, -42);
  EXPECT_EQ(step, 10);
}

}  // namespace testing
}  // namespace py
