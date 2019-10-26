#include "gtest/gtest.h"

#include "builtins-module.h"
#include "dict-builtins.h"
#include "runtime.h"
#include "str-builtins.h"
#include "test-utils.h"

namespace py {

using namespace testing;

using DictBuiltinsTest = RuntimeFixture;
using DictItemIteratorBuiltinsTest = RuntimeFixture;
using DictItemsBuiltinsTest = RuntimeFixture;
using DictKeyIteratorBuiltinsTest = RuntimeFixture;
using DictKeysBuiltinsTest = RuntimeFixture;
using DictValueIteratorBuiltinsTest = RuntimeFixture;
using DictValuesBuiltinsTest = RuntimeFixture;

TEST_F(DictBuiltinsTest, DictAtGrowsToInitialCapacity) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  EXPECT_EQ(dict.capacity(), 0);

  Object key(&scope, runtime_.newInt(123));
  Object hash(&scope, Interpreter::hash(thread_, key));
  ASSERT_FALSE(hash.isErrorException());
  Object value(&scope, runtime_.newInt(456));
  runtime_.dictAtPut(thread_, dict, key, hash, value);
  int expected = Runtime::kInitialDictCapacity;
  EXPECT_EQ(dict.capacity(), expected);
}

TEST_F(DictBuiltinsTest, ClearWithEmptyDictIsNoop) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  EXPECT_EQ(runBuiltin(DictBuiltins::clear, dict), NoneType::object());
}

TEST_F(DictBuiltinsTest, ClearWithNonEmptyDictRemovesAllElements) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  pass
d = {'a': C()}
)")
                   .isError());

  HandleScope scope(thread_);
  Dict dict(&scope, mainModuleAt(&runtime_, "d"));
  Object ref_obj(&scope, NoneType::object());
  {
    Object none(&scope, NoneType::object());
    Str key(&scope, runtime_.newStrFromCStr("a"));
    Object c(&scope, runtime_.dictAtByStr(thread_, dict, key));
    ref_obj = runtime_.newWeakRef(thread_, c, none);
  }
  WeakRef ref(&scope, *ref_obj);
  EXPECT_NE(ref.referent(), NoneType::object());
  runBuiltin(DictBuiltins::clear, dict);
  runtime_.collectGarbage();
  EXPECT_EQ(ref.referent(), NoneType::object());
}

TEST_F(DictBuiltinsTest, CopyWithDictReturnsNewInstance) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
d = {'a': 3}
result = dict.copy(d)
)")
                   .isError());
  HandleScope scope(thread_);
  Object dict(&scope, mainModuleAt(&runtime_, "d"));
  EXPECT_TRUE(dict.isDict());
  Object result_obj(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(result_obj.isDict());
  Dict result(&scope, *result_obj);
  EXPECT_NE(*dict, *result);
  EXPECT_EQ(result.numItems(), 1);
  EXPECT_EQ(result.numUsableItems(), 5 - 1);
}

TEST_F(DictBuiltinsTest, DunderContainsWithExistingKeyReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, "result = {'foo': 0}.__contains__('foo')")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  ASSERT_TRUE(result.isBool());
  EXPECT_TRUE(Bool::cast(*result).value());
}

TEST_F(DictBuiltinsTest, DunderContainsWithNonexistentKeyReturnsFalse) {
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = {}.__contains__('foo')").isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  ASSERT_TRUE(result.isBool());
  EXPECT_FALSE(Bool::cast(*result).value());
}

TEST_F(DictBuiltinsTest, DunderContainsWithUnhashableTypeRaisesTypeError) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  __hash__ = None
c = C()
)")
                   .isError());
  EXPECT_TRUE(raised(runFromCStr(&runtime_, "{}.__contains__(C())"),
                     LayoutId::kTypeError));
}

TEST_F(DictBuiltinsTest,
       DunderContainsWithNonCallableDunderHashRaisesTypeError) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  __hash__ = 4
)")
                   .isError());
  EXPECT_TRUE(raised(runFromCStr(&runtime_, "{}.__contains__(C())"),
                     LayoutId::kTypeError));
}

