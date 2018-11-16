#include "capi-fixture.h"

namespace python {

using ErrorsExtensionApiTest = ExtensionApi;

TEST_F(ErrorsExtensionApiTest, CompareErrorMessageOnThread) {
  const char* error_message = "An exception occured";

  PyErr_SetString(nullptr, error_message);
  ASSERT_NE(nullptr, PyErr_Occurred());

  EXPECT_TRUE(_PyErr_ExceptionMessageMatches(error_message));
}

}  // namespace python
