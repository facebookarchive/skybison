#include <signal.h>

#include <cwchar>

#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using PylifecycleExtensionApiTest = ExtensionApi;

TEST_F(PylifecycleExtensionApiTest, FatalErrorPrintsAndAbortsDeathTest) {
  EXPECT_DEATH(Py_FatalError("hello world"), "hello world");
}

TEST_F(PylifecycleExtensionApiTest, AtExitRegistersExitFunction) {
  ASSERT_EXIT(PyRun_SimpleString(R"(
def cleanup():
    import sys
    print("foo", file=sys.stderr)

import atexit
atexit.register(cleanup)
raise SystemExit(123)
)"),
              ::testing::ExitedWithCode(123), "foo");
}

TEST_F(PylifecycleExtensionApiTest, GetsigGetsCurrentSignalHandler) {
  PyOS_sighandler_t handler = [](int) {};
  PyOS_sighandler_t saved = PyOS_setsig(SIGABRT, handler);

  EXPECT_EQ(PyOS_getsig(SIGABRT), handler);

  PyOS_setsig(SIGABRT, saved);
}

TEST(PylifecycleExtensionApiTestNoFixture, InitializeSetsSysFlagsVariant0) {
  Py_DontWriteBytecodeFlag = 1;
  Py_IgnoreEnvironmentFlag = 0;
  Py_InspectFlag = 13;
  Py_InteractiveFlag = 0;
  Py_IsolatedFlag = 0;
  Py_NoSiteFlag = 1;
  Py_NoUserSiteDirectory = 0;
  Py_OptimizeFlag = 2;
  Py_QuietFlag = 0;
  Py_VerboseFlag = 13;
  Py_Initialize();

  {
    PyObjectPtr flags(moduleGet("sys", "flags"));
    ASSERT_NE(flags.get(), nullptr);
    EXPECT_TRUE(isLongEqualsLong(
        PyObject_GetAttrString(flags, "dont_write_bytecode"), 1));
    EXPECT_TRUE(isLongEqualsLong(
        PyObject_GetAttrString(flags, "ignore_environment"), 0));
    EXPECT_TRUE(isLongEqualsLong(PyObject_GetAttrString(flags, "inspect"), 13));
    EXPECT_TRUE(
        isLongEqualsLong(PyObject_GetAttrString(flags, "interactive"), 0));
    EXPECT_TRUE(isLongEqualsLong(PyObject_GetAttrString(flags, "isolated"), 0));
    EXPECT_TRUE(isLongEqualsLong(PyObject_GetAttrString(flags, "no_site"), 1));
    EXPECT_TRUE(
        isLongEqualsLong(PyObject_GetAttrString(flags, "no_user_site"), 0));
    EXPECT_TRUE(isLongEqualsLong(PyObject_GetAttrString(flags, "optimize"), 2));
    EXPECT_TRUE(isLongEqualsLong(PyObject_GetAttrString(flags, "quiet"), 0));
    EXPECT_TRUE(isLongEqualsLong(PyObject_GetAttrString(flags, "verbose"), 13));
  }

  Py_FinalizeEx();
}

