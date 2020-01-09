#include "dict-builtins.h"

#include "gtest/gtest.h"

#include "builtins-module.h"
#include "int-builtins.h"
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

TEST_F(DictBuiltinsTest, EmptyDictInvariants) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());

  EXPECT_EQ(dict.numItems(), 0);
  ASSERT_TRUE(dict.data().isTuple());
  EXPECT_EQ(Tuple::cast(dict.data()).length(), 0);
}

TEST_F(DictBuiltinsTest, DictAtPutRetainsExistingKeyObject) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  Str key0(&scope, runtime_->newStrFromCStr("foobarbazbam"));
  word key0_hash = strHash(thread_, *key0);
  Object value0(&scope, SmallInt::fromWord(123));
  Str key1(&scope, runtime_->newStrFromCStr("foobarbazbam"));
  word key1_hash = strHash(thread_, *key1);
  Object value1(&scope, SmallInt::fromWord(456));
  ASSERT_NE(key0, key1);
  ASSERT_EQ(key0_hash, key1_hash);

  dictAtPut(thread_, dict, key0, key0_hash, value0);
  ASSERT_EQ(dict.numItems(), 1);
  ASSERT_EQ(dictAt(thread_, dict, key0, key0_hash), *value0);

  // Overwrite the stored value
  dictAtPut(thread_, dict, key1, key1_hash, value1);
  ASSERT_EQ(dict.numItems(), 1);
  ASSERT_EQ(dictAt(thread_, dict, key1, key1_hash), *value1);

  Tuple data(&scope, dict.data());
  word i = Dict::Bucket::kFirst;
  Dict::Bucket::nextItem(*data, &i);
  EXPECT_EQ(Dict::Bucket::key(*data, i), key0);
}

TEST_F(DictBuiltinsTest, GetSet) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  Object key(&scope, SmallInt::fromWord(12345));
  word hash = intHash(*key);

  // Looking up a key that doesn't exist should fail
  EXPECT_TRUE(dictAt(thread_, dict, key, hash).isError());

  // Store a value
  Object stored(&scope, SmallInt::fromWord(67890));
  dictAtPut(thread_, dict, key, hash, stored);
  EXPECT_EQ(dict.numItems(), 1);

  // Retrieve the stored value
  RawObject retrieved = dictAt(thread_, dict, key, hash);
  EXPECT_EQ(retrieved, *stored);

  // Overwrite the stored value
  Object new_value(&scope, SmallInt::fromWord(5555));
  dictAtPut(thread_, dict, key, hash, new_value);
  EXPECT_EQ(dict.numItems(), 1);

  // Get the new value
  retrieved = dictAt(thread_, dict, key, hash);
  EXPECT_EQ(retrieved, *new_value);
}

TEST_F(DictBuiltinsTest, Remove) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  Object key(&scope, SmallInt::fromWord(12345));
  word hash = intHash(*key);

  // Removing a key that doesn't exist should fail
  bool is_missing = dictRemove(thread_, dict, key, hash).isError();
  EXPECT_TRUE(is_missing);

  // Removing a key that exists should succeed and return the value that was
  // stored.
  Object stored(&scope, SmallInt::fromWord(54321));

  dictAtPut(thread_, dict, key, hash, stored);
  EXPECT_EQ(dict.numItems(), 1);

  RawObject retrieved = dictRemove(thread_, dict, key, hash);
  ASSERT_FALSE(retrieved.isError());
  ASSERT_EQ(SmallInt::cast(retrieved).value(), SmallInt::cast(*stored).value());

  // Looking up a key that was deleted should fail
  EXPECT_TRUE(dictAt(thread_, dict, key, hash).isError());
  EXPECT_EQ(dict.numItems(), 0);
}

TEST_F(DictBuiltinsTest, Length) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());

  // Add 10 items and make sure length reflects it
  for (int i = 0; i < 10; i++) {
    Object key(&scope, SmallInt::fromWord(i));
    word hash = intHash(*key);
    dictAtPut(thread_, dict, key, hash, key);
  }
  EXPECT_EQ(dict.numItems(), 10);

  // Remove half the items
  for (int i = 0; i < 5; i++) {
    Object key(&scope, SmallInt::fromWord(i));
    word hash = intHash(*key);
    ASSERT_FALSE(dictRemove(thread_, dict, key, hash).isError());
  }
  EXPECT_EQ(dict.numItems(), 5);
}

