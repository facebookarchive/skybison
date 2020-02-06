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
