#include "gtest/gtest.h"

#include "builtins-module.h"
#include "dict-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(DictBuiltinsTest, DictAtGrowsToInitialCapacity) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  EXPECT_EQ(dict.capacity(), 0);

  Object key(&scope, runtime.newInt(123));
  Object value(&scope, runtime.newInt(456));
  runtime.dictAtPut(thread, dict, key, value);
  int expected = Runtime::kInitialDictCapacity;
  EXPECT_EQ(dict.capacity(), expected);
}

TEST(DictBuiltinsTest, ClearRemovesAllElements) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  pass
d = {'a': C()}
)")
                   .isError());

  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, moduleAt(&runtime, "__main__", "d"));
  Object ref_obj(&scope, NoneType::object());
  {
    Object none(&scope, NoneType::object());
    Str key(&scope, runtime.newStrFromCStr("a"));
    Object c(&scope, runtime.dictAt(thread, dict, key));
    ref_obj = runtime.newWeakRef(thread, c, none);
  }
  WeakRef ref(&scope, *ref_obj);
  EXPECT_NE(ref.referent(), NoneType::object());
  runBuiltin(DictBuiltins::clear, dict);
  runtime.collectGarbage();
  EXPECT_EQ(ref.referent(), NoneType::object());
}

TEST(DictBuiltinsTest, CopyWithNonDictRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
result = dict.copy(None)
)"),
                            LayoutId::kTypeError,
                            "expected 'dict' instance but got NoneType"));
}

TEST(DictBuiltinsTest, CopyWithDictReturnsNewInstance) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
d = {'a': 3}
result = dict.copy(d)
)")
                   .isError());
  HandleScope scope;
  Object dict(&scope, moduleAt(&runtime, "__main__", "d"));
  EXPECT_TRUE(dict.isDict());
  Object result_obj(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(result_obj.isDict());
  Dict result(&scope, *result_obj);
  EXPECT_NE(*dict, *result);
  EXPECT_EQ(result.numItems(), 1);
  EXPECT_EQ(result.numUsableItems(), 5 - 1);
}

TEST(DictBuiltinsTest, DunderContainsWithExistingKeyReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = {'foo': 0}.__contains__('foo')")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isBool());
  EXPECT_TRUE(Bool::cast(*result).value());
}

TEST(DictBuiltinsTest, DunderContainsWithNonexistentKeyReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = {}.__contains__('foo')").isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isBool());
  EXPECT_FALSE(Bool::cast(*result).value());
}

TEST(DictBuiltinsTest, DunderContainsWithUnhashableTypeRaisesTypeError) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  __hash__ = None
c = C()
)")
                   .isError());
  EXPECT_TRUE(raised(runFromCStr(&runtime, "{}.__contains__(C())"),
                     LayoutId::kTypeError));
}

TEST(DictBuiltinsTest, DunderContainsWithNonCallableDunderHashRaisesTypeError) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  __hash__ = 4
)")
                   .isError());
  EXPECT_TRUE(raised(runFromCStr(&runtime, "{}.__contains__(C())"),
                     LayoutId::kTypeError));
}

TEST(DictBuiltinsTest,
     DunderContainsWithTypeWithDunderHashReturningNonIntRaisesTypeError) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __hash__(self):
    return "boo"
)")
                   .isError());
  EXPECT_TRUE(raised(runFromCStr(&runtime, "{}.__contains__(C())"),
                     LayoutId::kTypeError));
}

TEST(DictBuiltinsTest, InWithExistingKeyReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
d = {"foo": 1}
foo_in_d = "foo" in d
)")
                   .isError());
  HandleScope scope;
  Bool foo_in_d(&scope, moduleAt(&runtime, "__main__", "foo_in_d"));

  EXPECT_TRUE(foo_in_d.value());
}

TEST(DictBuiltinsTest, InWithNonexistentKeyReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
d = {}
foo_in_d = "foo" in d
)")
                   .isError());
  HandleScope scope;
  Bool foo_in_d(&scope, moduleAt(&runtime, "__main__", "foo_in_d"));

  EXPECT_FALSE(foo_in_d.value());
}