TEST_F(DictBuiltinsTest,
       DunderContainsWithTypeWithDunderHashReturningNonIntRaisesTypeError) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __hash__(self):
    return "boo"
)")
                   .isError());
  EXPECT_TRUE(raised(runFromCStr(&runtime_, "{}.__contains__(C())"),
                     LayoutId::kTypeError));
}

TEST_F(DictBuiltinsTest, InWithExistingKeyReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
d = {"foo": 1}
foo_in_d = "foo" in d
)")
                   .isError());
  HandleScope scope(thread_);
  Bool foo_in_d(&scope, mainModuleAt(&runtime_, "foo_in_d"));

  EXPECT_TRUE(foo_in_d.value());
}

TEST_F(DictBuiltinsTest, InWithNonexistentKeyReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
d = {}
foo_in_d = "foo" in d
)")
                   .isError());
  HandleScope scope(thread_);
  Bool foo_in_d(&scope, mainModuleAt(&runtime_, "foo_in_d"));

  EXPECT_FALSE(foo_in_d.value());
}

TEST_F(DictBuiltinsTest, DunderDelItemOnExistingKeyReturnsNone) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDictWithSize(1));
  Str key(&scope, runtime_.newStrFromCStr("foo"));
  Object val(&scope, runtime_.newInt(0));
  runtime_.dictAtPutByStr(thread_, dict, key, val);
  RawObject result = runBuiltin(DictBuiltins::dunderDelItem, dict, key);
  EXPECT_TRUE(result.isNoneType());
}

TEST_F(DictBuiltinsTest, DunderDelItemOnNonexistentKeyRaisesKeyError) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDictWithSize(1));
  Str key(&scope, runtime_.newStrFromCStr("foo"));
  Object val(&scope, runtime_.newInt(0));
  runtime_.dictAtPutByStr(thread_, dict, key, val);

  // "bar" doesn't exist in this dictionary, attempting to delete it should
  // cause a KeyError.
  Object key2(&scope, runtime_.newStrFromCStr("bar"));
  RawObject result = runBuiltin(DictBuiltins::dunderDelItem, dict, key2);
  ASSERT_TRUE(result.isError());
}

TEST_F(DictBuiltinsTest, DelOnObjectHashReturningNonIntRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class E:
  def __hash__(self): return "non int"

d = {}
del d[E()]
)"),
                            LayoutId::kTypeError,
                            "__hash__ method should return an integer"));
}

TEST_F(DictBuiltinsTest, DelOnExistingKeyDeletesKey) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
d = {"foo": 1}
del d["foo"]
)")
                   .isError());
  HandleScope scope(thread_);
  Dict d(&scope, mainModuleAt(&runtime_, "d"));
  Str foo(&scope, runtime_.newStrFromCStr("foo"));

  EXPECT_FALSE(runtime_.dictIncludesByStr(thread_, d, foo));
}

TEST_F(DictBuiltinsTest, DelOnNonexistentKeyRaisesKeyError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
d = {}
del d["foo"]
)"),
                            LayoutId::kKeyError, "foo"));
}

TEST_F(DictBuiltinsTest,
       DunderSetItemWithKeyHashReturningNonIntRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class E:
  def __hash__(self): return "non int"

d = {}
d[E()] = 4
)"),
                            LayoutId::kTypeError,
                            "__hash__ method should return an integer"));
}

TEST_F(DictBuiltinsTest, DunderSetItemWithExistingKey) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDictWithSize(1));
  Str key(&scope, runtime_.newStrFromCStr("foo"));
  Object val(&scope, runtime_.newInt(0));
  Object val2(&scope, runtime_.newInt(1));
  runtime_.dictAtPutByStr(thread_, dict, key, val);

  Object result(&scope,
                runBuiltin(DictBuiltins::dunderSetItem, dict, key, val2));
  ASSERT_TRUE(result.isNoneType());
  ASSERT_EQ(dict.numItems(), 1);
  ASSERT_EQ(dict.numUsableItems(), 5 - 1);
  ASSERT_EQ(runtime_.dictAtByStr(thread_, dict, key), *val2);
}

