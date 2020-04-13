#include <getopt.h>

#include <clocale>

#include "Python.h"
#include "gtest/gtest.h"

namespace py {
namespace testing {

class MainExtensionApiTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Since this is global, we must reset it after every test
    optind = 1;
  }
};

TEST_F(MainExtensionApiTest, NoSiteZeroLoadsSite) {
  ASSERT_EQ(Py_NoSiteFlag, 0);
  ::testing::internal::CaptureStdout();
  wchar_t arg0[] = L"python";
  wchar_t arg1[] = L"-c";
  wchar_t arg2[] = L"import sys; print('site' in sys.modules)";
  wchar_t* argv[] = {arg0, arg1, arg2};
  EXPECT_EQ(Py_Main(/*argc=*/3, argv), 0);
  EXPECT_EQ(::testing::internal::GetCapturedStdout(), "True\n");
}

TEST_F(MainExtensionApiTest, NoSiteOneDoesNotLoadSite) {
  ASSERT_EQ(Py_NoSiteFlag, 0);
  ::testing::internal::CaptureStdout();
  wchar_t arg0[] = L"python";
  wchar_t arg1[] = L"-S";
  wchar_t arg2[] = L"-c";
  wchar_t arg3[] = L"import sys; print('site' in sys.modules)";
  wchar_t* argv[] = {arg0, arg1, arg2, arg3};
  EXPECT_EQ(Py_Main(/*argc=*/4, argv), 0);
  EXPECT_EQ(::testing::internal::GetCapturedStdout(), "False\n");
  Py_NoSiteFlag = 0;
}

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

TEST_F(MainExtensionApiTest, StoresProgramNamePyro) {
  wchar_t arg0[] = L"not-python";
  wchar_t arg1[] = L"-c";
  wchar_t arg2[] = L"None";
  wchar_t* argv[] = {arg0, arg1, arg2};
  EXPECT_EQ(Py_Main(/*argc=*/3, argv), 0);
  EXPECT_STREQ(Py_GetProgramName(), L"not-python");
}

}  // namespace testing
}  // namespace py