TEST(DictBuiltinsTest, DunderDelItemOnExistingKeyReturnsNone) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDictWithSize(1));
  Object key(&scope, runtime.newStrFromCStr("foo"));
  Object val(&scope, runtime.newInt(0));
  runtime.dictAtPut(thread, dict, key, val);
  RawObject result = runBuiltin(DictBuiltins::dunderDelItem, dict, key);
  EXPECT_TRUE(result.isNoneType());
}

TEST(DictBuiltinsTest, DunderDelItemOnNonexistentKeyRaisesKeyError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDictWithSize(1));
  Object key(&scope, runtime.newStrFromCStr("foo"));
  Object val(&scope, runtime.newInt(0));
  runtime.dictAtPut(thread, dict, key, val);

  // "bar" doesn't exist in this dictionary, attempting to delete it should
  // cause a KeyError.
  Object key2(&scope, runtime.newStrFromCStr("bar"));
  RawObject result = runBuiltin(DictBuiltins::dunderDelItem, dict, key2);
  ASSERT_TRUE(result.isError());
}

TEST(DictBuiltinsTest, DelOnObjectHashReturningNonIntRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class E:
  def __hash__(self): return "non int"

d = {}
del d[E()]
)"),
                            LayoutId::kTypeError,
                            "__hash__ method should return an integer"));
}

TEST(DictBuiltinsTest, DelOnExistingKeyDeletesKey) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
d = {"foo": 1}
del d["foo"]
)")
                   .isError());
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict d(&scope, moduleAt(&runtime, "__main__", "d"));
  Object foo(&scope, runtime.newStrFromCStr("foo"));

  EXPECT_FALSE(runtime.dictIncludes(thread, d, foo));
}

TEST(DictBuiltinsTest, DelOnNonexistentKeyRaisesKeyError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
d = {}
del d["foo"]
)"),
                            LayoutId::kKeyError, "foo"));
}

TEST(DictBuiltinsTest, DunderSetItemWithKeyHashReturningNonIntRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class E:
  def __hash__(self): return "non int"

d = {}
d[E()] = 4
)"),
                            LayoutId::kTypeError,
                            "__hash__ method should return an integer"));
}

TEST(DictBuiltinsTest, DunderSetItemWithExistingKey) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope;
  Dict dict(&scope, runtime.newDictWithSize(1));
  Object key(&scope, runtime.newStrFromCStr("foo"));
  Object val(&scope, runtime.newInt(0));
  Object val2(&scope, runtime.newInt(1));
  runtime.dictAtPut(thread, dict, key, val);

  Object result(&scope,
                runBuiltin(DictBuiltins::dunderSetItem, dict, key, val2));
  ASSERT_TRUE(result.isNoneType());
  ASSERT_EQ(dict.numItems(), 1);
  ASSERT_EQ(dict.numUsableItems(), 5 - 1);
  ASSERT_EQ(runtime.dictAt(thread, dict, key), *val2);
}

TEST(DictBuiltinsTest, DunderSetItemWithNonExistentKey) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope;
  Dict dict(&scope, runtime.newDictWithSize(1));
  ASSERT_EQ(dict.numItems(), 0);
  ASSERT_EQ(dict.numUsableItems(), 5);
  Object key(&scope, runtime.newStrFromCStr("foo"));
  Object val(&scope, runtime.newInt(0));
  Object result(&scope,
                runBuiltin(DictBuiltins::dunderSetItem, dict, key, val));
  ASSERT_TRUE(result.isNoneType());
  ASSERT_EQ(dict.numItems(), 1);
  ASSERT_EQ(dict.numUsableItems(), 5 - 1);
  ASSERT_EQ(runtime.dictAt(thread, dict, key), *val);
}

