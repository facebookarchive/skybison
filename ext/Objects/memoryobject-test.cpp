#include <cstring>

#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using MemoryViewExtensionApiTest = ExtensionApi;

TEST_F(MemoryViewExtensionApiTest, FromObjectWithNoneRaisesTypeError) {
  PyObjectPtr result(PyMemoryView_FromObject(Py_None));
  ASSERT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(MemoryViewExtensionApiTest, FromObjectWithBytesReturnsMemoryView) {
  PyObjectPtr bytes(PyBytes_FromString(""));
  PyObjectPtr result(PyMemoryView_FromObject(bytes));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyMemoryView_Check(result));
}

TEST_F(MemoryViewExtensionApiTest, FromObjectWithMemoryViewReturnsMemoryView) {
  PyObjectPtr bytes(PyBytes_FromString(""));
  PyObjectPtr view(PyMemoryView_FromObject(bytes));
  ASSERT_TRUE(PyMemoryView_Check(view));
  PyObjectPtr result(PyMemoryView_FromObject(view));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyMemoryView_Check(result));
  EXPECT_NE(result.get(), view.get());
}

TEST_F(MemoryViewExtensionApiTest,
       FromObjectWithBufferProtocolReturnsMemoryView) {
  static char contents[] = "hello world";
  static Py_ssize_t contents_len = std::strlen(contents);
  getbufferproc getbuffer_func = [](PyObject* obj, Py_buffer* view, int flags) {
    return PyBuffer_FillInfo(view, obj, ::strdup(contents), contents_len,
                             /*readonly=*/1, flags);
  };
  releasebufferproc releasebuffer_func = [](PyObject*, Py_buffer* view) {
    std::free(view->buf);
    view->obj = nullptr;
  };
  PyType_Slot slots[] = {
      {Py_bf_getbuffer, reinterpret_cast<void*>(getbuffer_func)},
      {Py_bf_releasebuffer, reinterpret_cast<void*>(releasebuffer_func)},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  PyObjectPtr instance(PyObject_CallFunction(type, nullptr));
  ASSERT_NE(instance, nullptr);
  PyObjectPtr view(PyMemoryView_FromObject(instance));
  ASSERT_NE(view, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyMemoryView_Check(view));
}

TEST_F(MemoryViewExtensionApiTest,
       DunderNewWithBufferProtocolReturnsMemoryView) {
  static char contents[] = "hello world";
  static Py_ssize_t contents_len = std::strlen(contents);
  getbufferproc getbuffer_func = [](PyObject* obj, Py_buffer* view, int flags) {
    return PyBuffer_FillInfo(view, obj, ::strdup(contents), contents_len,
                             /*readonly=*/1, flags);
  };
  releasebufferproc releasebuffer_func = [](PyObject*, Py_buffer* view) {
    std::free(view->buf);
    view->obj = nullptr;
  };
  PyType_Slot slots[] = {
      {Py_bf_getbuffer, reinterpret_cast<void*>(getbuffer_func)},
      {Py_bf_releasebuffer, reinterpret_cast<void*>(releasebuffer_func)},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  PyObjectPtr instance(PyObject_CallFunction(type, nullptr));
  ASSERT_NE(instance, nullptr);
  PyObjectPtr view(PyObject_CallFunction(
      reinterpret_cast<PyObject*>(&PyMemoryView_Type), "O", instance.get()));
  ASSERT_NE(view, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyMemoryView_Check(view));
}

TEST_F(MemoryViewExtensionApiTest, FromMemoryReturnsMemoryView) {
  const int size = 5;
  int memory[size] = {0};
  PyObjectPtr result(PyMemoryView_FromMemory(reinterpret_cast<char*>(memory),
                                             size, PyBUF_READ));
  EXPECT_NE(result, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyMemoryView_Check(result));
}

}  // namespace testing
}  // namespace py