TEST_F(DictBuiltinsTest, AtIfAbsentPutLength) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());

  Object k1(&scope, SmallInt::fromWord(1));
  word k1_hash = intHash(*k1);
  Object v1(&scope, SmallInt::fromWord(111));
  dictAtPut(thread_, dict, k1, k1_hash, v1);
  EXPECT_EQ(dict.numItems(), 1);

  class SmallIntCallback : public Callback<RawObject> {
   public:
    explicit SmallIntCallback(int i) : i_(i) {}
    RawObject call() override { return SmallInt::fromWord(i_); }

   private:
    int i_;
  };

  // Add new item
  Object k2(&scope, SmallInt::fromWord(2));
  word k2_hash = intHash(*k2);
  SmallIntCallback cb(222);
  dictAtIfAbsentPut(thread_, dict, k2, k2_hash, &cb);
  EXPECT_EQ(dict.numItems(), 2);
  RawObject retrieved = dictAt(thread_, dict, k2, k2_hash);
  EXPECT_TRUE(isIntEqualsWord(retrieved, 222));

  // Don't overrwite existing item 1 -> v1
  Object k3(&scope, SmallInt::fromWord(1));
  word k3_hash = intHash(*k3);
  SmallIntCallback cb3(333);
  dictAtIfAbsentPut(thread_, dict, k3, k3_hash, &cb3);
  EXPECT_EQ(dict.numItems(), 2);
  retrieved = dictAt(thread_, dict, k3, k3_hash);
  EXPECT_EQ(retrieved, *v1);
}

TEST_F(DictBuiltinsTest, DictAtPutGrowsDictWhenDictIsEmpty) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  EXPECT_EQ(dict.capacity(), 0);

  Object first_key(&scope, SmallInt::fromWord(0));
  word hash = intHash(*first_key);
  Object first_value(&scope, SmallInt::fromWord(1));
  dictAtPut(thread_, dict, first_key, hash, first_value);

  word initial_capacity = Runtime::kInitialDictCapacity;
  EXPECT_EQ(dict.numItems(), 1);
  EXPECT_EQ(dict.capacity(), initial_capacity);
}

TEST_F(DictBuiltinsTest, DictAtPutGrowsDictWhenTwoThirdsUsed) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());

  // Fill in one fewer keys than would require growing the underlying object
  // array again.
  word threshold = ((Runtime::kInitialDictCapacity * 2) / 3) - 1;
  for (word i = 0; i < threshold; i++) {
    Object key(&scope, SmallInt::fromWord(i));
    word hash = intHash(*key);
    Object value(&scope, SmallInt::fromWord(-i));
    dictAtPut(thread_, dict, key, hash, value);
  }
  EXPECT_EQ(dict.numItems(), threshold);
  EXPECT_EQ(dict.numUsableItems(), 1);
  word initial_capacity = Runtime::kInitialDictCapacity;
  EXPECT_EQ(dict.capacity(), initial_capacity);

  // Add another key which should force us to double the capacity
  Object last_key(&scope, SmallInt::fromWord(threshold));
  word last_key_hash = intHash(*last_key);
  Object last_value(&scope, SmallInt::fromWord(-threshold));
  dictAtPut(thread_, dict, last_key, last_key_hash, last_value);
  EXPECT_EQ(dict.numItems(), threshold + 1);
  EXPECT_EQ(dict.capacity(), initial_capacity * Runtime::kDictGrowthFactor);
  EXPECT_EQ(dict.numUsableItems(),
            ((dict.capacity() * 2) / 3) - dict.numItems());

  // Make sure we can still read all the stored keys/values.
  for (word i = 0; i <= threshold; i++) {
    Object key(&scope, SmallInt::fromWord(i));
    word hash = intHash(*key);
    RawObject value = dictAt(thread_, dict, key, hash);
    ASSERT_FALSE(value.isError());
    EXPECT_TRUE(isIntEqualsWord(value, -i));
  }
}

TEST_F(DictBuiltinsTest, CollidingKeys) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __eq__(self, other):
    return self is other
  def __hash__(self):
    return 0
