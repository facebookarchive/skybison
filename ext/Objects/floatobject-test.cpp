#include <cfloat>
#include <cmath>
#include <cstring>

#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using FloatExtensionApiTest = ExtensionApi;

TEST_F(FloatExtensionApiTest, PyFloatFromStringWithUnicodeReturnsFloat) {
  const char* str = "15.5";
  PyObjectPtr pyunicode(PyUnicode_FromString(str));
  PyObjectPtr flt(PyFloat_FromString(pyunicode));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyFloat_CheckExact(flt));
  EXPECT_EQ(PyFloat_AsDouble(flt), 15.5);
}

TEST_F(FloatExtensionApiTest, PyFloatFromStringWithBytesReturnsFloat) {
  const char* str = "25.5";
  PyObjectPtr pybytes(PyBytes_FromString(str));
  PyObjectPtr flt(PyFloat_FromString(pybytes));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyFloat_CheckExact(flt));
  EXPECT_EQ(PyFloat_AsDouble(flt), 25.5);
}

TEST_F(FloatExtensionApiTest, PyFloatFromStringWithMemoryViewReturnsFloat) {
  const char* str = "5.5";
  PyObjectPtr memory_view(
      PyMemoryView_FromMemory(const_cast<char*>(str), 3, PyBUF_READ));
  PyObjectPtr flt(PyFloat_FromString(memory_view));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyFloat_CheckExact(flt));
  EXPECT_EQ(PyFloat_AsDouble(flt), 5.5);
}

// TODO(T57022841): Needs PyFloatFromStringWithByteArrayReturnsFloat
// when bytearray support is added to float()

TEST_F(FloatExtensionApiTest, PyFloatFromStringWithUserBufferReturnsFloat) {
  static char contents[] = "45.5";
  static Py_ssize_t contents_len = std::strlen(contents);
  getbufferproc mocked_getbuffer_func = [](PyObject* obj, Py_buffer* view,
                                           int flags) {
    return PyBuffer_FillInfo(view, obj, contents, contents_len, /*readonly=*/1,
                             flags);
  };
  releasebufferproc mocked_releasebuffer_func = [](PyObject*, Py_buffer* view) {
    view->buf = nullptr;
    view->obj = nullptr;
  };
  PyType_Slot slots[] = {
      {Py_bf_getbuffer, reinterpret_cast<void*>(mocked_getbuffer_func)},
      {Py_bf_releasebuffer, reinterpret_cast<void*>(mocked_releasebuffer_func)},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr buf_type(PyType_FromSpec(&spec));
  ASSERT_NE(buf_type, nullptr);
  ASSERT_TRUE(PyType_CheckExact(buf_type));

  PyObjectPtr instance(PyObject_CallFunction(buf_type, nullptr));
  ASSERT_TRUE(PyObject_CheckBuffer(instance.get()));

  PyObjectPtr flt(PyFloat_FromString(instance));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyFloat_CheckExact(flt));
  EXPECT_EQ(PyFloat_AsDouble(flt), 45.5);
}

TEST_F(FloatExtensionApiTest,
       PyFloatFromStringWithUserBufferAndOnlyGetbufferReturnsFloat) {
  static char contents[] = "45.5";
  static Py_ssize_t contents_len = std::strlen(contents);
  getbufferproc mocked_getbuffer_func = [](PyObject* obj, Py_buffer* view,
                                           int flags) {
    return PyBuffer_FillInfo(view, obj, contents, contents_len, /*readonly=*/1,
                             flags);
  };
  PyType_Slot slots[] = {
      {Py_bf_getbuffer, reinterpret_cast<void*>(mocked_getbuffer_func)},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr buf_type(PyType_FromSpec(&spec));
  ASSERT_NE(buf_type, nullptr);
  ASSERT_TRUE(PyType_CheckExact(buf_type));

  PyObjectPtr instance(PyObject_CallFunction(buf_type, nullptr));
  ASSERT_TRUE(PyObject_CheckBuffer(instance.get()));

  PyObjectPtr flt(PyFloat_FromString(instance));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyFloat_CheckExact(flt));
  EXPECT_EQ(PyFloat_AsDouble(flt), 45.5);
}

