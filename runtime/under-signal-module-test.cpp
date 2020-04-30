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

TEST_F(UnderSignalModuleTest, DefaultIntHandlerRaisesKeyboardInterrupt) {
  EXPECT_TRUE(raised(runBuiltin(FUNC(_signal, default_int_handler)),
                     LayoutId::kKeyboardInterrupt));
}

TEST_F(UnderSignalModuleTest, GetsignalWithNegativeRaisesValueError) {
  HandleScope scope(thread_);
  Object signum(&scope, SmallInt::fromWord(-1));
  EXPECT_TRUE(raisedWithStr(runBuiltin(FUNC(_signal, getsignal), signum),
                            LayoutId::kValueError,
                            "signal number out of range"));
}

TEST_F(UnderSignalModuleTest, GetsignalWithLargeNumberRaisesValueError) {
  HandleScope scope(thread_);
  Object signum(&scope, SmallInt::fromWord(128));
  EXPECT_TRUE(raisedWithStr(runBuiltin(FUNC(_signal, getsignal), signum),
                            LayoutId::kValueError,
                            "signal number out of range"));
}

TEST_F(UnderSignalModuleTest, GetsignalWithSigintReturnsCallback) {
  HandleScope scope(thread_);
  Object signum(&scope, SmallInt::fromWord(SIGINT));
  Object callback(&scope, runBuiltin(FUNC(_signal, getsignal), signum));
  EXPECT_EQ(callback,
            moduleAtByCStr(runtime_, "_signal", "default_int_handler"));
}

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

TEST_F(UnderSignalModuleTest, SignalWithNegativeRaisesValueError) {
  HandleScope scope(thread_);
  Object signum(&scope, SmallInt::fromWord(-1));
  Object callback(&scope, kDefaultHandler);
  EXPECT_TRUE(raised(runBuiltin(FUNC(_signal, signal), signum, callback),
                     LayoutId::kValueError));
}

TEST_F(UnderSignalModuleTest, SignalWithLargeNumberRaisesValueError) {
  HandleScope scope(thread_);
  Object signum(&scope, SmallInt::fromWord(128));
  Object callback(&scope, kIgnoreHandler);
  EXPECT_TRUE(raisedWithStr(runBuiltin(FUNC(_signal, signal), signum, callback),
                            LayoutId::kValueError,
                            "signal number out of range"));
}

TEST_F(UnderSignalModuleTest, SignalWithInvalidCallbackRaisesTypeError) {
  HandleScope scope(thread_);
  Object signum(&scope, SmallInt::fromWord(SIGINT));
  Object callback(&scope, runtime_->emptyTuple());
  EXPECT_TRUE(raisedWithStr(runBuiltin(FUNC(_signal, signal), signum, callback),
                            LayoutId::kTypeError,
                            "signal handler must be signal.SIG_IGN, "
                            "signal.SIG_DFL, or a callable object"));
}

TEST_F(UnderSignalModuleTest, SignalSetsSignalCallback) {
  HandleScope scope(thread_);
  Object signum(&scope, SmallInt::fromWord(SIGINT));
  Object sig_ign(&scope, kIgnoreHandler);
  Object old_callback(&scope,
                      runBuiltin(FUNC(_signal, signal), signum, sig_ign));
  Object new_callback(&scope, runBuiltin(FUNC(_signal, getsignal), signum));
  EXPECT_EQ(new_callback, sig_ign);

  ASSERT_FALSE(runBuiltin(FUNC(_signal, signal), signum, old_callback)
                   .isErrorException());
}

}  // namespace testing
}  // namespace py
