#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using IterExtensionApiTest = ExtensionApi;

TEST_F(IterExtensionApiTest, SeqIterNewWithNonSequenceRaises) {
  ASSERT_EQ(PySeqIter_New(Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(IterExtensionApiTest, SeqIterNewWithSequenceReturnsIterator) {
  PyObjectPtr list(PyList_New(0));
  PyObject* seq = PySeqIter_New(list);
  ASSERT_NE(seq, nullptr);
  EXPECT_TRUE(PyIter_Check(seq));
  Py_DECREF(seq);
}

}  // namespace python