TEST_F(FloatExtensionApiTest,
       PyFloatFromStringWithNonNullTermUserBufferReturnsFloat) {
  static char contents[] = "55.5 not null terminated";
  static Py_ssize_t contents_len = 4;  // Not the whole string
  getbufferproc mocked_getbuffer_func = [](PyObject* obj, Py_buffer* view,
                                           int flags) {
    return PyBuffer_FillInfo(view, obj, contents, contents_len, /*readonly*/ 1,
                             flags);
  };
  releasebufferproc mocked_releasebuffer_func = [](PyObject*, Py_buffer* view) {
    view->buf = nullptr;
    view->obj = nullptr;
  };
  PyType_Slot slots[] = {
      {Py_bf_getbuffer, reinterpret_cast<void*>(mocked_getbuffer_func)},
      {Py_bf_releasebuffer, reinterpret_cast<void*>(mocked_releasebuffer_func)},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr buf_type(PyType_FromSpec(&spec));
  ASSERT_NE(buf_type, nullptr);
  ASSERT_TRUE(PyType_CheckExact(buf_type));

  PyObjectPtr instance(PyObject_CallFunction(buf_type, nullptr));
  ASSERT_TRUE(PyObject_CheckBuffer(instance.get()));

  PyObjectPtr flt(PyFloat_FromString(instance));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyFloat_CheckExact(flt));
  EXPECT_EQ(PyFloat_AsDouble(flt), 55.5);
}

