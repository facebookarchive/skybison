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
  Bool foo_in_d(&scope, moduleAt(&runtime, "__main__", "foo_in_d"));

  EXPECT_TRUE(foo_in_d->value());
}

TEST(DictBuiltinsTest, InWithNonexistentKeyReturnsFalse) {
  Runtime runtime;
  runtime.runFromCStr(R"(
d = {}
foo_in_d = "foo" in d
)");
  HandleScope scope;
  Bool foo_in_d(&scope, moduleAt(&runtime, "__main__", "foo_in_d"));

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
  Dict d(&scope, moduleAt(&runtime, "__main__", "d"));
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

TEST(DictBuiltinsTest, DunderIterReturnsDictKeyIter) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object iter(&scope, runBuiltin(DictBuiltins::dunderIter, dict));
  ASSERT_TRUE(iter->isDictKeyIterator());
}

TEST(DictBuiltinsTest, DunderItemsReturnsDictItems) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object items(&scope, runBuiltin(DictBuiltins::dunderItems, dict));
  ASSERT_TRUE(items->isDictItems());
}

TEST(DictBuiltinsTest, DunderKeysReturnsDictKeys) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object keys(&scope, runBuiltin(DictBuiltins::dunderKeys, dict));
  ASSERT_TRUE(keys->isDictKeys());
}

TEST(DictBuiltinsTest, DunderValuesReturnsDictValues) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object values(&scope, runBuiltin(DictBuiltins::dunderValues, dict));
  ASSERT_TRUE(values->isDictValues());
}

TEST(DictItemsBuiltinsTest, DunderIterReturnsIter) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  DictItems items(&scope, runtime.newDictItems(dict));
  Object iter(&scope, runBuiltin(DictItemsBuiltins::dunderIter, items));
  ASSERT_TRUE(iter->isDictItemIterator());
}

TEST(DictKeysBuiltinsTest, DunderIterReturnsIter) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  DictKeys keys(&scope, runtime.newDictKeys(dict));
  Object iter(&scope, runBuiltin(DictKeysBuiltins::dunderIter, keys));
  ASSERT_TRUE(iter->isDictKeyIterator());
}

TEST(DictValuesBuiltinsTest, DunderIterReturnsIter) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  DictValues values(&scope, runtime.newDictValues(dict));
  Object iter(&scope, runBuiltin(DictValuesBuiltins::dunderIter, values));
  ASSERT_TRUE(iter->isDictValueIterator());
}

