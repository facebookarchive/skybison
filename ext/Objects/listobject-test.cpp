#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using ListExtensionApiTest = ExtensionApi;

TEST_F(ListExtensionApiTest, NewWithBadLengthReturnsNull) {
  Py_ssize_t invalid_length = -1;
  PyObject* pyresult = PyList_New(invalid_length);
  EXPECT_EQ(pyresult, nullptr);
}

TEST_F(ListExtensionApiTest, NewReturnsList) {
  Py_ssize_t length = 0;
  PyObject* pyresult = PyList_New(length);
  EXPECT_TRUE(PyList_CheckExact(pyresult));

  // TODO(eelizondo): Check list size once PyList_Size is implemented
  Py_ssize_t length2 = 5;
  PyObject* pyresult2 = PyList_New(length2);
  EXPECT_TRUE(PyList_CheckExact(pyresult2));
}

}  // namespace python
