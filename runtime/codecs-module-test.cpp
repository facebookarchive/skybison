#include "gtest/gtest.h"

#include "codecs-module.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(CodecsModuleTest, RegisterErrorWithNonStringFirstReturnsError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
a = _codecs.register_error([], [])
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(
      raisedWithStr(*a, LayoutId::kTypeError,
                    "register_error() argument 1 must be str, not list"));
}

TEST(CodecsModuleTest, RegisterErrorWithNonCallableSecondReturnsError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
a = _codecs.register_error("", [])
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(
      raisedWithStr(*a, LayoutId::kTypeError, "handler must be callable"));
}

TEST(CodecsModuleTest, LookupErrorWithNonStringReturnsError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
a = _codecs.lookup_error([])
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(raisedWithStr(*a, LayoutId::kTypeError,
                            "lookup_error() argument must be str, not list"));
}

TEST(CodecsModuleTest, LookupErrorWithUnkownNameReturnsError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
a = _codecs.lookup_error('unknown')
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(raisedWithStr(*a, LayoutId::kLookupError,
                            "unknown error handler name 'unknown'"));
}

TEST(CodecsModuleTest, LookupErrorWithRegisteredErrorReturnsFunction) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
def test_errors():
    pass
_codecs.register_error("test", test_errors)
a = _codecs.lookup_error("test")
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a.isFunction());

  Function error(&scope, *a);
  EXPECT_TRUE(isStrEqualsCStr(error.name(), "test_errors"));
}

TEST(CodecsModuleTest, LookupErrorWithIgnoreReturnsFunction) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
a = _codecs.lookup_error('ignore')
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a.isFunction());

  Function error(&scope, *a);
  EXPECT_TRUE(isStrEqualsCStr(error.name(), "ignore_errors"));
}

TEST(CodecsModuleTest, LookupErrorWithStrictReturnsFunction) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
a = _codecs.lookup_error('strict')
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a.isFunction());

  Function error(&scope, *a);
  EXPECT_TRUE(isStrEqualsCStr(error.name(), "strict_errors"));
}

TEST(CodecsModuleTest, CodecsIsImportable) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "import codecs").isError());
}

}  // namespace python