i0 = C()
i1 = C()
)")
                   .isError());
  Object i0(&scope, mainModuleAt(runtime_, "i0"));
  Object i0_hash_obj(&scope, Interpreter::hash(thread_, i0));
  ASSERT_FALSE(i0_hash_obj.isErrorException());
  word i0_hash = SmallInt::cast(*i0_hash_obj).value();
  Object i1(&scope, mainModuleAt(runtime_, "i1"));
  Object i1_hash_obj(&scope, Interpreter::hash(thread_, i1));
  ASSERT_FALSE(i1_hash_obj.isErrorException());
  word i1_hash = SmallInt::cast(*i1_hash_obj).value();
  ASSERT_EQ(i0_hash, i1_hash);

  Dict dict(&scope, runtime_->newDict());

  // Add two different keys with different values using the same hash
  dictAtPut(thread_, dict, i0, i0_hash, i0);
  dictAtPut(thread_, dict, i1, i1_hash, i1);

  // Make sure we get both back
  Object retrieved(&scope, dictAt(thread_, dict, i0, i0_hash));
  EXPECT_EQ(retrieved, i0);

  retrieved = dictAt(thread_, dict, i1, i1_hash);
  EXPECT_EQ(retrieved, i1);
}

TEST_F(DictBuiltinsTest, MixedKeys) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());

  // Add keys of different type
  Object int_key(&scope, SmallInt::fromWord(100));
  word int_key_hash = intHash(*int_key);
  dictAtPut(thread_, dict, int_key, int_key_hash, int_key);

  Object str_key(&scope, runtime_->newStrFromCStr("testing 123"));
  word str_key_hash = strHash(thread_, *str_key);
  dictAtPut(thread_, dict, str_key, str_key_hash, str_key);

  // Make sure we get the appropriate values back out
  RawObject retrieved = dictAt(thread_, dict, int_key, int_key_hash);
  EXPECT_EQ(retrieved, *int_key);

  retrieved = dictAt(thread_, dict, str_key, str_key_hash);
  ASSERT_TRUE(retrieved.isStr());
  EXPECT_EQ(*str_key, retrieved);
}

TEST_F(DictBuiltinsTest, GetKeys) {
  HandleScope scope(thread_);

  // Create keys
  Tuple keys(&scope, runtime_->newTuple(4));
  keys.atPut(0, SmallInt::fromWord(100));
  keys.atPut(1, runtime_->newStrFromCStr("testing 123"));
  keys.atPut(2, Bool::trueObj());
  keys.atPut(3, NoneType::object());

  // Add keys to dict
  Dict dict(&scope, runtime_->newDict());
  for (word i = 0; i < keys.length(); i++) {
    Object key(&scope, keys.at(i));
    Object hash_obj(&scope, Interpreter::hash(thread_, key));
    ASSERT_FALSE(hash_obj.isErrorException());
    word hash = SmallInt::cast(*hash_obj).value();
    dictAtPut(thread_, dict, key, hash, key);
  }

  // Grab the keys and verify everything is there
  List retrieved(&scope, dictKeys(thread_, dict));
  ASSERT_EQ(retrieved.numItems(), keys.length());
  for (word i = 0; i < keys.length(); i++) {
    Object key(&scope, keys.at(i));
    EXPECT_TRUE(listContains(retrieved, key)) << " missing key " << i;
  }
}

TEST_F(DictBuiltinsTest, CanCreateDictItems) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  RawObject iter = runtime_->newDictItemIterator(thread_, dict);
  ASSERT_TRUE(iter.isDictItemIterator());
}

TEST_F(DictBuiltinsTest, DictAtGrowsToInitialCapacity) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  EXPECT_EQ(dict.capacity(), 0);

  Object key(&scope, runtime_->newInt(123));
  word hash = intHash(*key);
  Object value(&scope, runtime_->newInt(456));
  dictAtPut(thread_, dict, key, hash, value);
  int expected = Runtime::kInitialDictCapacity;
  EXPECT_EQ(dict.capacity(), expected);
}

TEST_F(DictBuiltinsTest, ClearWithEmptyDictIsNoop) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  EXPECT_EQ(runBuiltin(DictBuiltins::clear, dict), NoneType::object());
}

TEST_F(DictBuiltinsTest, ClearWithNonEmptyDictRemovesAllElements) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  pass
d = {'a': C()}
)")
                   .isError());

  HandleScope scope(thread_);
  Dict dict(&scope, mainModuleAt(runtime_, "d"));
  Object ref_obj(&scope, NoneType::object());
  {
    Object none(&scope, NoneType::object());
    Str key(&scope, runtime_->newStrFromCStr("a"));
    Object c(&scope, dictAtByStr(thread_, dict, key));
    ref_obj = runtime_->newWeakRef(thread_, c, none);
  }
  WeakRef ref(&scope, *ref_obj);
  EXPECT_NE(ref.referent(), NoneType::object());
  runBuiltin(DictBuiltins::clear, dict);
  runtime_->collectGarbage();
  EXPECT_EQ(ref.referent(), NoneType::object());
}