TEST_F(DictBuiltinsTest, DunderSetItemWithNonExistentKey) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDictWithSize(1));
  ASSERT_EQ(dict.numItems(), 0);
  ASSERT_EQ(dict.numUsableItems(), 5);
  Str key(&scope, runtime_.newStrFromCStr("foo"));
  Object val(&scope, runtime_.newInt(0));
  Object result(&scope,
                runBuiltin(DictBuiltins::dunderSetItem, dict, key, val));
  ASSERT_TRUE(result.isNoneType());
  ASSERT_EQ(dict.numItems(), 1);
  ASSERT_EQ(dict.numUsableItems(), 5 - 1);
  ASSERT_EQ(runtime_.dictAtByStr(thread_, dict, key), *val);
}

TEST_F(DictBuiltinsTest, NonTypeInDunderNew) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
dict.__new__(1)
)"),
                            LayoutId::kTypeError, "not a type object"));
}

TEST_F(DictBuiltinsTest, NonSubclassInDunderNew) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class Foo: pass
dict.__new__(Foo)
)"),
                            LayoutId::kTypeError, "not a subtype of dict"));
}

TEST_F(DictBuiltinsTest, DunderNewConstructsDict) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_.typeAt(LayoutId::kDict));
  Object result(&scope, runBuiltin(DictBuiltins::dunderNew, type));
  ASSERT_TRUE(result.isDict());
}

TEST_F(DictBuiltinsTest, DunderSetItemWithDictSubclassSetsItem) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class foo(dict):
  pass
d = foo()
)")
                   .isError());
  Dict dict(&scope, mainModuleAt(&runtime_, "d"));
  Str key(&scope, runtime_.newStrFromCStr("a"));
  Str value(&scope, runtime_.newStrFromCStr("b"));
  Object result1(&scope,
                 runBuiltin(DictBuiltins::dunderSetItem, dict, key, value));
  EXPECT_TRUE(result1.isNoneType());
  Object result2(&scope, runtime_.dictAtByStr(thread_, dict, key));
  ASSERT_TRUE(result2.isStr());
  EXPECT_EQ(Str::cast(*result2), *value);
}

TEST_F(DictBuiltinsTest, DunderIterReturnsDictKeyIter) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  Object iter(&scope, runBuiltin(DictBuiltins::dunderIter, dict));
  ASSERT_TRUE(iter.isDictKeyIterator());
}

TEST_F(DictBuiltinsTest, DunderItemsReturnsDictItems) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  Object items(&scope, runBuiltin(DictBuiltins::items, dict));
  ASSERT_TRUE(items.isDictItems());
}

TEST_F(DictBuiltinsTest, KeysReturnsDictKeys) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  Object keys(&scope, runBuiltin(DictBuiltins::keys, dict));
  ASSERT_TRUE(keys.isDictKeys());
}

TEST_F(DictBuiltinsTest, ValuesReturnsDictValues) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  Object values(&scope, runBuiltin(DictBuiltins::values, dict));
  ASSERT_TRUE(values.isDictValues());
}

TEST_F(DictBuiltinsTest, UpdateWithNoArgumentsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "dict.update()"), LayoutId::kTypeError,
      "TypeError: 'dict.update' takes min 1 positional arguments but 0 given"));
}

TEST_F(DictBuiltinsTest, UpdateWithNonDictRaisesTypeError) {
  EXPECT_TRUE(raised(runFromCStr(&runtime_, "dict.update([], None)"),
                     LayoutId::kTypeError));
}

TEST_F(DictBuiltinsTest, UpdateWithNonMappingTypeRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "dict.update({}, 1)"),
                            LayoutId::kTypeError,
                            "'int' object is not iterable"));
}

