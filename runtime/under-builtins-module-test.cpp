#include "gtest/gtest.h"

#include "runtime.h"
#include "test-utils.h"
#include "trampolines.h"
#include "under-builtins-module.h"

namespace python {

using namespace testing;

using UnderBuiltinsModuleTest = RuntimeFixture;
using UnderBuiltinsModuleDeathTest = RuntimeFixture;

TEST_F(UnderBuiltinsModuleTest, CopyFunctionEntriesCopies) {
  HandleScope scope(thread_);
  Function::Entry entry = UnderBuiltinsModule::underIntCheck;
  Str qualname(&scope, runtime_.symbols()->UnderIntCheck());
  Function func(&scope, runtime_.newBuiltinFunction(SymbolId::kUnderIntCheck,
                                                    qualname, entry));
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def _int_check(self):
  "docstring"
  pass
)")
                   .isError());
  Function python_func(&scope, moduleAt(&runtime_, "__main__", "_int_check"));
  copyFunctionEntries(Thread::current(), func, python_func);
  Code base_code(&scope, func.code());
  Code patch_code(&scope, python_func.code());
  EXPECT_EQ(patch_code.code(), base_code.code());
  EXPECT_EQ(python_func.entry(), &builtinTrampoline);
  EXPECT_EQ(python_func.entryKw(), &builtinTrampolineKw);
  EXPECT_EQ(python_func.entryEx(), &builtinTrampolineEx);
}

TEST_F(UnderBuiltinsModuleDeathTest, CopyFunctionEntriesRedefinitionDies) {
  HandleScope scope(thread_);
  Function::Entry entry = UnderBuiltinsModule::underIntCheck;
  Str qualname(&scope, runtime_.symbols()->UnderIntCheck());
  Function func(&scope, runtime_.newBuiltinFunction(SymbolId::kUnderIntCheck,
                                                    qualname, entry));
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def _int_check(self):
  return True
)")
                   .isError());
  Function python_func(&scope, moduleAt(&runtime_, "__main__", "_int_check"));
  ASSERT_DEATH(
      copyFunctionEntries(Thread::current(), func, python_func),
      "Redefinition of native code method '_int_check' in managed code");
}

TEST_F(UnderBuiltinsModuleTest, UnderPatchWithBadPatchFuncRaisesTypeError) {
  HandleScope scope(thread_);
  Object not_func(&scope, runtime_.newInt(12));
  EXPECT_TRUE(
      raisedWithStr(runBuiltin(UnderBuiltinsModule::underPatch, not_func),
                    LayoutId::kTypeError, "_patch expects function argument"));
}

TEST_F(UnderBuiltinsModuleTest, UnderPatchWithMissingFuncRaisesAttributeError) {
  HandleScope scope(thread_);
  SymbolId func_name = SymbolId::kHex;
  Str qualname(&scope, runtime_.symbols()->at(func_name));
  Function func(
      &scope, runtime_.newBuiltinFunction(func_name, qualname,
                                          UnderBuiltinsModule::underIntCheck));
  Str module_name(&scope, runtime_.newStrFromCStr("foo"));
  Module module(&scope, runtime_.newModule(module_name));
  runtime_.addModule(module);
  func.setModule(*module_name);
  EXPECT_TRUE(raisedWithStr(runBuiltin(UnderBuiltinsModule::underPatch, func),
                            LayoutId::kAttributeError,
                            "function hex not found in module foo"));
}

TEST_F(UnderBuiltinsModuleTest, UnderPatchWithBadBaseFuncRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
not_a_function = 1234

@_patch
def not_a_function():
  pass
)"),
                            LayoutId::kTypeError,
                            "_patch can only patch functions"));
}

TEST_F(UnderBuiltinsModuleTest, UnderStrArrayIaddWithStrReturnsStrArray) {
  HandleScope scope(thread_);
  StrArray self(&scope, runtime_.newStrArray());
  const char* test_str = "hello";
  Str other(&scope, runtime_.newStrFromCStr(test_str));
  StrArray result(
      &scope, runBuiltin(UnderBuiltinsModule::underStrArrayIadd, self, other));
  EXPECT_TRUE(isStrEqualsCStr(runtime_.strFromStrArray(result), test_str));
  EXPECT_EQ(self, result);
}

TEST_F(UnderBuiltinsModuleDeathTest, UnderUnimplementedAbortsProgram) {
  ASSERT_DEATH(static_cast<void>(runFromCStr(&runtime_, "_unimplemented()")),
               ".*'_unimplemented' called.");
}

TEST_F(UnderBuiltinsModuleDeathTest, UnderUnimplementedPrintsFunctionName) {
  ASSERT_DEATH(static_cast<void>(runFromCStr(&runtime_, R"(
def foobar():
  _unimplemented()
foobar()
)")),
               ".*'_unimplemented' called in function 'foobar'.");
}

}  // namespace python