TEST_F(DictBuiltinsTest, CopyWithDictReturnsNewInstance) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
d = {'a': 3}
result = dict.copy(d)
)")
                   .isError());
  HandleScope scope(thread_);
  Object dict(&scope, mainModuleAt(runtime_, "d"));
  EXPECT_TRUE(dict.isDict());
  Object result_obj(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(result_obj.isDict());
  Dict result(&scope, *result_obj);
  EXPECT_NE(*dict, *result);
  EXPECT_EQ(result.numItems(), 1);
  EXPECT_EQ(result.numUsableItems(), 5 - 1);
}

TEST_F(DictBuiltinsTest, DunderContainsWithExistingKeyReturnsTrue) {
  ASSERT_FALSE(runFromCStr(runtime_, "result = {'foo': 0}.__contains__('foo')")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result.isBool());
  EXPECT_TRUE(Bool::cast(*result).value());
}

TEST_F(DictBuiltinsTest, DunderContainsWithNonexistentKeyReturnsFalse) {
  ASSERT_FALSE(
      runFromCStr(runtime_, "result = {}.__contains__('foo')").isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result.isBool());
  EXPECT_FALSE(Bool::cast(*result).value());
}

TEST_F(DictBuiltinsTest, DunderContainsWithUnhashableTypeRaisesTypeError) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  __hash__ = None
c = C()
)")
                   .isError());
  EXPECT_TRUE(raised(runFromCStr(runtime_, "{}.__contains__(C())"),
                     LayoutId::kTypeError));
}

TEST_F(DictBuiltinsTest,
       DunderContainsWithNonCallableDunderHashRaisesTypeError) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  __hash__ = 4
)")
                   .isError());
  EXPECT_TRUE(raised(runFromCStr(runtime_, "{}.__contains__(C())"),
                     LayoutId::kTypeError));
}

TEST_F(DictBuiltinsTest,
       DunderContainsWithTypeWithDunderHashReturningNonIntRaisesTypeError) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __hash__(self):
    return "boo"
)")
                   .isError());
  EXPECT_TRUE(raised(runFromCStr(runtime_, "{}.__contains__(C())"),
                     LayoutId::kTypeError));
}

TEST_F(DictBuiltinsTest, InWithExistingKeyReturnsTrue) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
d = {"foo": 1}
foo_in_d = "foo" in d
)")
                   .isError());
  HandleScope scope(thread_);
  Bool foo_in_d(&scope, mainModuleAt(runtime_, "foo_in_d"));

  EXPECT_TRUE(foo_in_d.value());
}

TEST_F(DictBuiltinsTest, InWithNonexistentKeyReturnsFalse) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
d = {}
foo_in_d = "foo" in d
)")
                   .isError());
  HandleScope scope(thread_);
  Bool foo_in_d(&scope, mainModuleAt(runtime_, "foo_in_d"));

  EXPECT_FALSE(foo_in_d.value());
}

TEST_F(DictBuiltinsTest, DunderDelitemOnExistingKeyReturnsNone) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDictWithSize(1));
  Str key(&scope, runtime_->newStrFromCStr("foo"));
  Object val(&scope, runtime_->newInt(0));
  dictAtPutByStr(thread_, dict, key, val);
  RawObject result = runBuiltin(DictBuiltins::dunderDelitem, dict, key);
  EXPECT_TRUE(result.isNoneType());
}

TEST_F(DictBuiltinsTest, DunderDelitemOnNonexistentKeyRaisesKeyError) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDictWithSize(1));
  Str key(&scope, runtime_->newStrFromCStr("foo"));
  Object val(&scope, runtime_->newInt(0));
  dictAtPutByStr(thread_, dict, key, val);

  // "bar" doesn't exist in this dictionary, attempting to delete it should
  // cause a KeyError.
  Object key2(&scope, runtime_->newStrFromCStr("bar"));
  RawObject result = runBuiltin(DictBuiltins::dunderDelitem, dict, key2);
  ASSERT_TRUE(result.isError());
}

TEST_F(DictBuiltinsTest, DelOnObjectHashReturningNonIntRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class E:
  def __hash__(self): return "non int"

d = {}
del d[E()]
)"),
                            LayoutId::kTypeError,
                            "__hash__ method should return an integer"));
}