TEST_F(DictBuiltinsTest,
       UpdateWithListContainerWithObjectHashReturningNonIntRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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

TEST_F(DictBuiltinsTest,
       UpdateWithTupleContainerWithObjectHashReturningNonIntRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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

TEST_F(DictBuiltinsTest,
       UpdateWithIterContainerWithObjectHashReturningNonIntRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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

TEST_F(DictBuiltinsTest, UpdateWithDictReturnsUpdatedDict) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
d1 = {"a": 1, "b": 2}
d2 = {"c": 3, "d": 4}
d3 = {"a": 123}
)")
                   .isError());
  HandleScope scope(thread_);
  Dict d1(&scope, mainModuleAt(&runtime_, "d1"));
  Dict d2(&scope, mainModuleAt(&runtime_, "d2"));
  ASSERT_EQ(d1.numItems(), 2);
  ASSERT_EQ(d1.numUsableItems(), 5 - 2);
  ASSERT_EQ(d2.numItems(), 2);
  ASSERT_EQ(d2.numUsableItems(), 5 - 2);
  ASSERT_TRUE(runFromCStr(&runtime_, "d1.update(d2)").isNoneType());
  EXPECT_EQ(d1.numItems(), 4);
  ASSERT_EQ(d1.numUsableItems(), 5 - 4);
  EXPECT_EQ(d2.numItems(), 2);
  ASSERT_EQ(d2.numUsableItems(), 5 - 2);

  ASSERT_TRUE(runFromCStr(&runtime_, "d1.update(d3)").isNoneType());
  EXPECT_EQ(d1.numItems(), 4);
  ASSERT_EQ(d1.numUsableItems(), 5 - 4);
  Str a(&scope, runtime_.newStrFromCStr("a"));
  Object a_val(&scope, runtime_.dictAtByStr(thread_, d1, a));
  EXPECT_TRUE(isIntEqualsWord(*a_val, 123));
}

TEST_F(DictItemsBuiltinsTest, DunderIterReturnsIter) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  DictItems items(&scope, runtime_.newDictItems(thread_, dict));
  Object iter(&scope, runBuiltin(DictItemsBuiltins::dunderIter, items));
  ASSERT_TRUE(iter.isDictItemIterator());
}

TEST_F(DictKeysBuiltinsTest, DunderIterReturnsIter) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  DictKeys keys(&scope, runtime_.newDictKeys(thread_, dict));
  Object iter(&scope, runBuiltin(DictKeysBuiltins::dunderIter, keys));
  ASSERT_TRUE(iter.isDictKeyIterator());
}

TEST_F(DictValuesBuiltinsTest, DunderIterReturnsIter) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  DictValues values(&scope, runtime_.newDictValues(thread_, dict));
  Object iter(&scope, runBuiltin(DictValuesBuiltins::dunderIter, values));
  ASSERT_TRUE(iter.isDictValueIterator());
}

