#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using GetArgsExtensionApiTest = ExtensionApi;

TEST_F(GetArgsExtensionApiTest, ParseTupleOneObject) {
  PyObject* pytuple = PyTuple_New(1);
  PyObject* in = PyLong_FromLong(42);
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, in));

  PyObject* out;
  PyArg_ParseTuple(pytuple, "O:xyz", &out);
  EXPECT_EQ(42, PyLong_AsLong(out));
}

TEST_F(GetArgsExtensionApiTest, ParseTupleMultipleObjects) {
  PyObject* pytuple = PyTuple_New(3);
  PyObject* in1 = PyLong_FromLong(111);
  PyObject* in2 = Py_None;
  PyObject* in3 = PyLong_FromLong(333);
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, in1));
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 1, in2));
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 2, in3));

  PyObject* out1;
  PyObject* out2;
  PyObject* out3;
  PyArg_ParseTuple(pytuple, "OOO:xyz", &out1, &out2, &out3);
  EXPECT_EQ(111, PyLong_AsLong(out1));
  EXPECT_EQ(Py_None, out2);
  EXPECT_EQ(333, PyLong_AsLong(out3));
}

}  // namespace python
