#include <signal.h>

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

TEST_F(PylifecycleExtensionApiTest, SetsigSetsSignalHandler) {
  PyOS_sighandler_t saved = PyOS_getsig(SIGUSR1);
  PyOS_sighandler_t handler = [](int) { PyRun_SimpleString("handled = True"); };

  PyOS_sighandler_t old_handler = PyOS_setsig(SIGUSR1, handler);
  EXPECT_EQ(old_handler, saved);
  EXPECT_EQ(PyOS_getsig(SIGUSR1), handler);

  ::raise(SIGUSR1);
  PyObjectPtr handled(moduleGet("__main__", "handled"));
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

}  // namespace testing
}  // namespace py
