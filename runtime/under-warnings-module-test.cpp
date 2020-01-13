#include "under-warnings-module.h"

#include "gtest/gtest.h"

#include "runtime.h"
#include "test-utils.h"

namespace py {
using namespace testing;

using UnderWarningsModuleTest = RuntimeFixture;

TEST_F(UnderWarningsModuleTest, ModuleImporting) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import _warnings
  )")
                   .isError());
  RawObject warnings = mainModuleAt(runtime_, "_warnings");
  EXPECT_TRUE(warnings.isModule());
}

TEST_F(UnderWarningsModuleTest, WarnDoesNothing) {
  // TODO(T39431178): _warnings.warn() should actually do things.
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import _warnings
result = _warnings.warn("something went wrong")
)")
                   .isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(result.isNoneType());
}

TEST_F(UnderWarningsModuleTest, WarnWithNoArgsRaisesTypeError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(runtime_, R"(
import _warnings
_warnings.warn()
)"),
                    LayoutId::kTypeError,
                    "'warn' takes min 1 positional arguments but 0 given"));
}

TEST_F(UnderWarningsModuleTest, WarnWithInvalidCategoryRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
import _warnings
_warnings.warn("warning!", 1234)
)"),
                            LayoutId::kTypeError,
                            "category must be a Warning subclass"));
}

TEST_F(UnderWarningsModuleTest, WarnWithLargeStacklevelRaisesOverflowError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
import _warnings
_warnings.warn("hello", stacklevel=1180591620717411303424)  # 2 ** 70
)"),
                            LayoutId::kOverflowError,
                            "Python int too large to convert to C ssize_t"));
}

TEST_F(UnderWarningsModuleTest, WarnWithInvalidKwRaisesTypeError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(runtime_, R"(
import _warnings
_warnings.warn("hello", stack_level=3)
  )"),
                    LayoutId::kTypeError,
                    "warn() got an unexpected keyword argument 'stack_level'"));
}

}  // namespace py