TEST(DictItemIteratorBuiltinsTest, CallDunderIterReturnsSelf) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  DictItemIterator iter(&scope, runtime.newDictItemIterator(dict));
  // Now call __iter__ on the iterator object
  Object result(&scope, runBuiltin(DictItemIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST(DictKeyIteratorBuiltinsTest, CallDunderIterReturnsSelf) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  DictKeyIterator iter(&scope, runtime.newDictKeyIterator(dict));
  // Now call __iter__ on the iterator object
  Object result(&scope, runBuiltin(DictKeyIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST(DictValueIteratorBuiltinsTest, CallDunderIterReturnsSelf) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  DictValueIterator iter(&scope, runtime.newDictValueIterator(dict));
  // Now call __iter__ on the iterator object
  Object result(&scope,
                runBuiltin(DictValueIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST(DictItemIteratorBuiltinsTest,
     DunderLengthHintOnEmptyDictItemIteratorReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Dict empty_dict(&scope, runtime.newDict());
  DictItemIterator iter(&scope, runtime.newDictItemIterator(empty_dict));
  Object length_hint(
      &scope, runBuiltin(DictItemIteratorBuiltins::dunderLengthHint, iter));
  ASSERT_TRUE(length_hint->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint)->value(), 0);
}

TEST(DictKeyIteratorBuiltinsTest,
     DunderLengthHintOnEmptyDictKeyIteratorReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Dict empty_dict(&scope, runtime.newDict());
  DictKeyIterator iter(&scope, runtime.newDictKeyIterator(empty_dict));
  Object length_hint(
      &scope, runBuiltin(DictKeyIteratorBuiltins::dunderLengthHint, iter));
  ASSERT_TRUE(length_hint->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint)->value(), 0);
}

TEST(DictValueIteratorBuiltinsTest,
     DunderLengthHintOnEmptyDictValueIteratorReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Dict empty_dict(&scope, runtime.newDict());
  DictValueIterator iter(&scope, runtime.newDictValueIterator(empty_dict));
  Object length_hint(
      &scope, runBuiltin(DictValueIteratorBuiltins::dunderLengthHint, iter));
  ASSERT_TRUE(length_hint->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint)->value(), 0);
}

TEST(DictItemIteratorBuiltinsTest, CallDunderNextReadsItemsSequentially) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDictWithSize(5));
  Object hello(&scope, runtime.newStrFromCStr("hello"));
  Object world(&scope, runtime.newStrFromCStr("world"));
  Object goodbye(&scope, runtime.newStrFromCStr("goodbye"));
  Object moon(&scope, runtime.newStrFromCStr("moon"));
  runtime.dictAtPut(dict, hello, world);
  runtime.dictAtPut(dict, goodbye, moon);
  DictItemIterator iter(&scope, runtime.newDictItemIterator(dict));

  Object item1(&scope, runBuiltin(DictItemIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item1->isTuple());
  EXPECT_EQ(Tuple::cast(item1)->at(0), hello);
  EXPECT_EQ(Tuple::cast(item1)->at(1), world);

  Object item2(&scope, runBuiltin(DictItemIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item2->isTuple());
  EXPECT_EQ(Tuple::cast(item2)->at(0), goodbye);
  EXPECT_EQ(Tuple::cast(item2)->at(1), moon);

  Object item3(&scope, runBuiltin(DictItemIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item3->isError());
}

TEST(DictKeyIteratorBuiltinsTest, CallDunderNextReadsKeysSequentially) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDictWithSize(5));
  Object hello(&scope, runtime.newStrFromCStr("hello"));
  Object world(&scope, runtime.newStrFromCStr("world"));
  Object goodbye(&scope, runtime.newStrFromCStr("goodbye"));
  Object moon(&scope, runtime.newStrFromCStr("moon"));
  runtime.dictAtPut(dict, hello, world);
  runtime.dictAtPut(dict, goodbye, moon);
  DictKeyIterator iter(&scope, runtime.newDictKeyIterator(dict));

  Object item1(&scope, runBuiltin(DictKeyIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item1->isStr());
  EXPECT_EQ(Str::cast(item1), hello);

  Object item2(&scope, runBuiltin(DictKeyIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item2->isStr());
  EXPECT_EQ(Str::cast(item2), goodbye);

  Object item3(&scope, runBuiltin(DictKeyIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item3->isError());
}

TEST(DictValueIteratorBuiltinsTest, CallDunderNextReadsValuesSequentially) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDictWithSize(5));
  Object hello(&scope, runtime.newStrFromCStr("hello"));
  Object world(&scope, runtime.newStrFromCStr("world"));
  Object goodbye(&scope, runtime.newStrFromCStr("goodbye"));
  Object moon(&scope, runtime.newStrFromCStr("moon"));
  runtime.dictAtPut(dict, hello, world);
  runtime.dictAtPut(dict, goodbye, moon);
  DictValueIterator iter(&scope, runtime.newDictValueIterator(dict));

  Object item1(&scope, runBuiltin(DictValueIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item1->isStr());
  EXPECT_EQ(Str::cast(item1), world);

  Object item2(&scope, runBuiltin(DictValueIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item2->isStr());
  EXPECT_EQ(Str::cast(item2), moon);

  Object item3(&scope, runBuiltin(DictValueIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item3->isError());
}

TEST(DictItemIteratorBuiltinsTest,
     DunderLengthHintOnConsumedDictItemIteratorReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object hello(&scope, runtime.newStrFromCStr("hello"));
  Object world(&scope, runtime.newStrFromCStr("world"));
  runtime.dictAtPut(dict, hello, world);
  DictItemIterator iter(&scope, runtime.newDictItemIterator(dict));

  Object item1(&scope, runBuiltin(DictItemIteratorBuiltins::dunderNext, iter));
  ASSERT_FALSE(item1->isError());

  Object length_hint(
      &scope, runBuiltin(DictItemIteratorBuiltins::dunderLengthHint, iter));
  ASSERT_TRUE(length_hint->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint)->value(), 0);
}

TEST(DictKeyIteratorBuiltinsTest,
     DunderLengthHintOnConsumedDictKeyIteratorReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object hello(&scope, runtime.newStrFromCStr("hello"));
  Object world(&scope, runtime.newStrFromCStr("world"));
  runtime.dictAtPut(dict, hello, world);
  DictKeyIterator iter(&scope, runtime.newDictKeyIterator(dict));

  Object item1(&scope, runBuiltin(DictKeyIteratorBuiltins::dunderNext, iter));
  ASSERT_FALSE(item1->isError());

  Object length_hint(
      &scope, runBuiltin(DictKeyIteratorBuiltins::dunderLengthHint, iter));
  ASSERT_TRUE(length_hint->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint)->value(), 0);
}

TEST(DictValueIteratorBuiltinsTest,
     DunderLengthHintOnConsumedDictValueIteratorReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object hello(&scope, runtime.newStrFromCStr("hello"));
  Object world(&scope, runtime.newStrFromCStr("world"));
  runtime.dictAtPut(dict, hello, world);
  DictValueIterator iter(&scope, runtime.newDictValueIterator(dict));

  Object item1(&scope, runBuiltin(DictValueIteratorBuiltins::dunderNext, iter));
  ASSERT_FALSE(item1->isError());

  Object length_hint(
      &scope, runBuiltin(DictValueIteratorBuiltins::dunderLengthHint, iter));
  ASSERT_TRUE(length_hint->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint)->value(), 0);
}

}  // namespace python