TEST(DictBuiltinsTest, NonTypeInDunderNew) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
dict.__new__(1)
)"),
                            LayoutId::kTypeError, "not a type object"));
}

TEST(DictBuiltinsTest, NonSubclassInDunderNew) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Foo: pass
dict.__new__(Foo)
)"),
                            LayoutId::kTypeError, "not a subtype of dict"));
}

TEST(DictBuiltinsTest, DunderNewConstructsDict) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kDict));
  Object result(&scope, runBuiltin(DictBuiltins::dunderNew, type));
  ASSERT_TRUE(result.isDict());
}

TEST(DictBuiltinsTest, DunderSetItemWithDictSubclassSetsItem) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class foo(dict):
  pass
d = foo()
)")
                   .isError());
  Dict dict(&scope, moduleAt(&runtime, "__main__", "d"));
  Str key(&scope, runtime.newStrFromCStr("a"));
  Str value(&scope, runtime.newStrFromCStr("b"));
  Object result1(&scope,
                 runBuiltin(DictBuiltins::dunderSetItem, dict, key, value));
  EXPECT_TRUE(result1.isNoneType());
  Object result2(&scope, runtime.dictAt(thread, dict, key));
  ASSERT_TRUE(result2.isStr());
  EXPECT_EQ(Str::cast(*result2), *value);
}

TEST(DictBuiltinsTest, DunderIterReturnsDictKeyIter) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  Object iter(&scope, runBuiltin(DictBuiltins::dunderIter, dict));
  ASSERT_TRUE(iter.isDictKeyIterator());
}

TEST(DictBuiltinsTest, DunderItemsReturnsDictItems) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  Object items(&scope, runBuiltin(DictBuiltins::items, dict));
  ASSERT_TRUE(items.isDictItems());
}

TEST(DictBuiltinsTest, KeysReturnsDictKeys) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  Object keys(&scope, runBuiltin(DictBuiltins::keys, dict));
  ASSERT_TRUE(keys.isDictKeys());
}

TEST(DictBuiltinsTest, ValuesReturnsDictValues) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  Object values(&scope, runBuiltin(DictBuiltins::values, dict));
  ASSERT_TRUE(values.isDictValues());
}

TEST(DictBuiltinsTest, DunderEqWithNonDict) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  Object not_a_dict(&scope, SmallInt::fromWord(5));
  EXPECT_EQ(runBuiltin(DictBuiltins::dunderEq, dict, not_a_dict),
            NotImplementedType::object());
}

TEST(DictBuiltinsTest, UpdateWithNoArgumentsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "dict.update()"), LayoutId::kTypeError,
      "TypeError: 'dict.update' takes min 1 positional arguments but 0 given"));
}

TEST(DictBuiltinsTest, UpdateWithNonDictRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raised(runFromCStr(&runtime, "dict.update([], None)"),
                     LayoutId::kTypeError));
}

TEST(DictBuiltinsTest, UpdateWithNonMappingTypeRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "dict.update({}, 1)"),
                            LayoutId::kTypeError,
                            "'int' object is not iterable"));
}

TEST(DictBuiltinsTest,
     UpdateWithListContainerWithObjectHashReturningNonIntRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class E:
  def __hash__(self): return "non int"

class C:
  def __init__(self):
    self.item = E()

  def __getitem__(self, idx):
    return self.item

  def keys(self):
    return [self.item]

dict.update({1:4}, C())
)"),
                            LayoutId::kTypeError,
                            "__hash__ method should return an integer"));
}

TEST(DictBuiltinsTest,
     UpdateWithTupleContainerWithObjectHashReturningNonIntRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class E:
  def __hash__(self): return "non int"

class C:
  def __init__(self):
    self.item = E()

  def __getitem__(self, idx):
    return self.item

  def keys(self):
    return (self.item,)

dict.update({1:4}, C())
)"),
                            LayoutId::kTypeError,
                            "__hash__ method should return an integer"));
}

