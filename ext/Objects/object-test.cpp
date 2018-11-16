#include "capi-fixture.h"

namespace python {

using ObjectExtensionApiTest = ExtensionApi;

TEST_F(ObjectExtensionApiTest, PyNoneIdentityIsEqual) {
  // Test Identitiy
  PyObject* none1 = Py_None;
  PyObject* none2 = Py_None;
  EXPECT_EQ(none1, none2);
}

}  // namespace python
