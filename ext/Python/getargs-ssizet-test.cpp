#define PY_SSIZE_T_CLEAN
#include "Python.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using GetArgsSsizeTExtensionApiTest = ExtensionApi;

TEST_F(GetArgsSsizeTExtensionApiTest, ParseTupleStringFromNone) {
  PyObjectPtr pytuple(PyTuple_New(2));
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, Py_None));
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 1, Py_None));

  char *out1, *out2;
  Py_ssize_t size = 123;
  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "zz#", &out1, &out2, &size));
  EXPECT_EQ(nullptr, out1);
  EXPECT_EQ(nullptr, out2);
  EXPECT_EQ(0, size);
}

TEST_F(GetArgsSsizeTExtensionApiTest, ParseTupleStringWithSize) {
  PyObjectPtr pytuple(PyTuple_New(2));
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, PyUnicode_FromString("hello")));
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 1, PyUnicode_FromString("cpython")));

  char *out1, *out2;
  Py_ssize_t size1, size2;
  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "s#z#", &out1, &size1, &out2, &size2));
  EXPECT_STREQ("hello", out1);
  EXPECT_EQ(5, size1);
  EXPECT_STREQ("cpython", out2);
  EXPECT_EQ(7, size2);
}

}  // namespace python