TEST(DictBuiltinsTest,
     UpdateWithIterContainerWithObjectHashReturningNonIntRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class E:
  def __hash__(self): return "non int"

class C:
  def __init__(self):
    self.item = E()

  def __getitem__(self, idx):
    return self.item

  def keys(self):
    return iter([self.item])

dict.update({1:4}, C())
)"),
                            LayoutId::kTypeError,
                            "__hash__ method should return an integer"));
}

TEST(DictBuiltinsTest, UpdateWithDictReturnsUpdatedDict) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
d1 = {"a": 1, "b": 2}
d2 = {"c": 3, "d": 4}
d3 = {"a": 123}
)")
                   .isError());
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict d1(&scope, moduleAt(&runtime, "__main__", "d1"));
  Dict d2(&scope, moduleAt(&runtime, "__main__", "d2"));
  ASSERT_EQ(d1.numItems(), 2);
  ASSERT_EQ(d1.numUsableItems(), 5 - 2);
  ASSERT_EQ(d2.numItems(), 2);
  ASSERT_EQ(d2.numUsableItems(), 5 - 2);
  ASSERT_TRUE(runFromCStr(&runtime, "d1.update(d2)").isNoneType());
  EXPECT_EQ(d1.numItems(), 4);
  ASSERT_EQ(d1.numUsableItems(), 5 - 4);
  EXPECT_EQ(d2.numItems(), 2);
  ASSERT_EQ(d2.numUsableItems(), 5 - 2);

  ASSERT_TRUE(runFromCStr(&runtime, "d1.update(d3)").isNoneType());
  EXPECT_EQ(d1.numItems(), 4);
  ASSERT_EQ(d1.numUsableItems(), 5 - 4);
  Str a(&scope, runtime.newStrFromCStr("a"));
  Object a_val(&scope, runtime.dictAt(thread, d1, a));
  EXPECT_TRUE(isIntEqualsWord(*a_val, 123));
}

TEST(DictItemsBuiltinsTest, DunderIterReturnsIter) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  DictItems items(&scope, runtime.newDictItems(thread, dict));
  Object iter(&scope, runBuiltin(DictItemsBuiltins::dunderIter, items));
  ASSERT_TRUE(iter.isDictItemIterator());
}

TEST(DictKeysBuiltinsTest, DunderIterReturnsIter) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  DictKeys keys(&scope, runtime.newDictKeys(thread, dict));
  Object iter(&scope, runBuiltin(DictKeysBuiltins::dunderIter, keys));
  ASSERT_TRUE(iter.isDictKeyIterator());
}

TEST(DictValuesBuiltinsTest, DunderIterReturnsIter) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  DictValues values(&scope, runtime.newDictValues(thread, dict));
  Object iter(&scope, runBuiltin(DictValuesBuiltins::dunderIter, values));
  ASSERT_TRUE(iter.isDictValueIterator());
}