TEST_F(DictBuiltinsTest, DelOnExistingKeyDeletesKey) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
d = {"foo": 1}
del d["foo"]
)")
                   .isError());
  HandleScope scope(thread_);
  Dict d(&scope, mainModuleAt(runtime_, "d"));
  Str foo(&scope, runtime_->newStrFromCStr("foo"));

  EXPECT_FALSE(dictIncludesByStr(thread_, d, foo));
}

TEST_F(DictBuiltinsTest, DelOnNonexistentKeyRaisesKeyError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
d = {}
del d["foo"]
)"),
                            LayoutId::kKeyError, "foo"));
}

TEST_F(DictBuiltinsTest, NonTypeInDunderNew) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
dict.__new__(1)
)"),
                            LayoutId::kTypeError, "not a type object"));
}

TEST_F(DictBuiltinsTest, NonSubclassInDunderNew) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class Foo: pass
dict.__new__(Foo)
)"),
                            LayoutId::kTypeError, "not a subtype of dict"));
}

TEST_F(DictBuiltinsTest, DunderNewConstructsDict) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_->typeAt(LayoutId::kDict));
  Object result(&scope, runBuiltin(DictBuiltins::dunderNew, type));
  ASSERT_TRUE(result.isDict());
}

TEST_F(DictBuiltinsTest, DunderIterReturnsDictKeyIter) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  Object iter(&scope, runBuiltin(DictBuiltins::dunderIter, dict));
  ASSERT_TRUE(iter.isDictKeyIterator());
}

TEST_F(DictBuiltinsTest, DunderItemsReturnsDictItems) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  Object items(&scope, runBuiltin(DictBuiltins::items, dict));
  ASSERT_TRUE(items.isDictItems());
}

TEST_F(DictBuiltinsTest, KeysReturnsDictKeys) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  Object keys(&scope, runBuiltin(DictBuiltins::keys, dict));
  ASSERT_TRUE(keys.isDictKeys());
}

TEST_F(DictBuiltinsTest, ValuesReturnsDictValues) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  Object values(&scope, runBuiltin(DictBuiltins::values, dict));
  ASSERT_TRUE(values.isDictValues());
}

TEST_F(DictBuiltinsTest, UpdateWithNoArgumentsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "dict.update()"), LayoutId::kTypeError,
      "'dict.update' takes min 1 positional arguments but 0 given"));
}

TEST_F(DictBuiltinsTest, UpdateWithNonDictRaisesTypeError) {
  EXPECT_TRUE(raised(runFromCStr(runtime_, "dict.update([], None)"),
                     LayoutId::kTypeError));
}

TEST_F(DictBuiltinsTest, UpdateWithNonMappingTypeRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "dict.update({}, 1)"),
                            LayoutId::kTypeError,
                            "'int' object is not iterable"));
}

TEST_F(DictBuiltinsTest,
       UpdateWithListContainerWithObjectHashReturningNonIntRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
  ASSERT_FALSE(runFromCStr(runtime_, R"(
d1 = {"a": 1, "b": 2}
d2 = {"c": 3, "d": 4}
d3 = {"a": 123}
)")
                   .isError());
  HandleScope scope(thread_);
  Dict d1(&scope, mainModuleAt(runtime_, "d1"));
  Dict d2(&scope, mainModuleAt(runtime_, "d2"));
  ASSERT_EQ(d1.numItems(), 2);
  ASSERT_EQ(d1.numUsableItems(), 5 - 2);
  ASSERT_EQ(d2.numItems(), 2);
  ASSERT_EQ(d2.numUsableItems(), 5 - 2);
  ASSERT_TRUE(runFromCStr(runtime_, "d1.update(d2)").isNoneType());
  EXPECT_EQ(d1.numItems(), 4);
  ASSERT_EQ(d1.numUsableItems(), 5 - 4);
  EXPECT_EQ(d2.numItems(), 2);
  ASSERT_EQ(d2.numUsableItems(), 5 - 2);

  ASSERT_TRUE(runFromCStr(runtime_, "d1.update(d3)").isNoneType());
  EXPECT_EQ(d1.numItems(), 4);
  ASSERT_EQ(d1.numUsableItems(), 5 - 4);
  Str a(&scope, runtime_->newStrFromCStr("a"));
  Object a_val(&scope, dictAtByStr(thread_, d1, a));
  EXPECT_TRUE(isIntEqualsWord(*a_val, 123));
}