TEST(PylifecycleExtensionApiTestNoFixture, InitializeSetsSysFlagsVariant1) {
  Py_DontWriteBytecodeFlag = 0;
  Py_IgnoreEnvironmentFlag = 1;
  Py_InspectFlag = 0;
  Py_InteractiveFlag = 1;
  Py_IsolatedFlag = 1;
  Py_NoSiteFlag = 0;
  Py_NoUserSiteDirectory = 1;
  Py_OptimizeFlag = 0;
  Py_QuietFlag = 1;
  Py_VerboseFlag = 0;
  Py_Initialize();

  {
    PyObjectPtr flags(moduleGet("sys", "flags"));
    ASSERT_NE(flags.get(), nullptr);
    EXPECT_TRUE(isLongEqualsLong(
        PyObject_GetAttrString(flags, "dont_write_bytecode"), 0));
    EXPECT_TRUE(isLongEqualsLong(
        PyObject_GetAttrString(flags, "ignore_environment"), 1));
    EXPECT_TRUE(isLongEqualsLong(PyObject_GetAttrString(flags, "inspect"), 0));
    EXPECT_TRUE(
        isLongEqualsLong(PyObject_GetAttrString(flags, "interactive"), 1));
    EXPECT_TRUE(isLongEqualsLong(PyObject_GetAttrString(flags, "isolated"), 1));
    EXPECT_TRUE(isLongEqualsLong(PyObject_GetAttrString(flags, "no_site"), 0));
    EXPECT_TRUE(
        isLongEqualsLong(PyObject_GetAttrString(flags, "no_user_site"), 1));
    EXPECT_TRUE(isLongEqualsLong(PyObject_GetAttrString(flags, "optimize"), 0));
    EXPECT_TRUE(isLongEqualsLong(PyObject_GetAttrString(flags, "quiet"), 1));
    EXPECT_TRUE(isLongEqualsLong(PyObject_GetAttrString(flags, "verbose"), 0));
  }

  Py_FinalizeEx();
}

TEST_F(PylifecycleExtensionApiTest, SetsigSetsSignalHandler) {
  PyOS_sighandler_t saved = PyOS_getsig(SIGUSR1);
  PyOS_sighandler_t handler = [](int) { PyRun_SimpleString("handled = True"); };

  PyOS_sighandler_t old_handler = PyOS_setsig(SIGUSR1, handler);
  EXPECT_EQ(old_handler, saved);
  EXPECT_EQ(PyOS_getsig(SIGUSR1), handler);

  ::raise(SIGUSR1);
  PyObjectPtr handled(mainModuleGet("handled"));
  EXPECT_EQ(handled, Py_True);

  PyOS_setsig(SIGUSR1, saved);
}

TEST_F(PylifecycleExtensionApiTest, RestoreSignalRestoresToDefault) {
  PyOS_sighandler_t handler = [](int) {};
  EXPECT_NE(handler, SIG_DFL);

  PyOS_setsig(SIGUSR1, handler);
  PyOS_setsig(SIGPIPE, handler);
  PyOS_setsig(SIGXFSZ, handler);

  EXPECT_EQ(PyOS_getsig(SIGUSR1), handler);
  EXPECT_EQ(PyOS_getsig(SIGPIPE), handler);
  EXPECT_EQ(PyOS_getsig(SIGXFSZ), handler);

  _Py_RestoreSignals();

  EXPECT_EQ(PyOS_getsig(SIGUSR1), handler);
  EXPECT_EQ(PyOS_getsig(SIGPIPE), SIG_DFL);
  EXPECT_EQ(PyOS_getsig(SIGXFSZ), SIG_DFL);
}

TEST_F(PylifecycleExtensionApiTest, GetProgramNameReturnsString) {
  const wchar_t* program_name = Py_GetProgramName();
  EXPECT_TRUE(std::wcsstr(program_name, L"python-tests") != nullptr ||
              std::wcsstr(program_name, L"cpython-tests") != nullptr);
}

TEST_F(PylifecycleExtensionApiTest, SetProgramNameSetsName) {
  wchar_t name[] = L"new-program-name";
  Py_SetProgramName(name);
  EXPECT_STREQ(Py_GetProgramName(), L"new-program-name");
}

TEST(PylifecycleExtensionApiTestNoFixture,
     IsFinalizingReturnsFalseBeforeAndAfterFinalizePyro) {
  Py_Initialize();
  ASSERT_EQ(_Py_IsFinalizing(), 0);
  ASSERT_EQ(Py_FinalizeEx(), 0);
  EXPECT_EQ(_Py_IsFinalizing(), 0);
}

}  // namespace testing
}  // namespace py
