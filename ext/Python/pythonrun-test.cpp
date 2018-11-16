#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"

namespace python {

using PythonrunExtensionApiTest = ExtensionApi;

TEST_F(PythonrunExtensionApiTest, RunSimpleStringReturnsZero) {
  EXPECT_EQ(PyRun_SimpleString("a = 42"), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

}  // namespace python