TEST_F(DictItemsBuiltinsTest, DunderIterReturnsIter) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  DictItems items(&scope, runtime_->newDictItems(thread_, dict));
  Object iter(&scope, runBuiltin(DictItemsBuiltins::dunderIter, items));
  ASSERT_TRUE(iter.isDictItemIterator());
}

TEST_F(DictKeysBuiltinsTest, DunderIterReturnsIter) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  DictKeys keys(&scope, runtime_->newDictKeys(thread_, dict));
  Object iter(&scope, runBuiltin(DictKeysBuiltins::dunderIter, keys));
  ASSERT_TRUE(iter.isDictKeyIterator());
}

TEST_F(DictValuesBuiltinsTest, DunderIterReturnsIter) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  DictValues values(&scope, runtime_->newDictValues(thread_, dict));
  Object iter(&scope, runBuiltin(DictValuesBuiltins::dunderIter, values));
  ASSERT_TRUE(iter.isDictValueIterator());
}

TEST_F(DictItemIteratorBuiltinsTest, CallDunderIterReturnsSelf) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  DictItemIterator iter(&scope, runtime_->newDictItemIterator(thread_, dict));
  // Now call __iter__ on the iterator object
  Object result(&scope, runBuiltin(DictItemIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST_F(DictKeyIteratorBuiltinsTest, CallDunderIterReturnsSelf) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  DictKeyIterator iter(&scope, runtime_->newDictKeyIterator(thread_, dict));
  // Now call __iter__ on the iterator object
  Object result(&scope, runBuiltin(DictKeyIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST_F(DictValueIteratorBuiltinsTest, CallDunderIterReturnsSelf) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  DictValueIterator iter(&scope, runtime_->newDictValueIterator(thread_, dict));
  // Now call __iter__ on the iterator object
  Object result(&scope,
                runBuiltin(DictValueIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST_F(DictItemIteratorBuiltinsTest,
       DunderLengthHintOnEmptyDictItemIteratorReturnsZero) {
  HandleScope scope(thread_);
  Dict empty_dict(&scope, runtime_->newDict());
  DictItemIterator iter(&scope,
                        runtime_->newDictItemIterator(thread_, empty_dict));
  Object length_hint(
      &scope, runBuiltin(DictItemIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST_F(DictKeyIteratorBuiltinsTest,
       DunderLengthHintOnEmptyDictKeyIteratorReturnsZero) {
  HandleScope scope(thread_);
  Dict empty_dict(&scope, runtime_->newDict());
  DictKeyIterator iter(&scope,
                       runtime_->newDictKeyIterator(thread_, empty_dict));
  Object length_hint(
      &scope, runBuiltin(DictKeyIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST_F(DictValueIteratorBuiltinsTest,
       DunderLengthHintOnEmptyDictValueIteratorReturnsZero) {
  HandleScope scope(thread_);
  Dict empty_dict(&scope, runtime_->newDict());
  DictValueIterator iter(&scope,
                         runtime_->newDictValueIterator(thread_, empty_dict));
  Object length_hint(
      &scope, runBuiltin(DictValueIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST_F(DictItemIteratorBuiltinsTest, CallDunderNextReadsItemsSequentially) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDictWithSize(5));
  Str hello(&scope, runtime_->newStrFromCStr("hello"));
  Object world(&scope, runtime_->newStrFromCStr("world"));
  Str goodbye(&scope, runtime_->newStrFromCStr("goodbye"));
  Object moon(&scope, runtime_->newStrFromCStr("moon"));
  dictAtPutByStr(thread_, dict, hello, world);
  dictAtPutByStr(thread_, dict, goodbye, moon);
  DictItemIterator iter(&scope, runtime_->newDictItemIterator(thread_, dict));

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
  Dict dict(&scope, runtime_->newDictWithSize(5));
  Str hello(&scope, runtime_->newStrFromCStr("hello"));
  Object world(&scope, runtime_->newStrFromCStr("world"));
  Str goodbye(&scope, runtime_->newStrFromCStr("goodbye"));
  Object moon(&scope, runtime_->newStrFromCStr("moon"));
  dictAtPutByStr(thread_, dict, hello, world);
  dictAtPutByStr(thread_, dict, goodbye, moon);
  DictKeyIterator iter(&scope, runtime_->newDictKeyIterator(thread_, dict));

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
  Dict dict(&scope, runtime_->newDictWithSize(5));
  Str hello(&scope, runtime_->newStrFromCStr("hello"));
  Object world(&scope, runtime_->newStrFromCStr("world"));
  Str goodbye(&scope, runtime_->newStrFromCStr("goodbye"));
  Object moon(&scope, runtime_->newStrFromCStr("moon"));
  dictAtPutByStr(thread_, dict, hello, world);
  dictAtPutByStr(thread_, dict, goodbye, moon);
  DictValueIterator iter(&scope, runtime_->newDictValueIterator(thread_, dict));

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
  Dict dict(&scope, runtime_->newDict());
  Str hello(&scope, runtime_->newStrFromCStr("hello"));
  Object world(&scope, runtime_->newStrFromCStr("world"));
  dictAtPutByStr(thread_, dict, hello, world);
  DictItemIterator iter(&scope, runtime_->newDictItemIterator(thread_, dict));

  Object item1(&scope, runBuiltin(DictItemIteratorBuiltins::dunderNext, iter));
  ASSERT_FALSE(item1.isError());

  Object length_hint(
      &scope, runBuiltin(DictItemIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST_F(DictKeyIteratorBuiltinsTest,
       DunderLengthHintOnConsumedDictKeyIteratorReturnsZero) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  Str hello(&scope, runtime_->newStrFromCStr("hello"));
  Object world(&scope, runtime_->newStrFromCStr("world"));
  dictAtPutByStr(thread_, dict, hello, world);
  DictKeyIterator iter(&scope, runtime_->newDictKeyIterator(thread_, dict));

  Object item1(&scope, runBuiltin(DictKeyIteratorBuiltins::dunderNext, iter));
  ASSERT_FALSE(item1.isError());

  Object length_hint(
      &scope, runBuiltin(DictKeyIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST_F(DictValueIteratorBuiltinsTest,
       DunderLengthHintOnConsumedDictValueIteratorReturnsZero) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  Str hello(&scope, runtime_->newStrFromCStr("hello"));
  Object world(&scope, runtime_->newStrFromCStr("world"));
  dictAtPutByStr(thread_, dict, hello, world);
  DictValueIterator iter(&scope, runtime_->newDictValueIterator(thread_, dict));

  Object item1(&scope, runBuiltin(DictValueIteratorBuiltins::dunderNext, iter));
  ASSERT_FALSE(item1.isError());

  Object length_hint(
      &scope, runBuiltin(DictValueIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST_F(DictBuiltinsTest, ItemIteratorNextOnOneElementDictReturnsElement) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  Str key(&scope, runtime_->newStrFromCStr("hello"));
  Object value(&scope, runtime_->newStrFromCStr("world"));
  dictAtPutByStr(thread_, dict, key, value);
  DictItemIterator iter(&scope, runtime_->newDictItemIterator(thread_, dict));
  Object next(&scope, dictItemIteratorNext(thread_, iter));
  ASSERT_TRUE(next.isTuple());
  EXPECT_EQ(Tuple::cast(*next).at(0), key);
  EXPECT_EQ(Tuple::cast(*next).at(1), value);

  next = dictItemIteratorNext(thread_, iter);
  ASSERT_TRUE(next.isError());
}

TEST_F(DictBuiltinsTest, KeyIteratorNextOnOneElementDictReturnsElement) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  Str key(&scope, runtime_->newStrFromCStr("hello"));
  Object value(&scope, runtime_->newStrFromCStr("world"));
  dictAtPutByStr(thread_, dict, key, value);
  DictKeyIterator iter(&scope, runtime_->newDictKeyIterator(thread_, dict));
  Object next(&scope, dictKeyIteratorNext(thread_, iter));
  EXPECT_EQ(next, key);

  next = dictKeyIteratorNext(thread_, iter);
  ASSERT_TRUE(next.isError());
}

TEST_F(DictBuiltinsTest, ValueIteratorNextOnOneElementDictReturnsElement) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  Str key(&scope, runtime_->newStrFromCStr("hello"));
  Object value(&scope, runtime_->newStrFromCStr("world"));
  dictAtPutByStr(thread_, dict, key, value);
  DictValueIterator iter(&scope, runtime_->newDictValueIterator(thread_, dict));
  Object next(&scope, dictValueIteratorNext(thread_, iter));
  EXPECT_EQ(next, value);

  next = dictValueIteratorNext(thread_, iter);
  ASSERT_TRUE(next.isError());
}

TEST_F(DictBuiltinsTest, NextOnDictWithOnlyTombstonesReturnsFalse) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  Str key(&scope, runtime_->newStrFromCStr("hello"));
  Object value(&scope, runtime_->newStrFromCStr("world"));
  dictAtPutByStr(thread_, dict, key, value);
  ASSERT_FALSE(dictRemoveByStr(thread_, dict, key).isError());
  Tuple data(&scope, dict.data());
  word i = Dict::Bucket::kFirst;
  ASSERT_FALSE(Dict::Bucket::nextItem(*data, &i));
}

TEST_F(DictBuiltinsTest, RecursiveDictPrintsEllipsis) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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
      isStrEqualsCStr(mainModuleAt(runtime_, "result"), "{'hello': {...}}"));
}

TEST_F(DictBuiltinsTest, PopWithKeyPresentReturnsValue) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
d = {"hello": "world"}
result = d.pop("hello")
)")
                   .isError());
  HandleScope scope(thread_);
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "result"), "world"));
  Dict dict(&scope, mainModuleAt(runtime_, "d"));
  EXPECT_EQ(dict.numItems(), 0);
  EXPECT_EQ(dict.numUsableItems(), 5 - 1);
}

TEST_F(DictBuiltinsTest, PopWithMissingKeyAndDefaultReturnsDefault) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
d = {}
result = d.pop("hello", "world")
)")
                   .isError());
  HandleScope scope(thread_);
  Dict dict(&scope, mainModuleAt(runtime_, "d"));
  EXPECT_EQ(dict.numItems(), 0);
  EXPECT_EQ(dict.numUsableItems(), 5);
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "result"), "world"));
}

TEST_F(DictBuiltinsTest, PopWithMisingKeyRaisesKeyError) {
  EXPECT_TRUE(
      raised(runFromCStr(runtime_, "{}.pop('hello')"), LayoutId::kKeyError));
}

TEST_F(DictBuiltinsTest, PopWithSubclassDoesNotCallDunderDelitem) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C(dict):
    def __delitem__(self, key):
        raise Exception(key)
c = C({'hello': 'world'})
result = c.pop('hello')
)")
                   .isError());
  ASSERT_FALSE(thread_->hasPendingException());
  HandleScope scope(thread_);
  Dict dict(&scope, mainModuleAt(runtime_, "c"));
  EXPECT_EQ(dict.numItems(), 0);
  EXPECT_EQ(dict.numUsableItems(), 5 - 1);
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "result"), "world"));
}

