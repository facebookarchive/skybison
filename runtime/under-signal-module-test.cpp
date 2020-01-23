#include "under-signal-module.h"

#include <signal.h>

#include "gtest/gtest.h"

#include "runtime.h"
#include "test-utils.h"

namespace py {

using namespace testing;

using UnderSignalModuleTest = RuntimeFixture;

TEST_F(UnderSignalModuleTest, TestSigDflExists) {
  HandleScope scope(thread_);
  Object sig_dfl(&scope, moduleAtByCStr(runtime_, "_signal", "SIG_DFL"));
  ASSERT_TRUE(sig_dfl.isSmallInt());
  ASSERT_EQ(SmallInt::cast(*sig_dfl).value(), reinterpret_cast<word>(SIG_DFL));
}

TEST_F(UnderSignalModuleTest, TestSigIgnExists) {
  HandleScope scope(thread_);
  Object sig_dfl(&scope, moduleAtByCStr(runtime_, "_signal", "SIG_IGN"));
  ASSERT_TRUE(sig_dfl.isSmallInt());
  ASSERT_EQ(SmallInt::cast(*sig_dfl).value(), reinterpret_cast<word>(SIG_IGN));
}

}  // namespace py
