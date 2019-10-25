#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"

namespace py {

using RandomExtensionApiTest = ExtensionApi;

TEST_F(RandomExtensionApiTest, URandomNegativePositiveSizeDoesNotRaise) {
  char buffer[10];
  EXPECT_EQ(_PyOS_URandom(buffer, sizeof(buffer)), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(RandomExtensionApiTest, URandomNegativeSizeRaisesValueError) {
  char buffer[10];
  EXPECT_EQ(_PyOS_URandom(buffer, -1), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(RandomExtensionApiTest, URandomNonblockPositiveSizeDoesNotRaise) {
  char buffer[10];
  EXPECT_EQ(_PyOS_URandomNonblock(buffer, sizeof(buffer)), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(RandomExtensionApiTest, URandomNonblockNegativeSizeRaisesValueError) {
  char buffer[10];
  EXPECT_EQ(_PyOS_URandomNonblock(buffer, -1), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

}  // namespace py
