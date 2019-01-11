#include "gtest/gtest.h"

#include "function-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {
namespace testing {

TEST(FunctionBuiltinsTest, NativeFunctionObjectsExposeNoneDunderCode) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
code = len.__code__
)");
  HandleScope scope;
  Object code(&scope, moduleAt(&runtime, "__main__", "code"));
  ASSERT_TRUE(code->isNoneType());
}

TEST(FunctionBuiltinsTest, ManagedFunctionObjectsExposeDunderCode) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
def foo(x):
  return x + 1
code = foo.__code__
)");
  HandleScope scope;
  Object code(&scope, moduleAt(&runtime, "__main__", "code"));
  ASSERT_TRUE(code->isCode());
}

TEST(FunctionBuiltinsTest,
     ChangingCodeOfFunctionObjectChangesFunctionBehavior) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
def foo(x):
  return x + 1
def bar(x):
  return x + 5
foo.__code__ = bar.__code__
a = foo(5)
)");
  HandleScope scope;
  Object a_obj(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a_obj->isSmallInt());
  SmallInt a(&scope, *a_obj);
  ASSERT_EQ(a->value(), 10);
}

}  // namespace testing
}  // namespace python