TEST(DictItemIteratorBuiltinsTest, CallDunderIterReturnsSelf) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  DictItemIterator iter(&scope, runtime.newDictItemIterator(thread, dict));
  // Now call __iter__ on the iterator object
  Object result(&scope, runBuiltin(DictItemIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST(DictKeyIteratorBuiltinsTest, CallDunderIterReturnsSelf) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  DictKeyIterator iter(&scope, runtime.newDictKeyIterator(thread, dict));
  // Now call __iter__ on the iterator object
  Object result(&scope, runBuiltin(DictKeyIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST(DictValueIteratorBuiltinsTest, CallDunderIterReturnsSelf) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  DictValueIterator iter(&scope, runtime.newDictValueIterator(thread, dict));
  // Now call __iter__ on the iterator object
  Object result(&scope,
                runBuiltin(DictValueIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST(DictItemIteratorBuiltinsTest,
     DunderLengthHintOnEmptyDictItemIteratorReturnsZero) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict empty_dict(&scope, runtime.newDict());
  DictItemIterator iter(&scope,
                        runtime.newDictItemIterator(thread, empty_dict));
  Object length_hint(
      &scope, runBuiltin(DictItemIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST(DictKeyIteratorBuiltinsTest,
     DunderLengthHintOnEmptyDictKeyIteratorReturnsZero) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict empty_dict(&scope, runtime.newDict());
  DictKeyIterator iter(&scope, runtime.newDictKeyIterator(thread, empty_dict));
  Object length_hint(
      &scope, runBuiltin(DictKeyIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST(DictValueIteratorBuiltinsTest,
     DunderLengthHintOnEmptyDictValueIteratorReturnsZero) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict empty_dict(&scope, runtime.newDict());
  DictValueIterator iter(&scope,
                         runtime.newDictValueIterator(thread, empty_dict));
  Object length_hint(
      &scope, runBuiltin(DictValueIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST(DictItemIteratorBuiltinsTest, CallDunderNextReadsItemsSequentially) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDictWithSize(5));
  Object hello(&scope, runtime.newStrFromCStr("hello"));
  Object world(&scope, runtime.newStrFromCStr("world"));
  Object goodbye(&scope, runtime.newStrFromCStr("goodbye"));
  Object moon(&scope, runtime.newStrFromCStr("moon"));
  runtime.dictAtPut(thread, dict, hello, world);
  runtime.dictAtPut(thread, dict, goodbye, moon);
  DictItemIterator iter(&scope, runtime.newDictItemIterator(thread, dict));

  Object item1(&scope, runBuiltin(DictItemIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item1.isTuple());
  EXPECT_EQ(Tuple::cast(*item1).at(0), hello);
  EXPECT_EQ(Tuple::cast(*item1).at(1), world);

  Object item2(&scope, runBuiltin(DictItemIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item2.isTuple());
  EXPECT_EQ(Tuple::cast(*item2).at(0), goodbye);
  EXPECT_EQ(Tuple::cast(*item2).at(1), moon);

  Object item3(&scope, runBuiltin(DictItemIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item3.isError());
}

TEST(DictKeyIteratorBuiltinsTest, CallDunderNextReadsKeysSequentially) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDictWithSize(5));
  Object hello(&scope, runtime.newStrFromCStr("hello"));
  Object world(&scope, runtime.newStrFromCStr("world"));
  Object goodbye(&scope, runtime.newStrFromCStr("goodbye"));
  Object moon(&scope, runtime.newStrFromCStr("moon"));
  runtime.dictAtPut(thread, dict, hello, world);
  runtime.dictAtPut(thread, dict, goodbye, moon);
  DictKeyIterator iter(&scope, runtime.newDictKeyIterator(thread, dict));

  Object item1(&scope, runBuiltin(DictKeyIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item1.isStr());
  EXPECT_EQ(Str::cast(*item1), hello);

  Object item2(&scope, runBuiltin(DictKeyIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item2.isStr());
  EXPECT_EQ(Str::cast(*item2), goodbye);

  Object item3(&scope, runBuiltin(DictKeyIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item3.isError());
}

TEST(DictValueIteratorBuiltinsTest, CallDunderNextReadsValuesSequentially) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDictWithSize(5));
  Object hello(&scope, runtime.newStrFromCStr("hello"));
  Object world(&scope, runtime.newStrFromCStr("world"));
  Object goodbye(&scope, runtime.newStrFromCStr("goodbye"));
  Object moon(&scope, runtime.newStrFromCStr("moon"));
  runtime.dictAtPut(thread, dict, hello, world);
  runtime.dictAtPut(thread, dict, goodbye, moon);
  DictValueIterator iter(&scope, runtime.newDictValueIterator(thread, dict));

  Object item1(&scope, runBuiltin(DictValueIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item1.isStr());
  EXPECT_EQ(Str::cast(*item1), world);

  Object item2(&scope, runBuiltin(DictValueIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item2.isStr());
  EXPECT_EQ(Str::cast(*item2), moon);

  Object item3(&scope, runBuiltin(DictValueIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item3.isError());
}

TEST(DictItemIteratorBuiltinsTest,
     DunderLengthHintOnConsumedDictItemIteratorReturnsZero) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  Object hello(&scope, runtime.newStrFromCStr("hello"));
  Object world(&scope, runtime.newStrFromCStr("world"));
  runtime.dictAtPut(thread, dict, hello, world);
  DictItemIterator iter(&scope, runtime.newDictItemIterator(thread, dict));

  Object item1(&scope, runBuiltin(DictItemIteratorBuiltins::dunderNext, iter));
  ASSERT_FALSE(item1.isError());

  Object length_hint(
      &scope, runBuiltin(DictItemIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST(DictKeyIteratorBuiltinsTest,
     DunderLengthHintOnConsumedDictKeyIteratorReturnsZero) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  Object hello(&scope, runtime.newStrFromCStr("hello"));
  Object world(&scope, runtime.newStrFromCStr("world"));
  runtime.dictAtPut(thread, dict, hello, world);
  DictKeyIterator iter(&scope, runtime.newDictKeyIterator(thread, dict));

  Object item1(&scope, runBuiltin(DictKeyIteratorBuiltins::dunderNext, iter));
  ASSERT_FALSE(item1.isError());

  Object length_hint(
      &scope, runBuiltin(DictKeyIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST(DictValueIteratorBuiltinsTest,
     DunderLengthHintOnConsumedDictValueIteratorReturnsZero) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  Object hello(&scope, runtime.newStrFromCStr("hello"));
  Object world(&scope, runtime.newStrFromCStr("world"));
  runtime.dictAtPut(thread, dict, hello, world);
  DictValueIterator iter(&scope, runtime.newDictValueIterator(thread, dict));

  Object item1(&scope, runBuiltin(DictValueIteratorBuiltins::dunderNext, iter));
  ASSERT_FALSE(item1.isError());

  Object length_hint(
      &scope, runBuiltin(DictValueIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST(DictBuiltinsTest, ItemIteratorNextOnOneElementDictReturnsElement) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  Object key(&scope, runtime.newStrFromCStr("hello"));
  Object value(&scope, runtime.newStrFromCStr("world"));
  runtime.dictAtPut(thread, dict, key, value);
  DictItemIterator iter(&scope, runtime.newDictItemIterator(thread, dict));
  Object next(&scope, dictItemIteratorNext(thread, iter));
  ASSERT_TRUE(next.isTuple());
  EXPECT_EQ(Tuple::cast(*next).at(0), key);
  EXPECT_EQ(Tuple::cast(*next).at(1), value);

  next = dictItemIteratorNext(thread, iter);
  ASSERT_TRUE(next.isError());
}

TEST(DictBuiltinsTest, KeyIteratorNextOnOneElementDictReturnsElement) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  Object key(&scope, runtime.newStrFromCStr("hello"));
  Object value(&scope, runtime.newStrFromCStr("world"));
  runtime.dictAtPut(thread, dict, key, value);
  DictKeyIterator iter(&scope, runtime.newDictKeyIterator(thread, dict));
  Object next(&scope, dictKeyIteratorNext(thread, iter));
  EXPECT_EQ(next, key);

  next = dictKeyIteratorNext(thread, iter);
  ASSERT_TRUE(next.isError());
}

TEST(DictBuiltinsTest, ValueIteratorNextOnOneElementDictReturnsElement) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  Object key(&scope, runtime.newStrFromCStr("hello"));
  Object value(&scope, runtime.newStrFromCStr("world"));
  runtime.dictAtPut(thread, dict, key, value);
  DictValueIterator iter(&scope, runtime.newDictValueIterator(thread, dict));
  Object next(&scope, dictValueIteratorNext(thread, iter));
  EXPECT_EQ(next, value);

  next = dictValueIteratorNext(thread, iter);
  ASSERT_TRUE(next.isError());
}

TEST(DictBuiltinsTest, GetWithNotEnoughArgumentsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "dict.get()"), LayoutId::kTypeError,
      "TypeError: 'dict.get' takes min 2 positional arguments but 0 given"));
}

TEST(DictBuiltinsTest, GetWithTooManyArgumentsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "dict.get({}, 123, 456, 789)"),
      LayoutId::kTypeError,
      "TypeError: 'dict.get' takes max 3 positional arguments but 4 given"));
}

TEST(DictBuiltinsTest, GetWithNonDictRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object foo(&scope, runtime.newInt(123));
  Object bar(&scope, runtime.newInt(456));
  Object baz(&scope, runtime.newInt(789));
  EXPECT_TRUE(raised(runBuiltin(DictBuiltins::get, foo, bar, baz),
                     LayoutId::kTypeError));
}

TEST(DictBuiltinsTest, GetWithUnhashableTypeRaisesTypeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  __hash__ = 2
key = Foo()
)")
                   .isError());
  Dict dict(&scope, runtime.newDict());
  Object key(&scope, moduleAt(&runtime, "__main__", "key"));
  Object default_obj(&scope, NoneType::object());
  ASSERT_TRUE(raised(runBuiltin(DictBuiltins::get, dict, key, default_obj),
                     LayoutId::kTypeError));
}

TEST(DictBuiltinsTest, GetWithIntSubclassHashReturnsDefaultValue) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class N(int):
  pass
class Foo:
  def __hash__(self):
    return N(12)
key = Foo()
)")
                   .isError());
  Dict dict(&scope, runtime.newDict());
  Object key(&scope, moduleAt(&runtime, "__main__", "key"));
  Object default_obj(&scope, runtime.newInt(5));
  EXPECT_EQ(runBuiltin(DictBuiltins::get, dict, key, default_obj), default_obj);
}

TEST(DictBuiltinsTest, GetReturnsDefaultValue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "res = {}.get(123, 456)").isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "res"), RawSmallInt::fromWord(456));
}

TEST(DictBuiltinsTest, GetReturnsNone) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = {}.get(123)").isError());
  EXPECT_TRUE(moduleAt(&runtime, "__main__", "result").isNoneType());
}

TEST(DictBuiltinsTest, GetReturnsValue) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  Object key(&scope, runtime.newInt(123));
  Object value(&scope, runtime.newInt(456));
  runtime.dictAtPut(thread, dict, key, value);
  Object dflt(&scope, runtime.newInt(789));
  Object result(&scope, runBuiltin(DictBuiltins::get, dict, key, dflt));
  EXPECT_TRUE(isIntEqualsWord(*result, 456));
}

TEST(DictBuiltinsTest, NextOnDictWithOnlyTombstonesReturnsFalse) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  Object key(&scope, runtime.newStrFromCStr("hello"));
  Object value(&scope, runtime.newStrFromCStr("world"));
  runtime.dictAtPut(thread, dict, key, value);
  ASSERT_FALSE(runtime.dictRemove(thread, dict, key).isError());
  Tuple data(&scope, dict.data());
  word i = Dict::Bucket::kFirst;
  ASSERT_FALSE(Dict::Bucket::nextItem(*data, &i));
}

TEST(DictBuiltinsTest, RecursiveDictPrintsEllipsis) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __init__(self, obj):
    self.val = obj
  def __repr__(self):
    return self.val.__repr__()
  def __hash__(self):
    return 5

d = dict()
c = C(d)
d['hello'] = c
result = d.__repr__()
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"),
                              "{'hello': {...}}"));
}