TEST_F(DictItemIteratorBuiltinsTest, CallDunderIterReturnsSelf) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  DictItemIterator iter(&scope, runtime_.newDictItemIterator(thread_, dict));
  // Now call __iter__ on the iterator object
  Object result(&scope, runBuiltin(DictItemIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST_F(DictKeyIteratorBuiltinsTest, CallDunderIterReturnsSelf) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  DictKeyIterator iter(&scope, runtime_.newDictKeyIterator(thread_, dict));
  // Now call __iter__ on the iterator object
  Object result(&scope, runBuiltin(DictKeyIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST_F(DictValueIteratorBuiltinsTest, CallDunderIterReturnsSelf) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  DictValueIterator iter(&scope, runtime_.newDictValueIterator(thread_, dict));
  // Now call __iter__ on the iterator object
  Object result(&scope,
                runBuiltin(DictValueIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST_F(DictItemIteratorBuiltinsTest,
       DunderLengthHintOnEmptyDictItemIteratorReturnsZero) {
  HandleScope scope(thread_);
  Dict empty_dict(&scope, runtime_.newDict());
  DictItemIterator iter(&scope,
                        runtime_.newDictItemIterator(thread_, empty_dict));
  Object length_hint(
      &scope, runBuiltin(DictItemIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST_F(DictKeyIteratorBuiltinsTest,
       DunderLengthHintOnEmptyDictKeyIteratorReturnsZero) {
  HandleScope scope(thread_);
  Dict empty_dict(&scope, runtime_.newDict());
  DictKeyIterator iter(&scope,
                       runtime_.newDictKeyIterator(thread_, empty_dict));
  Object length_hint(
      &scope, runBuiltin(DictKeyIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST_F(DictValueIteratorBuiltinsTest,
       DunderLengthHintOnEmptyDictValueIteratorReturnsZero) {
  HandleScope scope(thread_);
  Dict empty_dict(&scope, runtime_.newDict());
  DictValueIterator iter(&scope,
                         runtime_.newDictValueIterator(thread_, empty_dict));
  Object length_hint(
      &scope, runBuiltin(DictValueIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST_F(DictItemIteratorBuiltinsTest, CallDunderNextReadsItemsSequentially) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDictWithSize(5));
  Str hello(&scope, runtime_.newStrFromCStr("hello"));
  Object world(&scope, runtime_.newStrFromCStr("world"));
  Str goodbye(&scope, runtime_.newStrFromCStr("goodbye"));
  Object moon(&scope, runtime_.newStrFromCStr("moon"));
  runtime_.dictAtPutByStr(thread_, dict, hello, world);
  runtime_.dictAtPutByStr(thread_, dict, goodbye, moon);
  DictItemIterator iter(&scope, runtime_.newDictItemIterator(thread_, dict));

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

TEST_F(DictKeyIteratorBuiltinsTest, CallDunderNextReadsKeysSequentially) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDictWithSize(5));
  Str hello(&scope, runtime_.newStrFromCStr("hello"));
  Object world(&scope, runtime_.newStrFromCStr("world"));
  Str goodbye(&scope, runtime_.newStrFromCStr("goodbye"));
  Object moon(&scope, runtime_.newStrFromCStr("moon"));
  runtime_.dictAtPutByStr(thread_, dict, hello, world);
  runtime_.dictAtPutByStr(thread_, dict, goodbye, moon);
  DictKeyIterator iter(&scope, runtime_.newDictKeyIterator(thread_, dict));

  Object item1(&scope, runBuiltin(DictKeyIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item1.isStr());
  EXPECT_EQ(Str::cast(*item1), hello);

  Object item2(&scope, runBuiltin(DictKeyIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item2.isStr());
  EXPECT_EQ(Str::cast(*item2), goodbye);

  Object item3(&scope, runBuiltin(DictKeyIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item3.isError());
}

TEST_F(DictValueIteratorBuiltinsTest, CallDunderNextReadsValuesSequentially) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDictWithSize(5));
  Str hello(&scope, runtime_.newStrFromCStr("hello"));
  Object world(&scope, runtime_.newStrFromCStr("world"));
  Str goodbye(&scope, runtime_.newStrFromCStr("goodbye"));
  Object moon(&scope, runtime_.newStrFromCStr("moon"));
  runtime_.dictAtPutByStr(thread_, dict, hello, world);
  runtime_.dictAtPutByStr(thread_, dict, goodbye, moon);
  DictValueIterator iter(&scope, runtime_.newDictValueIterator(thread_, dict));

  Object item1(&scope, runBuiltin(DictValueIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item1.isStr());
  EXPECT_EQ(Str::cast(*item1), world);

  Object item2(&scope, runBuiltin(DictValueIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item2.isStr());
  EXPECT_EQ(Str::cast(*item2), moon);

  Object item3(&scope, runBuiltin(DictValueIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item3.isError());
}

TEST_F(DictItemIteratorBuiltinsTest,
       DunderLengthHintOnConsumedDictItemIteratorReturnsZero) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  Str hello(&scope, runtime_.newStrFromCStr("hello"));
  Object world(&scope, runtime_.newStrFromCStr("world"));
  runtime_.dictAtPutByStr(thread_, dict, hello, world);
  DictItemIterator iter(&scope, runtime_.newDictItemIterator(thread_, dict));

  Object item1(&scope, runBuiltin(DictItemIteratorBuiltins::dunderNext, iter));
  ASSERT_FALSE(item1.isError());

  Object length_hint(
      &scope, runBuiltin(DictItemIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST_F(DictKeyIteratorBuiltinsTest,
       DunderLengthHintOnConsumedDictKeyIteratorReturnsZero) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  Str hello(&scope, runtime_.newStrFromCStr("hello"));
  Object world(&scope, runtime_.newStrFromCStr("world"));
  runtime_.dictAtPutByStr(thread_, dict, hello, world);
  DictKeyIterator iter(&scope, runtime_.newDictKeyIterator(thread_, dict));

  Object item1(&scope, runBuiltin(DictKeyIteratorBuiltins::dunderNext, iter));
  ASSERT_FALSE(item1.isError());

  Object length_hint(
      &scope, runBuiltin(DictKeyIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST_F(DictValueIteratorBuiltinsTest,
       DunderLengthHintOnConsumedDictValueIteratorReturnsZero) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  Str hello(&scope, runtime_.newStrFromCStr("hello"));
  Object world(&scope, runtime_.newStrFromCStr("world"));
  runtime_.dictAtPutByStr(thread_, dict, hello, world);
  DictValueIterator iter(&scope, runtime_.newDictValueIterator(thread_, dict));

  Object item1(&scope, runBuiltin(DictValueIteratorBuiltins::dunderNext, iter));
  ASSERT_FALSE(item1.isError());

  Object length_hint(
      &scope, runBuiltin(DictValueIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST_F(DictBuiltinsTest, ItemIteratorNextOnOneElementDictReturnsElement) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  Str key(&scope, runtime_.newStrFromCStr("hello"));
  Object value(&scope, runtime_.newStrFromCStr("world"));
  runtime_.dictAtPutByStr(thread_, dict, key, value);
  DictItemIterator iter(&scope, runtime_.newDictItemIterator(thread_, dict));
  Object next(&scope, dictItemIteratorNext(thread_, iter));
  ASSERT_TRUE(next.isTuple());
  EXPECT_EQ(Tuple::cast(*next).at(0), key);
  EXPECT_EQ(Tuple::cast(*next).at(1), value);

  next = dictItemIteratorNext(thread_, iter);
  ASSERT_TRUE(next.isError());
}

TEST_F(DictBuiltinsTest, KeyIteratorNextOnOneElementDictReturnsElement) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  Str key(&scope, runtime_.newStrFromCStr("hello"));
  Object value(&scope, runtime_.newStrFromCStr("world"));
  runtime_.dictAtPutByStr(thread_, dict, key, value);
  DictKeyIterator iter(&scope, runtime_.newDictKeyIterator(thread_, dict));
  Object next(&scope, dictKeyIteratorNext(thread_, iter));
  EXPECT_EQ(next, key);

  next = dictKeyIteratorNext(thread_, iter);
  ASSERT_TRUE(next.isError());
}

TEST_F(DictBuiltinsTest, ValueIteratorNextOnOneElementDictReturnsElement) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  Str key(&scope, runtime_.newStrFromCStr("hello"));
  Object value(&scope, runtime_.newStrFromCStr("world"));
  runtime_.dictAtPutByStr(thread_, dict, key, value);
  DictValueIterator iter(&scope, runtime_.newDictValueIterator(thread_, dict));
  Object next(&scope, dictValueIteratorNext(thread_, iter));
  EXPECT_EQ(next, value);

  next = dictValueIteratorNext(thread_, iter);
  ASSERT_TRUE(next.isError());
}

TEST_F(DictBuiltinsTest, NextOnDictWithOnlyTombstonesReturnsFalse) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  Str key(&scope, runtime_.newStrFromCStr("hello"));
  Object value(&scope, runtime_.newStrFromCStr("world"));
  runtime_.dictAtPutByStr(thread_, dict, key, value);
  ASSERT_FALSE(runtime_.dictRemoveByStr(thread_, dict, key).isError());
  Tuple data(&scope, dict.data());
  word i = Dict::Bucket::kFirst;
  ASSERT_FALSE(Dict::Bucket::nextItem(*data, &i));
}

TEST_F(DictBuiltinsTest, RecursiveDictPrintsEllipsis) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  EXPECT_TRUE(
      isStrEqualsCStr(mainModuleAt(&runtime_, "result"), "{'hello': {...}}"));
}

TEST_F(DictBuiltinsTest, PopWithKeyPresentReturnsValue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
d = {"hello": "world"}
result = d.pop("hello")
)")
                   .isError());
  HandleScope scope(thread_);
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(&runtime_, "result"), "world"));
  Dict dict(&scope, mainModuleAt(&runtime_, "d"));
  EXPECT_EQ(dict.numItems(), 0);
  EXPECT_EQ(dict.numUsableItems(), 5 - 1);
}

TEST_F(DictBuiltinsTest, PopWithMissingKeyAndDefaultReturnsDefault) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
d = {}
result = d.pop("hello", "world")
)")
                   .isError());
  HandleScope scope(thread_);
  Dict dict(&scope, mainModuleAt(&runtime_, "d"));
  EXPECT_EQ(dict.numItems(), 0);
  EXPECT_EQ(dict.numUsableItems(), 5);
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(&runtime_, "result"), "world"));
}

TEST_F(DictBuiltinsTest, PopWithMisingKeyRaisesKeyError) {
  EXPECT_TRUE(
      raised(runFromCStr(&runtime_, "{}.pop('hello')"), LayoutId::kKeyError));
}

TEST_F(DictBuiltinsTest, PopWithSubclassDoesNotCallDunderDelItem) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C(dict):
    def __delitem__(self, key):
        raise Exception(key)
c = C({'hello': 'world'})
result = c.pop('hello')
)")
                   .isError());
  ASSERT_FALSE(thread_->hasPendingException());
  HandleScope scope(thread_);
  Dict dict(&scope, mainModuleAt(&runtime_, "c"));
  EXPECT_EQ(dict.numItems(), 0);
  EXPECT_EQ(dict.numUsableItems(), 5 - 1);
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(&runtime_, "result"), "world"));
}

TEST_F(DictBuiltinsTest, DictInitWithSubclassInitializesElements) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C(dict):
    pass
c = C({'hello': 'world'})
)")
                   .isError());
  HandleScope scope(thread_);
  Dict dict(&scope, mainModuleAt(&runtime_, "c"));
  EXPECT_EQ(dict.numItems(), 1);
}