TEST_F(FloatExtensionApiTest,
       PyFloatFromStringWithEmbededNullTermUserBufferRaisesValueError) {
  static char contents[] = "55.5\0 embeded null";
  static Py_ssize_t contents_len = 18;  // Include the null and more...
  getbufferproc mocked_getbuffer_func = [](PyObject* obj, Py_buffer* view,
                                           int flags) {
    return PyBuffer_FillInfo(view, obj, contents, contents_len, /*readonly*/ 1,
                             flags);
  };
  releasebufferproc mocked_releasebuffer_func = [](PyObject*, Py_buffer* view) {
    view->buf = nullptr;
    view->obj = nullptr;
  };
  PyType_Slot slots[] = {
      {Py_bf_getbuffer, reinterpret_cast<void*>(mocked_getbuffer_func)},
      {Py_bf_releasebuffer, reinterpret_cast<void*>(mocked_releasebuffer_func)},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr buf_type(PyType_FromSpec(&spec));
  ASSERT_NE(buf_type, nullptr);
  ASSERT_TRUE(PyType_CheckExact(buf_type));

  PyObjectPtr instance(PyObject_CallFunction(buf_type, nullptr));
  ASSERT_TRUE(PyObject_CheckBuffer(instance.get()));

  PyObjectPtr flt(PyFloat_FromString(instance));
  EXPECT_FALSE(flt);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(FloatExtensionApiTest,
       PyFloatFromStringWithUserBufferAndRaisingGetBufferRaisesTypeError) {
  getbufferproc mocked_getbuffer_func = [](PyObject*, Py_buffer*, int) {
    PyErr_Format(PyExc_NotImplementedError, "not implemented");
    return -1;
  };
  releasebufferproc mocked_releasebuffer_func = [](PyObject*, Py_buffer* view) {
    view->buf = nullptr;
    view->obj = nullptr;
  };
  PyType_Slot slots[] = {
      {Py_bf_getbuffer, reinterpret_cast<void*>(mocked_getbuffer_func)},
      {Py_bf_releasebuffer, reinterpret_cast<void*>(mocked_releasebuffer_func)},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr buf_type(PyType_FromSpec(&spec));
  ASSERT_NE(buf_type, nullptr);
  ASSERT_TRUE(PyType_CheckExact(buf_type));

  PyObjectPtr instance(PyObject_CallFunction(buf_type, nullptr));
  ASSERT_TRUE(PyObject_CheckBuffer(instance.get()));

  PyObjectPtr flt(PyFloat_FromString(instance));
  EXPECT_FALSE(flt);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(FloatExtensionApiTest, PyFloatFromStringWithIntRaisesTypeError) {
  PyObjectPtr integer(PyLong_FromLong(100));
  EXPECT_EQ(PyFloat_FromString(integer), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(FloatExtensionApiTest, PyFloatFromStringWithFloatRaisesTypeError) {
  PyObjectPtr flt(PyFloat_FromDouble(1.5));
  EXPECT_EQ(PyFloat_FromString(flt), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(FloatExtensionApiTest, PyFloatFromStringWithNoneRaisesTypeError) {
  EXPECT_EQ(PyFloat_FromString(Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(FloatExtensionApiTest, FromDoubleReturnsFloat) {
  const double val = 15.4;
  PyObjectPtr flt(PyFloat_FromDouble(val));
  ASSERT_TRUE(PyFloat_CheckExact(flt));
  EXPECT_EQ(PyFloat_AsDouble(flt), val);
}

TEST_F(FloatExtensionApiTest, NegativeFromDoubleReturnsFloat) {
  const double val = -10000.123;
  PyObjectPtr flt(PyFloat_FromDouble(val));
  EXPECT_TRUE(PyFloat_CheckExact(flt));
  EXPECT_EQ(PyFloat_AsDouble(flt), val);
}

TEST_F(FloatExtensionApiTest, AsDoubleFromNullRaisesException) {
  double res = PyFloat_AsDouble(nullptr);
  EXPECT_EQ(res, -1);

  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(FloatExtensionApiTest, AsDoubleFromNonFloatRaisesException) {
  PyObjectPtr list(PyList_New(0));
  double res = PyFloat_AsDouble(list);
  EXPECT_EQ(res, -1);

  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(FloatExtensionApiTest, AsDoubleCallsDunderFloat) {
  PyRun_SimpleString(R"(
class FloatLikeClass:
  def __float__(self):
    return 1.5

f = FloatLikeClass();
  )");
  PyObjectPtr f(testing::mainModuleGet("f"));
  double res = PyFloat_AsDouble(f);
  EXPECT_EQ(res, 1.5);
}

TEST_F(FloatExtensionApiTest, AsDoubleWithDunderFloatPropagatesException) {
  PyRun_SimpleString(R"(
class FloatLikeClass:
  @property
  def __float__(self):
    raise KeyError

f = FloatLikeClass();
  )");
  PyObjectPtr f(mainModuleGet("f"));
  EXPECT_EQ(PyFloat_AsDouble(f), -1.0);

  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_KeyError));
}

TEST_F(FloatExtensionApiTest, AsDoubleFromFloatSubclassReturnsFloat) {
  PyRun_SimpleString(R"(
class SubFloat(float):
  def __new__(self, value):
    self.foo = 3
    return super().__new__(self, value)
subfloat = SubFloat(1.5)
subfloat_foo = subfloat.foo
  )");
  PyObjectPtr subfloat(testing::mainModuleGet("subfloat"));
  ASSERT_FALSE(PyLong_CheckExact(subfloat));
  ASSERT_TRUE(PyFloat_Check(subfloat));
  EXPECT_EQ(1.5, PyFloat_AsDouble(subfloat));

  PyObjectPtr subfloat_foo(testing::mainModuleGet("subfloat_foo"));
  ASSERT_TRUE(PyLong_Check(subfloat_foo));
  EXPECT_EQ(3, PyLong_AsLong(subfloat_foo));
}

TEST_F(FloatExtensionApiTest, GetMaxReturnsDblMax) {
  EXPECT_EQ(PyFloat_GetMax(), DBL_MAX);
}

TEST_F(FloatExtensionApiTest, GetMinReturnsDblMin) {
  EXPECT_EQ(PyFloat_GetMin(), DBL_MIN);
}

TEST_F(FloatExtensionApiTest, Pack2) {
  double expected = 1.5;
  unsigned char ptr[2] = {};
  ASSERT_EQ(_PyFloat_Pack2(expected, ptr, /* le */ true), 0);
  // 00000000 00111110
  ASSERT_EQ(ptr[0], 0);
  ASSERT_EQ(ptr[1], 62);
  EXPECT_EQ(_PyFloat_Unpack2(ptr, /* le */ true), 1.5);
}

TEST_F(FloatExtensionApiTest, Pack4) {
  double expected = 1.5;
  unsigned char ptr[4] = {};
  ASSERT_EQ(_PyFloat_Pack4(expected, ptr, /* le */ true), 0);
  // 0000000 0000000 11000000 00111111
  ASSERT_EQ(ptr[0], 0);
  ASSERT_EQ(ptr[1], 0);
  ASSERT_EQ(ptr[2], 192);
  ASSERT_EQ(ptr[3], 63);
  EXPECT_EQ(_PyFloat_Unpack4(ptr, /* le */ true), 1.5);
}

TEST_F(FloatExtensionApiTest, Pack8) {
  double expected = 1.5;
  unsigned char ptr[8] = {};
  ASSERT_EQ(_PyFloat_Pack8(expected, ptr, /* le */ true), 0);
  // 0000000 0000000 0000000 0000000 0000000 0000000 11111000 00111111
  ASSERT_EQ(ptr[0], 0);
  ASSERT_EQ(ptr[1], 0);
  ASSERT_EQ(ptr[2], 0);
  ASSERT_EQ(ptr[3], 0);
  ASSERT_EQ(ptr[4], 0);
  ASSERT_EQ(ptr[5], 0);
  ASSERT_EQ(ptr[6], 248);
  ASSERT_EQ(ptr[7], 63);
  EXPECT_EQ(_PyFloat_Unpack8(ptr, /* le */ true), 1.5);
}

TEST_F(FloatExtensionApiTest, PyReturnNanReturnsNan) {
  PyObjectPtr module(PyModule_New("mod"));
  binaryfunc meth = [](PyObject*, PyObject*) { Py_RETURN_NAN; };
  static PyMethodDef foo_func = {"foo", meth, METH_NOARGS};
  PyObjectPtr func(PyCFunction_NewEx(&foo_func, nullptr, module));
  PyObjectPtr result(_PyObject_CallNoArg(func));
  EXPECT_TRUE(std::isnan(PyFloat_AsDouble(result)));
}

TEST_F(FloatExtensionApiTest, PyReturnINFReturnsInf) {
  PyObjectPtr module(PyModule_New("mod"));
  binaryfunc meth = [](PyObject*, PyObject*) { Py_RETURN_INF(0); };
  static PyMethodDef foo_func = {"foo", meth, METH_NOARGS};
  PyObjectPtr func(PyCFunction_NewEx(&foo_func, nullptr, module));
  PyObjectPtr result(_PyObject_CallNoArg(func));
  EXPECT_TRUE(std::isinf(PyFloat_AsDouble(result)));
  EXPECT_GT(PyFloat_AsDouble(result), 0.);
}

TEST_F(FloatExtensionApiTest, PyReturnINFReturnsNegativeInf) {
  PyObjectPtr module(PyModule_New("mod"));
  binaryfunc meth = [](PyObject*, PyObject*) { Py_RETURN_INF(-1); };
  static PyMethodDef foo_func = {"foo", meth, METH_NOARGS};
  PyObjectPtr func(PyCFunction_NewEx(&foo_func, nullptr, module));
  PyObjectPtr result(_PyObject_CallNoArg(func));
  EXPECT_TRUE(std::isinf(PyFloat_AsDouble(result)));
  EXPECT_LT(PyFloat_AsDouble(result), 0.);
}

}  // namespace testing
}  // namespace py