TEST(DictBuiltinsTest, PopWithKeyPresentReturnsValue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
d = {"hello": "world"}
result = d.pop("hello")
)")
                   .isError());
  HandleScope scope;
  EXPECT_TRUE(
      isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"), "world"));
  Dict dict(&scope, moduleAt(&runtime, "__main__", "d"));
  EXPECT_EQ(dict.numItems(), 0);
  EXPECT_EQ(dict.numUsableItems(), 5 - 1);
}

TEST(DictBuiltinsTest, PopWithMissingKeyAndDefaultReturnsDefault) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
d = {}
result = d.pop("hello", "world")
)")
                   .isError());
  HandleScope scope;
  Dict dict(&scope, moduleAt(&runtime, "__main__", "d"));
  EXPECT_EQ(dict.numItems(), 0);
  EXPECT_EQ(dict.numUsableItems(), 5);
  EXPECT_TRUE(
      isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"), "world"));
}

TEST(DictBuiltinsTest, PopWithMisingKeyRaisesKeyError) {
  Runtime runtime;
  EXPECT_TRUE(
      raised(runFromCStr(&runtime, "{}.pop('hello')"), LayoutId::kKeyError));
}

TEST(DictBuiltinsTest, PopWithSubclassDoesNotCallDunderDelItem) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C(dict):
    def __delitem__(self, key):
        raise Exception(key)
