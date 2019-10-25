#include "gtest/gtest.h"

#include "strarray-builtins.h"
#include "test-utils.h"

namespace py {

using namespace testing;

using StrArrayBuiltinsTest = RuntimeFixture;

TEST_F(StrArrayBuiltinsTest, DunderNewAndDunderInitCreateStrArray) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "obj = _strarray('hello')").isError());
  StrArray self(&scope, mainModuleAt(&runtime_, "obj"));
  EXPECT_TRUE(isStrEqualsCStr(runtime_.strFromStrArray(self), "hello"));
}

TEST_F(StrArrayBuiltinsTest, DunderInitWithWrongTypeRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "obj = _strarray(b'hello')"),
                            LayoutId::kTypeError,
                            "_strarray can only be initialized with str"));
}

TEST_F(StrArrayBuiltinsTest, DunderReprWithNonStrArrayRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "_strarray.__repr__(b'')"),
                            LayoutId::kTypeError,
                            "'__repr__' requires a '_strarray' object"));
}

TEST_F(StrArrayBuiltinsTest, DunderReprWithSimpleStrArrayReturnsStr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "obj = _strarray('hello').__repr__()").isError());
  Str self(&scope, mainModuleAt(&runtime_, "obj"));
  EXPECT_TRUE(isStrEqualsCStr(*self, "_strarray('hello')"));
}

TEST_F(StrArrayBuiltinsTest, DunderStrWithNonStrArrayRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "_strarray.__str__(b'')"), LayoutId::kTypeError,
      "'__str__' requires a '_strarray' object but got 'bytes'"));
}

TEST_F(StrArrayBuiltinsTest, DunderStrWithEmptyStrArrayReturnsEmptyStr) {
  HandleScope scope;
  StrArray self(&scope, runtime_.newStrArray());
  Object repr(&scope, runBuiltin(StrArrayBuiltins::dunderStr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, ""));
}

TEST_F(StrArrayBuiltinsTest, DunderStrWithSimpleStrArrayReturnsStr) {
  HandleScope scope(thread_);
  StrArray self(&scope, runtime_.newStrArray());
  const char* test_str = "foo";
  Str foo(&scope, runtime_.newStrFromCStr(test_str));
  runtime_.strArrayAddStr(thread_, self, foo);
  Object repr(&scope, runBuiltin(StrArrayBuiltins::dunderStr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, test_str));
}

}  // namespace py
