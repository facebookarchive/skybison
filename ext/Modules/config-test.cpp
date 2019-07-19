#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using ConfigExtensionApiTest = ExtensionApi;

TEST_F(ConfigExtensionApiTest, ImportUnderCapsuleReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_capsule"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportUnderMyReadlineReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_myreadline"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportUnderStentryReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_stentry"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportMathReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("math"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

}  // namespace python
