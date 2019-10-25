#include "gtest/gtest.h"

#include <cstring>

#include "Python.h"
#include "capi-testing.h"

namespace py {

TEST(MysnprintfTest, Snprintf) {
  char str[5];

  std::memset(str, 0xFF, sizeof(str));
  EXPECT_EQ(PyOS_snprintf(str, sizeof(str), "%d", 123), 3);
  EXPECT_STREQ(str, "123");

  std::memset(str, 0xFF, sizeof(str));
  EXPECT_EQ(PyOS_snprintf(str, sizeof(str), "%d", 1234), 4);
  EXPECT_STREQ(str, "1234");

  std::memset(str, 0xFF, sizeof(str));
  EXPECT_EQ(PyOS_snprintf(str, sizeof(str), "%d", 12345678), 8);
  EXPECT_STREQ(str, "1234");
}

}  // namespace py
