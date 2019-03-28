#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

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

}  // namespace python
