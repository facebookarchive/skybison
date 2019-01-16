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

TEST(DictBuiltinsTest, DunderContainsWithUnhashableTypeRaisesTypeError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class C:
  __hash__ = None
c = C()
)");
  HandleScope scope;
  Object dict(&scope, runtime.newDictWithSize(0));
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  Object result(&scope, runBuiltin(DictBuiltins::dunderContains, dict, c));
  ASSERT_TRUE(result->isError());
  Thread* thread = Thread::currentThread();
  ASSERT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
  EXPECT_TRUE(thread->pendingExceptionValue().isStr());
}

TEST(DictBuiltinsTest, DunderContainsWithNonCallableDunderHashRaisesTypeError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class C:
  __hash__ = 4
c = C()
)");
  HandleScope scope;
  Object dict(&scope, runtime.newDictWithSize(0));
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  Object result(&scope, runBuiltin(DictBuiltins::dunderContains, dict, c));
  ASSERT_TRUE(result->isError());
  Thread* thread = Thread::currentThread();
  ASSERT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
  EXPECT_TRUE(thread->pendingExceptionValue().isStr());
}

TEST(DictBuiltinsTest,
     DunderContainsWithTypeWithDunderHashReturningNonIntRaisesTypeError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class C:
  def __hash__(self):
    return "boo"
c = C()
)");
  HandleScope scope;
  Object dict(&scope, runtime.newDictWithSize(0));
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  Object result(&scope, runBuiltin(DictBuiltins::dunderContains, dict, c));
  ASSERT_TRUE(result->isError());
  Thread* thread = Thread::currentThread();
  ASSERT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
  EXPECT_TRUE(thread->pendingExceptionValue().isStr());
}

TEST(DictBuiltinsTest, InWithExistingKeyReturnsTrue) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
d = {"foo": 1}
foo_in_d = "foo" in d
)");
  HandleScope scope;
  Bool foo_in_d(&scope, moduleAt(&runtime, "__main__", "foo_in_d"));

  EXPECT_TRUE(foo_in_d->value());
}

TEST(DictBuiltinsTest, InWithNonexistentKeyReturnsFalse) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
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
  runFromCStr(&runtime, R"(
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
  EXPECT_DEATH(runFromCStr(&runtime, R"(
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

TEST(DictBuiltinsDeathTest, NonTypeInDunderNew) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
dict.__new__(1)
)"),
               "not a type object");
}

TEST(DictBuiltinsDeathTest, NonSubclassInDunderNew) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
class Foo: pass
dict.__new__(Foo)
)"),
               "not a subtype of dict");
}

TEST(DictBuiltinsTest, DunderNewConstructsDict) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kDict));
  Object result(&scope, runBuiltin(DictBuiltins::dunderNew, type));
  ASSERT_TRUE(result->isDict());
}

TEST(DictBuiltinsTest, DunderSetItemWithDictSubclassSetsItem) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class foo(dict):
  pass
d = foo()
)");
  Dict dict(&scope, moduleAt(&runtime, "__main__", "d"));
  Str key(&scope, runtime.newStrFromCStr("a"));
  Str value(&scope, runtime.newStrFromCStr("b"));
  Object result1(&scope,
                 runBuiltin(DictBuiltins::dunderSetItem, dict, key, value));
  EXPECT_TRUE(result1->isNoneType());
  Object result2(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(result2->isStr());
  EXPECT_EQ(RawStr::cast(*result2), *value);
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
  Object items(&scope, runBuiltin(DictBuiltins::items, dict));
  ASSERT_TRUE(items->isDictItems());
}

TEST(DictBuiltinsTest, KeysReturnsDictKeys) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object keys(&scope, runBuiltin(DictBuiltins::keys, dict));
  ASSERT_TRUE(keys->isDictKeys());
}

TEST(DictBuiltinsTest, ValuesReturnsDictValues) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object values(&scope, runBuiltin(DictBuiltins::values, dict));
  ASSERT_TRUE(values->isDictValues());
}

TEST(DictBuiltinsTest, DunderEqWithNonDict) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object not_a_dict(&scope, SmallInt::fromWord(5));
  EXPECT_EQ(runBuiltin(DictBuiltins::dunderEq, dict, not_a_dict),
            runtime.notImplemented());
}