TEST_F(DictBuiltinsTest, SetDefaultWithNoDefaultSetsToNone) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
d = {}
d.setdefault("hello")
result = d["hello"]
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(&runtime_, "result"), NoneType::object());
}

TEST_F(DictBuiltinsTest, SetDefaultWithNotKeyInDictSetsDefault) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
d = {}
d.setdefault("hello", 4)
result = d["hello"]
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(&runtime_, "result"), 4));
}

TEST_F(DictBuiltinsTest, SetDefaultWithKeyInDictReturnsValue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
d = {"hello": 5}
d.setdefault("hello", 4)
result = d["hello"]
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(&runtime_, "result"), 5));
}

TEST_F(DictBuiltinsTest, NextBucketProbesAllBuckets) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  Object key(&scope, runtime_.newInt(123));
  Object key_hash(&scope, Interpreter::hash(thread_, key));
  ASSERT_FALSE(key_hash.isErrorException());
  Object value(&scope, runtime_.newInt(456));
  runtime_.dictAtPut(thread_, dict, key, key_hash, value);

  Tuple data(&scope, dict.data());
  ASSERT_EQ(data.length(),
            Runtime::kInitialDictCapacity * Dict::Bucket::kNumPointers);

  bool probed[Runtime::kInitialDictCapacity] = {false};

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

TEST_F(DictBuiltinsTest, NumAttributesMatchesObjectSize) {
  HandleScope scope(thread_);
  Layout layout(&scope, runtime_.layoutAt(LayoutId::kDict));
  EXPECT_EQ(layout.numInObjectAttributes(),
            (RawDict::kSize - RawHeapObject::kSize) / kPointerSize);
}

}  // namespace py