c = C({'hello': 'world'})
result = c.pop('hello')
)")
                   .isError());
  Thread* thread = Thread::current();
  ASSERT_FALSE(thread->hasPendingException());
  HandleScope scope(thread);
  Dict dict(&scope, moduleAt(&runtime, "__main__", "c"));
  EXPECT_EQ(dict.numItems(), 0);
  EXPECT_EQ(dict.numUsableItems(), 5 - 1);
  EXPECT_TRUE(
      isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"), "world"));
}

TEST(DictBuiltinsTest, DictInitWithSubclassInitializesElements) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C(dict):
    pass
c = C({'hello': 'world'})
)")
                   .isError());
  HandleScope scope;
  Dict dict(&scope, moduleAt(&runtime, "__main__", "c"));
  EXPECT_EQ(dict.numItems(), 1);
}

TEST(DictBuiltinsTest, SetDefaultWithNoDefaultSetsToNone) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
d = {}
d.setdefault("hello")
result = d["hello"]
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), NoneType::object());
}

TEST(DictBuiltinsTest, SetDefaultWithNotKeyInDictSetsDefault) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
d = {}
d.setdefault("hello", 4)
result = d["hello"]
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 4));
}

TEST(DictBuiltinsTest, SetDefaultWithKeyInDictReturnsValue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
d = {"hello": 5}
d.setdefault("hello", 4)
result = d["hello"]
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 5));
}