TEST(DictBuiltinsTest, UpdateWithNoArgumentsThrowsTypeError) {
  Runtime runtime;
  EXPECT_TRUE(runBuiltin(DictBuiltins::update).isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(DictBuiltinsTest, UpdateWithNonDictThrowsTypeError) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  List list(&scope, runtime.newList());
  EXPECT_TRUE(runBuiltin(DictBuiltins::update, list).isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(DictBuiltinsDeathTest, UpdateWithNonMappingTypeThrowsTypeError) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  Int i(&scope, runtime.newInt(1));
  EXPECT_TRUE(runBuiltin(DictBuiltins::update, dict, i).isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(DictBuiltinsTest, UpdateWithDictReturnsUpdatedDict) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
d1 = {"a": 1, "b": 2}
d2 = {"c": 3, "d": 4}
d3 = {"a": 123}
)");
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Dict d1(&scope, moduleAt(&runtime, "__main__", "d1"));
  Dict d2(&scope, moduleAt(&runtime, "__main__", "d2"));
  ASSERT_EQ(d1->numItems(), 2);
  ASSERT_EQ(d2->numItems(), 2);
  ASSERT_TRUE(runBuiltin(DictBuiltins::update, d1, d2).isNoneType());
  EXPECT_EQ(d1->numItems(), 4);
  EXPECT_EQ(d2->numItems(), 2);

  Dict d3(&scope, moduleAt(&runtime, "__main__", "d3"));
  ASSERT_TRUE(runBuiltin(DictBuiltins::update, d1, d3).isNoneType());
  EXPECT_EQ(d1->numItems(), 4);
  Str a(&scope, runtime.newStrFromCStr("a"));
  Object a_val(&scope, runtime.dictAt(d1, a));
  ASSERT_TRUE(a_val->isInt());
  EXPECT_EQ(RawInt::cast(*a_val).asWord(), 123);
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

TEST(DictBuiltinsTest, ItemIteratorNextOnOneElementDictReturnsElement) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object key(&scope, runtime.newStrFromCStr("hello"));
  Object value(&scope, runtime.newStrFromCStr("world"));
  runtime.dictAtPut(dict, key, value);
  DictItemIterator iter(&scope, runtime.newDictItemIterator(dict));
  Object next(&scope, dictItemIteratorNext(Thread::currentThread(), iter));
  ASSERT_TRUE(next->isTuple());
  EXPECT_EQ(Tuple::cast(next)->at(0), key);
  EXPECT_EQ(Tuple::cast(next)->at(1), value);

  next = dictItemIteratorNext(Thread::currentThread(), iter);
  ASSERT_TRUE(next->isError());
}

TEST(DictBuiltinsTest, KeyIteratorNextOnOneElementDictReturnsElement) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object key(&scope, runtime.newStrFromCStr("hello"));
  Object value(&scope, runtime.newStrFromCStr("world"));
  runtime.dictAtPut(dict, key, value);
  DictKeyIterator iter(&scope, runtime.newDictKeyIterator(dict));
  Object next(&scope, dictKeyIteratorNext(Thread::currentThread(), iter));
  EXPECT_EQ(next, key);

  next = dictKeyIteratorNext(Thread::currentThread(), iter);
  ASSERT_TRUE(next->isError());
}

TEST(DictBuiltinsTest, ValueIteratorNextOnOneElementDictReturnsElement) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object key(&scope, runtime.newStrFromCStr("hello"));
  Object value(&scope, runtime.newStrFromCStr("world"));
  runtime.dictAtPut(dict, key, value);
  DictValueIterator iter(&scope, runtime.newDictValueIterator(dict));
  Object next(&scope, dictValueIteratorNext(Thread::currentThread(), iter));
  EXPECT_EQ(next, value);

  next = dictValueIteratorNext(Thread::currentThread(), iter);
  ASSERT_TRUE(next->isError());
}

TEST(DictBuiltinsTest, GetWithNotEnoughArgumentsThrowsTypeError) {
  Runtime runtime;
  HandleScope scope;
  EXPECT_TRUE(runBuiltin(DictBuiltins::get).isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(DictBuiltinsTest, GetWithTooManyArgumentsThrowsTypeError) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object foo(&scope, runtime.newInt(123));
  Object bar(&scope, runtime.newInt(456));
  Object baz(&scope, runtime.newInt(789));
  EXPECT_TRUE(runBuiltin(DictBuiltins::get, dict, foo, bar, baz).isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(DictBuiltinsTest, GetWithNonDictThrowsTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object foo(&scope, runtime.newInt(123));
  Object bar(&scope, runtime.newInt(456));
  Object baz(&scope, runtime.newInt(789));
  EXPECT_TRUE(runBuiltin(DictBuiltins::get, foo, bar, baz).isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(DictBuiltinsTest, GetWithUnhashableTypeThrowsTypeError) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Foo:
  __hash__ = 2
key = Foo()
)");
  Dict dict(&scope, runtime.newDict());
  Object key(&scope, moduleAt(&runtime, "__main__", "key"));
  Object result(&scope, runBuiltin(DictBuiltins::get, dict, key));
  ASSERT_TRUE(result->isError());
  ASSERT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(DictBuiltinsTest, GetReturnsDefaultValue) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object key(&scope, runtime.newInt(123));
  Object dflt(&scope, runtime.newInt(456));
  Object result(&scope, runBuiltin(DictBuiltins::get, dict, key, dflt));
  ASSERT_TRUE(result->isInt());
  EXPECT_EQ(Int::cast(result)->asWord(), 456);
}

TEST(DictBuiltinsTest, GetReturnsNone) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object key(&scope, runtime.newInt(123));
  Object result(&scope, runBuiltin(DictBuiltins::get, dict, key));
  EXPECT_TRUE(result->isNoneType());
}

TEST(DictBuiltinsTest, GetReturnsValue) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object key(&scope, runtime.newInt(123));
  Object value(&scope, runtime.newInt(456));
  runtime.dictAtPut(dict, key, value);
  Object dflt(&scope, runtime.newInt(789));
  Object result(&scope, runBuiltin(DictBuiltins::get, dict, key, dflt));
  ASSERT_TRUE(result->isInt());
  EXPECT_EQ(Int::cast(result)->asWord(), 456);
}

TEST(DictBuiltinsTest, NextOnDictWithOnlyTombstonesReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object key(&scope, runtime.newStrFromCStr("hello"));
  Object value(&scope, runtime.newStrFromCStr("world"));
  runtime.dictAtPut(dict, key, value);
  ASSERT_FALSE(runtime.dictRemove(dict, key).isError());
  Tuple data(&scope, dict->data());
  word i = Dict::Bucket::kFirst;
  ASSERT_FALSE(Dict::Bucket::nextItem(*data, &i));
}

}  // namespace python