TEST_F(DictBuiltinsTest, DictInitWithSubclassInitializesElements) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C(dict):
    pass
c = C({'hello': 'world'})
)")
                   .isError());
  HandleScope scope(thread_);
  Dict dict(&scope, mainModuleAt(runtime_, "c"));
  EXPECT_EQ(dict.numItems(), 1);
}

TEST_F(DictBuiltinsTest, SetDefaultWithNoDefaultSetsToNone) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
d = {}
d.setdefault("hello")
result = d["hello"]
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(runtime_, "result"), NoneType::object());
}

TEST_F(DictBuiltinsTest, SetDefaultWithNotKeyInDictSetsDefault) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
d = {}
d.setdefault("hello", 4)
result = d["hello"]
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 4));
}

TEST_F(DictBuiltinsTest, SetDefaultWithKeyInDictReturnsValue) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
d = {"hello": 5}
d.setdefault("hello", 4)
result = d["hello"]
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 5));
}

TEST_F(DictBuiltinsTest, NextBucketProbesAllBuckets) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  Object key(&scope, runtime_->newInt(123));
  word hash = intHash(*key);
  Object value(&scope, runtime_->newInt(456));
  dictAtPut(thread_, dict, key, hash, value);

  Tuple data(&scope, dict.data());
  ASSERT_EQ(data.length(),
            Runtime::kInitialDictCapacity * Dict::Bucket::kNumPointers);

  bool probed[Runtime::kInitialDictCapacity] = {false};

  uword perturb;
  word bucket_mask;
  word current = Dict::Bucket::bucket(*data, hash, &bucket_mask, &perturb);
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
  Layout layout(&scope, runtime_->layoutAt(LayoutId::kDict));
  EXPECT_EQ(layout.numInObjectAttributes(),
            (RawDict::kSize - RawHeapObject::kSize) / kPointerSize);
}

}  // namespace py
