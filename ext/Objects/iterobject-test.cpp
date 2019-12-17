#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {

using namespace testing;

using IterExtensionApiTest = ExtensionApi;

TEST_F(IterExtensionApiTest, CallIterNewReturnsIterator) {
  PyRun_SimpleString(R"(
class C:
  def __init__(self):
    self.x = -3
  def __next__(self):
    self.x += 2
    return self.x
i = C()
)");
  PyObjectPtr i(moduleGet("__main__", "i"));
  PyObjectPtr sentinel(PyLong_FromLong(5));
  PyObjectPtr iterator(PyCallIter_New(i, sentinel));
  PyObjectPtr type(PyObject_Type(iterator));
  EXPECT_EQ(strcmp(_PyType_Name(reinterpret_cast<PyTypeObject*>(type.get())),
                   "callable_iterator"),
            0);
}

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

}  // namespace py
