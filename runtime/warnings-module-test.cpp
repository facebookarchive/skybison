#include "gtest/gtest.h"

#include "runtime.h"
#include "test-utils.h"
#include "warnings-module.h"

namespace python {
using namespace testing;

using WarningsModuleTest = RuntimeFixture;

TEST_F(WarningsModuleTest, ModuleImporting) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import _warnings
  )")
                   .isError());
  RawObject warnings = mainModuleAt(&runtime_, "_warnings");
  EXPECT_TRUE(warnings.isModule());
}

TEST_F(WarningsModuleTest, WarnDoesNothing) {
  // TODO(T39431178): _warnings.warn() should actually do things.
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import _warnings
result = _warnings.warn("something went wrong")
)")
                   .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(result.isNoneType());
}

TEST_F(WarningsModuleTest, WarnWithNoArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, R"(
import _warnings
_warnings.warn()
)"),
      LayoutId::kTypeError,
      "TypeError: 'warn' takes min 1 positional arguments but 0 given"));
}

TEST_F(WarningsModuleTest, WarnWithInvalidCategoryRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
import _warnings
_warnings.warn("warning!", 1234)
)"),
                            LayoutId::kTypeError,
                            "category must be a Warning subclass"));
}

TEST_F(WarningsModuleTest, WarnWithLargeStacklevelRaisesOverflowError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
import _warnings
_warnings.warn("hello", stacklevel=1180591620717411303424)  # 2 ** 70
)"),
                            LayoutId::kOverflowError,
                            "Python int too large to convert to C ssize_t"));
}

TEST_F(WarningsModuleTest, WarnWithInvalidKwRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
import _warnings
_warnings.warn("hello", stack_level=3)
  )"),
                            LayoutId::kTypeError,
                            "TypeError: invalid keyword argument supplied"));
}

}  // namespace python
