#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using ErrorsExtensionApiTest = ExtensionApi;

TEST_F(ErrorsExtensionApiTest, CompareErrorMessageOnThread) {
  const char* error_message = "An exception occured";

  PyErr_SetString(nullptr, error_message);
  ASSERT_NE(nullptr, PyErr_Occurred());

  EXPECT_TRUE(testing::exceptionValueMatches(error_message));
}

TEST_F(ErrorsExtensionApiTest, ClearError) {
  PyErr_SetString(nullptr, "Something blew up.");
  EXPECT_NE(PyErr_Occurred(), nullptr);

  PyErr_Clear();
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

}  // namespace python
