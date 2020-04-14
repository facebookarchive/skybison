#include "under-signal-module.h"

#include <signal.h>

#include "gtest/gtest.h"

#include "builtins.h"
#include "os.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {
namespace testing {

using UnderSignalModuleTest = RuntimeFixture;

TEST_F(UnderSignalModuleTest, TestNsigMatchesOS) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "import _signal").isError());
  Object nsig(&scope, moduleAtByCStr(runtime_, "_signal", "NSIG"));
  ASSERT_TRUE(nsig.isSmallInt());
  ASSERT_EQ(SmallInt::cast(*nsig).value(), OS::kNumSignals);
}

TEST_F(UnderSignalModuleTest, TestSigDflExists) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "import _signal").isError());
  Object sig_dfl(&scope, moduleAtByCStr(runtime_, "_signal", "SIG_DFL"));
  ASSERT_TRUE(sig_dfl.isSmallInt());
  ASSERT_EQ(SmallInt::cast(*sig_dfl).value(), reinterpret_cast<word>(SIG_DFL));
}

TEST_F(UnderSignalModuleTest, TestSigIgnExists) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "import _signal").isError());
  Object sig_dfl(&scope, moduleAtByCStr(runtime_, "_signal", "SIG_IGN"));
  ASSERT_TRUE(sig_dfl.isSmallInt());
  ASSERT_EQ(SmallInt::cast(*sig_dfl).value(), reinterpret_cast<word>(SIG_IGN));
}

TEST_F(UnderSignalModuleTest, TestSigintExists) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "import _signal").isError());
  Object sigint(&scope, moduleAtByCStr(runtime_, "_signal", "SIGINT"));
  ASSERT_TRUE(sigint.isSmallInt());
  EXPECT_TRUE(isIntEqualsWord(*sigint, SIGINT));
}

}  // namespace testing
}  // namespace py
