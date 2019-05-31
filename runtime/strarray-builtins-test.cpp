#include "gtest/gtest.h"

#include "strarray-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(StrArrayBuiltinsTest, DunderNewAndDunderInitCreateStrArray) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, "obj = _strarray('hello')").isError());
  StrArray self(&scope, moduleAt(&runtime, "__main__", "obj"));
  EXPECT_TRUE(isStrEqualsCStr(runtime.strFromStrArray(self), "hello"));
}

TEST(StrArrayBuiltinsTest, DunderInitWithWrongTypeRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "obj = _strarray(b'hello')"),
                            LayoutId::kTypeError,
                            "_strarray can only be initialized with str"));
}

TEST(StrArrayBuiltinsTest, DunderReprWithNonStrArrayRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "_strarray.__repr__(b'')"),
                            LayoutId::kTypeError,
                            "'__repr__' requires a '_strarray' object"));
}

TEST(StrArrayBuiltinsTest, DunderReprWithSimpleStrArrayReturnsStr) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(
      runFromCStr(&runtime, "obj = _strarray('hello').__repr__()").isError());
  Str self(&scope, moduleAt(&runtime, "__main__", "obj"));
  EXPECT_TRUE(isStrEqualsCStr(*self, "_strarray('hello')"));
}

TEST(StrArrayBuiltinsTest, DunderStrWithNonStrArrayRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "_strarray.__str__(b'')"), LayoutId::kTypeError,
      "'__str__' requires a '_strarray' object but got 'bytes'"));
}

TEST(StrArrayBuiltinsTest, DunderStrWithEmptyStrArrayReturnsEmptyStr) {
  Runtime runtime;
  HandleScope scope;
  StrArray self(&scope, runtime.newStrArray());
  Object repr(&scope, runBuiltin(StrArrayBuiltins::dunderStr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, ""));
}

TEST(StrArrayBuiltinsTest, DunderStrWithSimpleStrArrayReturnsStr) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  StrArray self(&scope, runtime.newStrArray());
  const char* test_str = "foo";
  Str foo(&scope, runtime.newStrFromCStr(test_str));
  runtime.strArrayAddStr(thread, self, foo);
  Object repr(&scope, runBuiltin(StrArrayBuiltins::dunderStr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, test_str));
}

}  // namespace python