TEST(DictBuiltinsTest, nextBucketProbesAllBuckets) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  Object key(&scope, runtime.newInt(123));
  Object value(&scope, runtime.newInt(456));
  runtime.dictAtPut(thread, dict, key, value);

  Tuple data(&scope, dict.data());
  ASSERT_EQ(data.length(),
            Runtime::kInitialDictCapacity * Dict::Bucket::kNumPointers);

  bool probed[Runtime::kInitialDictCapacity] = {false};
  Int key_hash(&scope, SmallInt::fromWord(123));

  uword perturb;
  word bucket_mask;
  word current = Dict::Bucket::bucket(*data, *key_hash, &bucket_mask, &perturb);
  probed[current] = true;
  // Probe until perturb becomes zero.
  while (perturb > 0) {
    current = Dict::Bucket::nextBucket(current, bucket_mask, &perturb);
    probed[current] = true;
  }
  // Probe as many times as the capacity.
  for (word i = 1; i < Runtime::kInitialDictCapacity; ++i) {
    current = Dict::Bucket::nextBucket(current, bucket_mask, &perturb);
    probed[current] = true;
  }
  bool all_probed = true;
  for (word i = 0; i < Runtime::kInitialDictCapacity; ++i) {
    all_probed &= probed[i];
  }
  EXPECT_TRUE(all_probed);
}

TEST(DictBuiltinsTest, NumAttributesMatchesObjectSize) {
  Runtime runtime;
  HandleScope scope;
  Layout layout(&scope, runtime.layoutAt(LayoutId::kDict));
  EXPECT_EQ(layout.numInObjectAttributes(),
            (RawDict::kSize - RawHeapObject::kSize) / kPointerSize);
}

}  // namespace python
