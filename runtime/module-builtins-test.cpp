#include "gtest/gtest.h"

#include "module-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ModuleBuiltinsTest, DunderName) {
  Runtime runtime;

  runFromCStr(&runtime, R"(
import sys
a = sys.__name__
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(a.equalsCStr("sys"));
}

TEST(ModuleBuiltinsDeathTest, DirectModuleAccessRaisesNameError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "module.__new__()"),
                            LayoutId::kNameError,
                            "name 'module' is not defined"));
}

TEST(ModuleBuiltinsDeathTest, DunderNewNotEnoughArgumentsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, "import sys; type(sys).__new__()"),
                    LayoutId::kTypeError,
                    "TypeError: '__new__' takes 2 arguments but 0 given"));
}

TEST(ModuleBuiltinsDeathTest, DunderNewTooManyArgumentsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "import sys; type(sys).__new__(123, 456, 789)"),
      LayoutId::kTypeError,
      "TypeError: '__new__' takes max 2 positional arguments but 3 given"));
}

TEST(ModuleBuiltinsTest, DunderNewWithNonTypeRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Int nontype(&scope, SmallInt::fromWord(123));
  Str module_name(&scope, runtime.newStrFromCStr("foo"));
  Object module(&scope,
                runBuiltin(ModuleBuiltins::dunderNew, nontype, module_name));
  EXPECT_TRUE(
      raisedWithStr(*module, LayoutId::kTypeError, "not a type object"));
}

TEST(ModuleBuiltinsTest, DunderNewWithNonModuleRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Type nonmodule(&scope, runtime.typeAt(LayoutId::kSmallStr));
  Str module_name(&scope, runtime.newStrFromCStr("foo"));
  Object module(&scope,
                runBuiltin(ModuleBuiltins::dunderNew, nonmodule, module_name));
  EXPECT_TRUE(
      raisedWithStr(*module, LayoutId::kTypeError, "not a subtype of module"));
}

TEST(ModuleBuiltinsTest, DunderNewReturnsModule) {
  Runtime runtime;
  HandleScope scope;
  Type moduletype(&scope, runtime.typeAt(LayoutId::kModule));
  Str module_name(&scope, runtime.newStrFromCStr("foo"));
  Object module(&scope,
                runBuiltin(ModuleBuiltins::dunderNew, moduletype, module_name));
  ASSERT_TRUE(module.isModule());
  Object result_module_name(&scope, RawModule::cast(*module).name());
  EXPECT_TRUE(isStrEquals(result_module_name, module_name));
}

TEST(ModuleBuiltinsTest, DunderNewWithNonStrNameRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Type moduletype(&scope, runtime.typeAt(LayoutId::kModule));
  Int non_str_name(&scope, SmallInt::fromWord(123));
  Object module(
      &scope, runBuiltin(ModuleBuiltins::dunderNew, moduletype, non_str_name));
  EXPECT_TRUE(raised(*module, LayoutId::kTypeError));
}

TEST(ModuleBuiltinsTest, DunderDict) {
  Runtime runtime;

  runFromCStr(&runtime, R"(
import sys
result = sys.__dict__
)");
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(result.isDict());
}

}  // namespace python
