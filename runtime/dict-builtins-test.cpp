#include "gtest/gtest.h"

#include "builtins-module.h"
#include "dict-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(DictBuiltinsTest, DunderContainsWithExistingKeyReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDictWithSize(1));
  Object key(&scope, runtime.newStrFromCStr("foo"));
  Object val(&scope, runtime.newInt(0));
  runtime.dictAtPut(dict, key, val);

  RawObject result = runBuiltin(DictBuiltins::dunderContains, dict, key);
  ASSERT_TRUE(result->isBool());
  EXPECT_TRUE(RawBool::cast(result)->value());
}

TEST(DictBuiltinsTest, DunderContainsWithNonexistentKeyReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object dict(&scope, runtime.newDictWithSize(0));
  Object str(&scope, runtime.newStrFromCStr("foo"));
  RawObject result = runBuiltin(DictBuiltins::dunderContains, dict, str);
  ASSERT_TRUE(result->isBool());
  EXPECT_FALSE(RawBool::cast(result)->value());
}

TEST(DictBuiltinsTest, InWithExistingKeyReturnsTrue) {
  Runtime runtime;
  runtime.runFromCStr(R"(
d = {"foo": 1}
foo_in_d = "foo" in d
)");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Bool foo_in_d(&scope, moduleAt(&runtime, main, "foo_in_d"));

  EXPECT_TRUE(foo_in_d->value());
}

TEST(DictBuiltinsTest, InWithNonexistentKeyReturnsFalse) {
  Runtime runtime;
  runtime.runFromCStr(R"(
d = {}
foo_in_d = "foo" in d
)");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Bool foo_in_d(&scope, moduleAt(&runtime, main, "foo_in_d"));

  EXPECT_FALSE(foo_in_d->value());
}

TEST(DictBuiltinsTest, DunderDelItemOnExistingKeyReturnsNone) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDictWithSize(1));
  Object key(&scope, runtime.newStrFromCStr("foo"));
  Object val(&scope, runtime.newInt(0));
  runtime.dictAtPut(dict, key, val);
  RawObject result = runBuiltin(DictBuiltins::dunderDelItem, dict, key);
  EXPECT_TRUE(result->isNoneType());
}

TEST(DictBuiltinsTest, DunderDelItemOnNonexistentKeyThrowsKeyError) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDictWithSize(1));
  Object key(&scope, runtime.newStrFromCStr("foo"));
  Object val(&scope, runtime.newInt(0));
  runtime.dictAtPut(dict, key, val);

  // "bar" doesn't exist in this dictionary, attempting to delete it should
  // cause a KeyError.
  Object key2(&scope, runtime.newStrFromCStr("bar"));
  RawObject result = runBuiltin(DictBuiltins::dunderDelItem, dict, key2);
  ASSERT_TRUE(result->isError());
}

TEST(DictBuiltinsTest, DelOnExistingKeyDeletesKey) {
  Runtime runtime;
  runtime.runFromCStr(R"(
d = {"foo": 1}
del d["foo"]
)");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Dict d(&scope, moduleAt(&runtime, main, "d"));
  Object foo(&scope, runtime.newStrFromCStr("foo"));

  EXPECT_FALSE(runtime.dictIncludes(d, foo));
}

TEST(DictBuiltinsDeathTest, DelOnNonexistentKeyThrowsKeyError) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
d = {}
del d["foo"]
)"),
               "aborting due to pending exception");
}

TEST(DictBuiltinsTest, DunderSetItemWithExistingKey) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDictWithSize(1));
  Object key(&scope, runtime.newStrFromCStr("foo"));
  Object val(&scope, runtime.newInt(0));
  Object val2(&scope, runtime.newInt(1));
  runtime.dictAtPut(dict, key, val);

  Object result(&scope,
                runBuiltin(DictBuiltins::dunderSetItem, dict, key, val2));
  ASSERT_TRUE(result->isNoneType());
  ASSERT_EQ(dict->numItems(), 1);
  ASSERT_EQ(runtime.dictAt(dict, key), *val2);
}

TEST(DictBuiltinsTest, DunderSetItemWithNonExistentKey) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDictWithSize(1));
  ASSERT_EQ(dict->numItems(), 0);
  Object key(&scope, runtime.newStrFromCStr("foo"));
  Object val(&scope, runtime.newInt(0));
  Object result(&scope,
                runBuiltin(DictBuiltins::dunderSetItem, dict, key, val));
  ASSERT_TRUE(result->isNoneType());
  ASSERT_EQ(dict->numItems(), 1);
  ASSERT_EQ(runtime.dictAt(dict, key), *val);
}

}  // namespace python
