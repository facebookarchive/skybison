#include <getopt.h>

#include <clocale>

#include "Python.h"
#include "gtest/gtest.h"

namespace py {

using namespace testing;

class MainExtensionApiTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Since this is global, we must reset it after every test
    optind = 1;
  }
};

TEST_F(MainExtensionApiTest, RunModule) {
  ::testing::internal::CaptureStdout();
  wchar_t arg0[] = L"python";
  wchar_t arg1[] = L"-m";
  wchar_t arg2[] = L"textwrap";
  wchar_t* argv[] = {arg0, arg1, arg2};
  EXPECT_EQ(Py_Main(/*argc=*/3, argv), 0);
  EXPECT_EQ(::testing::internal::GetCapturedStdout(),
            "Hello there.\n  This is indented.\n");
}

TEST_F(MainExtensionApiTest, RunCommand) {
  ::testing::internal::CaptureStdout();
  wchar_t arg0[] = L"python";
  wchar_t arg1[] = L"-c";
  wchar_t arg2[] = L"print(40 * 40)";
  wchar_t* argv[] = {arg0, arg1, arg2};
  EXPECT_EQ(Py_Main(/*argc=*/3, argv), 0);
  EXPECT_EQ(::testing::internal::GetCapturedStdout(), "1600\n");
}

}  // namespace py
