#include "gtest/gtest.h"

#include <cstdlib>
#include <memory>

#include "bytecode.h"
#include "frame.h"
#include "layout.h"
#include "object-builtins.h"
#include "runtime.h"
#include "symbols.h"
#include "test-utils.h"
#include "trampolines.h"

namespace python {
using namespace testing;

RawObject makeTestFunction() {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object name(&scope, runtime->newStrFromCStr("foo"));
  Code code(&scope, runtime->newEmptyCode(name));
  const byte bytecode[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  code.setCode(runtime->newBytesWithAll(bytecode));
  Tuple consts(&scope, runtime->newTuple(1));
  consts.atPut(0, NoneType::object());
  code.setConsts(*consts);
  Object qualname(&scope, runtime->newStrFromCStr("foo"));
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime->newDict());
  Dict builtins(&scope, runtime->newDict());
  return Interpreter::makeFunction(thread, qualname, code, none, none, none,
                                   none, globals, builtins);
}

TEST(RuntimeTest, CollectGarbage) {
  Runtime runtime;
  ASSERT_TRUE(runtime.heap()->verify());
  runtime.collectGarbage();
  ASSERT_TRUE(runtime.heap()->verify());
}

TEST(RuntimeTest, AllocateAndCollectGarbage) {
  const word heap_size = 32 * kMiB;
  const word array_length = 1024;
  const word allocation_size = LargeBytes::allocationSize(array_length);
  const word total_allocation_size = heap_size * 10;
  Runtime runtime(heap_size);
  ASSERT_TRUE(runtime.heap()->verify());
  for (word i = 0; i < total_allocation_size; i += allocation_size) {
    runtime.newBytes(array_length, 0);
  }
  ASSERT_TRUE(runtime.heap()->verify());
}

TEST(RuntimeTest, AttributeAtCallsDunderGetattribute) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  foo = None
  def __getattribute__(self, name):
    return (self, name)
c = C()
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  Object name(&scope, runtime.newStrFromCStr("foo"));
  Object result_obj(&scope, runtime.attributeAt(thread, c, name));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), c);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "foo"));
}

TEST(RuntimeTest, AttributeAtPropagatesExceptionFromDunderGetAttribute) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __getattribute__(self, name):
    raise UserWarning()
c = C()
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  Object name(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(
      raised(runtime.attributeAt(thread, c, name), LayoutId::kUserWarning));
}

TEST(RuntimeTest, AttributeAtCallsDunderGetattr) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  foo = 10
  def __getattr__(self, name):
    return (self, name)
c = C()
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  Object foo(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(runtime.attributeAt(thread, c, foo), 10));
  Object bar(&scope, runtime.newStrFromCStr("bar"));
  Object result_obj(&scope, runtime.attributeAt(thread, c, bar));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), c);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "bar"));
}

TEST(RuntimeTest, AttributeAtDoesNotCallDunderGetattrOnNonAttributeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __getattribute__(self, name):
    raise UserWarning()
  def __getattr__(self, name):
    _unimplemented()
c = C()
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  Object foo(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(
      raised(runtime.attributeAt(thread, c, foo), LayoutId::kUserWarning));
}

// Return the raw name of a builtin LayoutId, or "<invalid>" for user-defined or
// invalid LayoutIds.
static const char* layoutIdName(LayoutId id) {
  switch (id) {
    case LayoutId::kError:
      // Special-case the one type that isn't really a class so we don't have to
      // have it in CLASS_NAMES.
      return "RawError";

#define CASE(name)                                                             \
  case LayoutId::k##name:                                                      \
    return #name;
      CLASS_NAMES(CASE)
#undef CASE
    case LayoutId::kSentinelId:
      return "<SentinelId>";
  }
  return "<invalid>";
}

class BuiltinTypeIdsTest : public ::testing::TestWithParam<LayoutId> {};

// Make sure that each built-in class has a class object.  Check that its class
// object points to a layout with the same layout ID as the built-in class.
TEST_P(BuiltinTypeIdsTest, HasTypeObject) {
  Runtime runtime;
  HandleScope scope;

  LayoutId id = GetParam();
  ASSERT_EQ(runtime.layoutAt(id).layoutId(), LayoutId::kLayout)
      << "Bad RawLayout for " << layoutIdName(id);
  Object elt(&scope, runtime.typeAt(id));
  ASSERT_TRUE(elt.isType());
  Type cls(&scope, *elt);
  Layout layout(&scope, cls.instanceLayout());
  EXPECT_EQ(layout.id(), GetParam());
}

static const LayoutId kBuiltinHeapTypeIds[] = {
#define ENUM(x) LayoutId::k##x,
    HEAP_CLASS_NAMES(ENUM)
#undef ENUM
};

INSTANTIATE_TEST_CASE_P(BuiltinTypeIdsParameters, BuiltinTypeIdsTest,
                        ::testing::ValuesIn(kBuiltinHeapTypeIds), );

TEST(RuntimeByteArrayTest, EnsureCapacity) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  ByteArray array(&scope, runtime.newByteArray());
  word length = 1;
  word expected_capacity = 16;
  runtime.byteArrayEnsureCapacity(thread, array, length);
  EXPECT_EQ(array.capacity(), expected_capacity);

  length = 17;
  expected_capacity = 24;
  runtime.byteArrayEnsureCapacity(thread, array, length);
  EXPECT_EQ(array.capacity(), expected_capacity);

  length = 40;
  expected_capacity = 40;
  runtime.byteArrayEnsureCapacity(thread, array, length);
  EXPECT_EQ(array.capacity(), expected_capacity);
}

TEST(RuntimeByteArrayTest, Extend) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  ByteArray array(&scope, runtime.newByteArray());
  View<byte> hello(reinterpret_cast<const byte*>("Hello world!"), 5);
  runtime.byteArrayExtend(thread, array, hello);
  EXPECT_GE(array.capacity(), 5);
  EXPECT_EQ(array.numItems(), 5);

  Bytes bytes(&scope, array.bytes());
  bytes = runtime.bytesSubseq(thread, bytes, 0, 5);
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "Hello"));
}

TEST(RuntimeBytesTest, Concat) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  View<byte> foo(reinterpret_cast<const byte*>("foo"), 3);
  Bytes self(&scope, runtime.newBytesWithAll(foo));
  View<byte> bar(reinterpret_cast<const byte*>("bar"), 3);
  Bytes other(&scope, runtime.newBytesWithAll(bar));
  Bytes result(&scope, runtime.bytesConcat(thread, self, other));
  EXPECT_TRUE(isBytesEqualsCStr(result, "foobar"));
}

TEST(RuntimeBytesTest, FromTupleWithSizeReturnsBytesMatchingSize) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple(&scope, runtime.newTuple(3));
  tuple.atPut(0, SmallInt::fromWord(42));
  tuple.atPut(1, SmallInt::fromWord(123));
  Object result(&scope, runtime.bytesFromTuple(thread, tuple, 2));
  const byte bytes[] = {42, 123};
  EXPECT_TRUE(isBytesEqualsBytes(result, bytes));
}

TEST(RuntimeBytesTest, FromTupleWithNonIndexReturnsNone) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple(&scope, runtime.newTuple(1));
  tuple.atPut(0, runtime.newFloat(1));
  EXPECT_EQ(runtime.bytesFromTuple(thread, tuple, 1), NoneType::object());
}

TEST(RuntimeBytesTest, FromTupleWithNegativeIntRaisesValueError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple(&scope, runtime.newTuple(1));
  tuple.atPut(0, SmallInt::fromWord(-1));
  Object result(&scope, runtime.bytesFromTuple(thread, tuple, 1));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kValueError,
                            "bytes must be in range(0, 256)"));
}

TEST(RuntimeBytesTest, FromTupleWithBigIntRaisesValueError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple(&scope, runtime.newTuple(1));
  tuple.atPut(0, SmallInt::fromWord(256));
  Object result(&scope, runtime.bytesFromTuple(thread, tuple, 1));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kValueError,
                            "bytes must be in range(0, 256)"));
}

TEST(RuntimeBytesTest, FromTupleWithIntSubclassReturnsBytes) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C(int): pass
a = C(97)
b = C(98)
c = C(99)
)")
                   .isError());
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple(&scope, runtime.newTuple(3));
  tuple.atPut(0, moduleAt(&runtime, "__main__", "a"));
  tuple.atPut(1, moduleAt(&runtime, "__main__", "b"));
  tuple.atPut(2, moduleAt(&runtime, "__main__", "c"));
  Object result(&scope, runtime.bytesFromTuple(thread, tuple, 3));
  EXPECT_TRUE(isBytesEqualsCStr(result, "abc"));
}

TEST(RuntimeBytesTest, Subseq) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  View<byte> hello(reinterpret_cast<const byte*>("Hello world!"), 12);
  Bytes bytes(&scope, runtime.newBytesWithAll(hello));
  ASSERT_EQ(bytes.length(), 12);

  Bytes copy(&scope, runtime.bytesSubseq(thread, bytes, 6, 5));
  EXPECT_TRUE(isBytesEqualsCStr(copy, "world"));
}

TEST(RuntimeDictTest, EmptyDictInvariants) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());

  EXPECT_EQ(dict.numItems(), 0);
  ASSERT_TRUE(dict.data().isTuple());
  EXPECT_EQ(Tuple::cast(dict.data()).length(), 0);
}

TEST(RuntimeDictTest, GetSet) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  Object key(&scope, SmallInt::fromWord(12345));

  // Looking up a key that doesn't exist should fail
  EXPECT_TRUE(runtime.dictAt(thread, dict, key).isError());

  // Store a value
  Object stored(&scope, SmallInt::fromWord(67890));
  runtime.dictAtPut(thread, dict, key, stored);
  EXPECT_EQ(dict.numItems(), 1);

  // Retrieve the stored value
  RawObject retrieved = runtime.dictAt(thread, dict, key);
  EXPECT_EQ(retrieved, *stored);

  // Overwrite the stored value
  Object new_value(&scope, SmallInt::fromWord(5555));
  runtime.dictAtPut(thread, dict, key, new_value);
  EXPECT_EQ(dict.numItems(), 1);

  // Get the new value
  retrieved = runtime.dictAt(thread, dict, key);
  EXPECT_EQ(retrieved, *new_value);
}

TEST(RuntimeDictTest, Remove) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  Object key(&scope, SmallInt::fromWord(12345));

  // Removing a key that doesn't exist should fail
  bool is_missing = runtime.dictRemove(thread, dict, key).isError();
  EXPECT_TRUE(is_missing);

  // Removing a key that exists should succeed and return the value that was
  // stored.
  Object stored(&scope, SmallInt::fromWord(54321));

  runtime.dictAtPut(thread, dict, key, stored);
  EXPECT_EQ(dict.numItems(), 1);

  RawObject retrieved = runtime.dictRemove(thread, dict, key);
  ASSERT_FALSE(retrieved.isError());
  ASSERT_EQ(SmallInt::cast(retrieved).value(), SmallInt::cast(*stored).value());

  // Looking up a key that was deleted should fail
  EXPECT_TRUE(runtime.dictAt(thread, dict, key).isError());
  EXPECT_EQ(dict.numItems(), 0);
}

TEST(RuntimeDictTest, Length) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());

  // Add 10 items and make sure length reflects it
  for (int i = 0; i < 10; i++) {
    Object key(&scope, SmallInt::fromWord(i));
    runtime.dictAtPut(thread, dict, key, key);
  }
  EXPECT_EQ(dict.numItems(), 10);

  // Remove half the items
  for (int i = 0; i < 5; i++) {
    Object key(&scope, SmallInt::fromWord(i));
    ASSERT_FALSE(runtime.dictRemove(thread, dict, key).isError());
  }
  EXPECT_EQ(dict.numItems(), 5);
}

TEST(RuntimeDictTest, AtIfAbsentPutLength) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());

  Object k1(&scope, SmallInt::fromWord(1));
  Object v1(&scope, SmallInt::fromWord(111));
  runtime.dictAtPut(thread, dict, k1, v1);
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
  SmallIntCallback cb(222);
  runtime.dictAtIfAbsentPut(thread, dict, k2, &cb);
  EXPECT_EQ(dict.numItems(), 2);
  RawObject retrieved = runtime.dictAt(thread, dict, k2);
  EXPECT_TRUE(isIntEqualsWord(retrieved, 222));

  // Don't overrwite existing item 1 -> v1
  Object k3(&scope, SmallInt::fromWord(1));
  SmallIntCallback cb3(333);
  runtime.dictAtIfAbsentPut(thread, dict, k3, &cb3);
  EXPECT_EQ(dict.numItems(), 2);
  retrieved = runtime.dictAt(thread, dict, k3);
  EXPECT_EQ(retrieved, *v1);
}

TEST(RuntimeDictTest, DictAtPutGrowsDictWhenDictIsEmpty) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  EXPECT_EQ(dict.capacity(), 0);

  Object first_key(&scope, SmallInt::fromWord(0));
  Object first_value(&scope, SmallInt::fromWord(1));
  runtime.dictAtPut(thread, dict, first_key, first_value);

  word initial_capacity = Runtime::kInitialDictCapacity;
  EXPECT_EQ(dict.numItems(), 1);
  EXPECT_EQ(dict.capacity(), initial_capacity);
}

TEST(RuntimeDictTest, DictAtPutGrowsDictWhenTwoThirdsFull) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());

  // Fill in one fewer keys than would require growing the underlying object
  // array again.
  word threshold = ((Runtime::kInitialDictCapacity * 2) / 3);
  for (word i = 0; i < threshold; i++) {
    Object key(&scope, SmallInt::fromWord(i));
    Object value(&scope, SmallInt::fromWord(-i));
    runtime.dictAtPut(thread, dict, key, value);
  }
  EXPECT_EQ(dict.numItems(), threshold);
  word initial_capacity = Runtime::kInitialDictCapacity;
  EXPECT_EQ(dict.capacity(), initial_capacity);

  // Add another key which should force us to double the capacity
  Object last_key(&scope, SmallInt::fromWord(threshold));
  Object last_value(&scope, SmallInt::fromWord(-threshold));
  runtime.dictAtPut(thread, dict, last_key, last_value);
  EXPECT_EQ(dict.numItems(), threshold + 1);
  EXPECT_EQ(dict.capacity(), initial_capacity * Runtime::kDictGrowthFactor);

  // Make sure we can still read all the stored keys/values.
  for (word i = 0; i < threshold + 1; i++) {
    Object key(&scope, SmallInt::fromWord(i));
    RawObject value = runtime.dictAt(thread, dict, key);
    ASSERT_FALSE(value.isError());
    EXPECT_TRUE(Object::equals(value, SmallInt::fromWord(-i)));
  }
}

TEST(RuntimeDictTest, CollidingKeys) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());

  // Add two different keys with different values using the same hash
  Object key1(&scope, SmallInt::fromWord(1));
  runtime.dictAtPut(thread, dict, key1, key1);

  Object key2(&scope, Bool::trueObj());
  runtime.dictAtPut(thread, dict, key2, key2);

  // Make sure we get both back
  RawObject retrieved = runtime.dictAt(thread, dict, key1);
  EXPECT_EQ(retrieved, *key1);

  retrieved = runtime.dictAt(thread, dict, key2);
  ASSERT_TRUE(retrieved.isBool());
  EXPECT_EQ(Bool::cast(retrieved).value(), Bool::cast(*key2).value());
}

TEST(RuntimeDictTest, MixedKeys) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());

  // Add keys of different type
  Object int_key(&scope, SmallInt::fromWord(100));
  runtime.dictAtPut(thread, dict, int_key, int_key);

  Object str_key(&scope, runtime.newStrFromCStr("testing 123"));
  runtime.dictAtPut(thread, dict, str_key, str_key);

  // Make sure we get the appropriate values back out
  RawObject retrieved = runtime.dictAt(thread, dict, int_key);
  EXPECT_EQ(retrieved, *int_key);

  retrieved = runtime.dictAt(thread, dict, str_key);
  ASSERT_TRUE(retrieved.isStr());
  EXPECT_TRUE(Object::equals(*str_key, retrieved));
}

TEST(RuntimeDictTest, GetKeys) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  // Create keys
  Tuple keys(&scope, runtime.newTuple(4));
  keys.atPut(0, SmallInt::fromWord(100));
  keys.atPut(1, runtime.newStrFromCStr("testing 123"));
  keys.atPut(2, Bool::trueObj());
  keys.atPut(3, NoneType::object());

  // Add keys to dict
  Dict dict(&scope, runtime.newDict());
  for (word i = 0; i < keys.length(); i++) {
    Object key(&scope, keys.at(i));
    runtime.dictAtPut(thread, dict, key, key);
  }

  // Grab the keys and verify everything is there
  Tuple retrieved(&scope, runtime.dictKeys(thread, dict));
  ASSERT_EQ(retrieved.length(), keys.length());
  for (word i = 0; i < keys.length(); i++) {
    Object key(&scope, keys.at(i));
    EXPECT_TRUE(tupleContains(retrieved, key)) << " missing key " << i;
  }
}

TEST(RuntimeDictTest, CanCreateDictItems) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  RawObject iter = runtime.newDictItemIterator(thread, dict);
  ASSERT_TRUE(iter.isDictItemIterator());
}

TEST(RuntimeListTest, ListGrowth) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list(&scope, runtime.newList());
  Tuple array1(&scope, runtime.newTuple(1));
  list.setItems(*array1);
  EXPECT_EQ(array1.length(), 1);
  runtime.listEnsureCapacity(thread, list, 2);
  Tuple array2(&scope, list.items());
  EXPECT_NE(*array1, *array2);
  EXPECT_GE(array2.length(), 2);

  Tuple array4(&scope, runtime.newTuple(4));
  list.setItems(*array4);
  runtime.listEnsureCapacity(thread, list, 5);
  Tuple array16(&scope, list.items());
  EXPECT_NE(*array4, *array16);
  EXPECT_EQ(array16.length(), 16);
  runtime.listEnsureCapacity(thread, list, 17);
  Tuple array24(&scope, list.items());
  EXPECT_NE(*array16, *array24);
  EXPECT_EQ(array24.length(), 24);
  runtime.listEnsureCapacity(thread, list, 40);
  EXPECT_EQ(list.capacity(), 40);
}

TEST(RuntimeListTest, EmptyListInvariants) {
  Runtime runtime;
  RawList list = List::cast(runtime.newList());
  ASSERT_EQ(list.capacity(), 0);
  ASSERT_EQ(list.numItems(), 0);
}

TEST(RuntimeListTest, AppendToList) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list(&scope, runtime.newList());

  // Check that list capacity grows by 1.5
  word expected_capacity[] = {16, 16, 16, 16, 16, 16, 16, 16, 16,
                              16, 16, 16, 16, 16, 16, 16, 24, 24,
                              24, 24, 24, 24, 24, 24, 36};
  for (int i = 0; i < 25; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.listAdd(thread, list, value);
    ASSERT_EQ(list.capacity(), expected_capacity[i]) << i;
    ASSERT_EQ(list.numItems(), i + 1) << i;
  }

  // Sanity check list contents
  for (int i = 0; i < 25; i++) {
    EXPECT_TRUE(isIntEqualsWord(list.at(i), i)) << i;
  }
}

TEST(RuntimeTest, NewByteArray) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  ByteArray array(&scope, runtime.newByteArray());
  EXPECT_EQ(array.numItems(), 0);
  EXPECT_EQ(array.capacity(), 0);
}

TEST(RuntimeTest, NewBytes) {
  Runtime runtime;
  HandleScope scope;

  Bytes len0(&scope, Bytes::empty());
  EXPECT_EQ(len0.length(), 0);

  Bytes len3(&scope, runtime.newBytes(3, 9));
  EXPECT_EQ(len3.length(), 3);
  EXPECT_EQ(len3.byteAt(0), 9);
  EXPECT_EQ(len3.byteAt(1), 9);
  EXPECT_EQ(len3.byteAt(2), 9);

  Bytes len254(&scope, runtime.newBytes(254, 0));
  EXPECT_EQ(len254.length(), 254);

  Bytes len255(&scope, runtime.newBytes(255, 0));
  EXPECT_EQ(len255.length(), 255);
}

TEST(RuntimeTest, NewBytesWithAll) {
  Runtime runtime;
  HandleScope scope;

  Bytes len0(&scope, runtime.newBytesWithAll(View<byte>(nullptr, 0)));
  EXPECT_EQ(len0.length(), 0);

  const byte src1[] = {0x42};
  Bytes len1(&scope, runtime.newBytesWithAll(src1));
  EXPECT_EQ(len1.length(), 1);
  EXPECT_EQ(len1.byteAt(0), 0x42);

  const byte src3[] = {0xAA, 0xBB, 0xCC};
  Bytes len3(&scope, runtime.newBytesWithAll(src3));
  EXPECT_EQ(len3.length(), 3);
  EXPECT_EQ(len3.byteAt(0), 0xAA);
  EXPECT_EQ(len3.byteAt(1), 0xBB);
  EXPECT_EQ(len3.byteAt(2), 0xCC);
}

TEST(RuntimeTest, LargeBytesSizeRoundedUpToPointerSizeMultiple) {
  Runtime runtime;
  HandleScope scope;

  LargeBytes len10(&scope, runtime.newBytes(10, 0));
  EXPECT_EQ(len10.size(), Utils::roundUp(kPointerSize + 10, kPointerSize));

  LargeBytes len254(&scope, runtime.newBytes(254, 0));
  EXPECT_EQ(len254.size(), Utils::roundUp(kPointerSize + 254, kPointerSize));

  LargeBytes len255(&scope, runtime.newBytes(255, 0));
  EXPECT_EQ(len255.size(),
            Utils::roundUp(kPointerSize * 2 + 255, kPointerSize));
}

TEST(RuntimeTest, NewTuple) {
  Runtime runtime;
  HandleScope scope;

  Tuple a0(&scope, runtime.newTuple(0));
  EXPECT_EQ(a0.length(), 0);

  Tuple a1(&scope, runtime.newTuple(1));
  ASSERT_EQ(a1.length(), 1);
  EXPECT_EQ(a1.at(0), NoneType::object());
  a1.atPut(0, SmallInt::fromWord(42));
  EXPECT_EQ(a1.at(0), SmallInt::fromWord(42));

  Tuple a300(&scope, runtime.newTuple(300));
  ASSERT_EQ(a300.length(), 300);
}

TEST(RuntimeTest, NewStr) {
  Runtime runtime;
  HandleScope scope;
  Str empty0(&scope, runtime.newStrWithAll(View<byte>(nullptr, 0)));
  ASSERT_TRUE(empty0.isSmallStr());
  EXPECT_EQ(empty0.length(), 0);

  Str empty1(&scope, runtime.newStrWithAll(View<byte>(nullptr, 0)));
  ASSERT_TRUE(empty1.isSmallStr());
  EXPECT_EQ(*empty0, *empty1);

  Str empty2(&scope, runtime.newStrFromCStr("\0"));
  ASSERT_TRUE(empty2.isSmallStr());
  EXPECT_EQ(*empty0, *empty2);

  const byte bytes1[1] = {0};
  Str s1(&scope, runtime.newStrWithAll(bytes1));
  ASSERT_TRUE(s1.isSmallStr());
  EXPECT_EQ(s1.length(), 1);

  const byte bytes254[254] = {0};
  Str s254(&scope, runtime.newStrWithAll(bytes254));
  EXPECT_EQ(s254.length(), 254);
  ASSERT_TRUE(s254.isLargeStr());
  EXPECT_EQ(HeapObject::cast(*s254).size(),
            Utils::roundUp(kPointerSize + 254, kPointerSize));

  const byte bytes255[255] = {0};
  Str s255(&scope, runtime.newStrWithAll(bytes255));
  EXPECT_EQ(s255.length(), 255);
  ASSERT_TRUE(s255.isLargeStr());
  EXPECT_EQ(HeapObject::cast(*s255).size(),
            Utils::roundUp(kPointerSize * 2 + 255, kPointerSize));

  const byte bytes300[300] = {0};
  Str s300(&scope, runtime.newStrWithAll(bytes300));
  ASSERT_EQ(s300.length(), 300);
}

TEST(RuntimeTest, NewStrFromByteArrayCopiesByteArray) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  ByteArray array(&scope, runtime.newByteArray());
  Object result(&scope, runtime.newStrFromByteArray(array));
  EXPECT_TRUE(isStrEqualsCStr(*result, ""));

  const byte byte_array[] = {'h', 'e', 'l', 'l', 'o'};
  runtime.byteArrayExtend(thread, array, byte_array);
  result = runtime.newStrFromByteArray(array);
  EXPECT_TRUE(isStrEqualsCStr(*result, "hello"));

  const byte byte_array2[] = {' ', 'w', 'o', 'r', 'l', 'd'};
  runtime.byteArrayExtend(thread, array, byte_array2);
  result = runtime.newStrFromByteArray(array);
  EXPECT_TRUE(isStrEqualsCStr(*result, "hello world"));
}

TEST(RuntimeTest, NewStrFromFmtFormatsWord) {
  Runtime runtime;
  word x = 5;
  HandleScope scope;
  Object result(&scope, runtime.newStrFromFmt("hello %w world", x));
  EXPECT_TRUE(isStrEqualsCStr(*result, "hello 5 world"));
}

TEST(RuntimeTest, NewStrFromFmtWithStrArg) {
  Runtime runtime;
  HandleScope scope;

  Object str(&scope, runtime.newStrFromCStr("hello"));
  Object result(&scope, runtime.newStrFromFmt("%S", &str));
  EXPECT_EQ(*result, str);
}

TEST(StrBuiltinsTest, NewStrFromFmtFormatsFunctionName) {
  Runtime runtime;
  HandleScope scope;
  Function function(&scope, runtime.newFunction());
  function.setQualname(runtime.newStrFromCStr("foo"));
  Object str(&scope, runtime.newStrFromFmt("hello %F", &function));
  EXPECT_TRUE(isStrEqualsCStr(*str, "hello foo"));
}

TEST(StrBuiltinsTest, NewStrFromFmtFormatsTypeName) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, runtime.newDict());
  Object str(&scope, runtime.newStrFromFmt("hello %T", &obj));
  EXPECT_TRUE(isStrEqualsCStr(*str, "hello dict"));
}

TEST(StrBuiltinsTest, NewStrFromFmtFormatsSymbolid) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromFmt("hello %Y", SymbolId::kDict));
  EXPECT_TRUE(isStrEqualsCStr(*str, "hello dict"));
}

TEST(RuntimeTest, NewStrWithAll) {
  Runtime runtime;
  HandleScope scope;

  Str str0(&scope, runtime.newStrWithAll(View<byte>(nullptr, 0)));
  EXPECT_EQ(str0.length(), 0);
  EXPECT_TRUE(str0.equalsCStr(""));

  const byte bytes3[] = {'A', 'B', 'C'};
  Str str3(&scope, runtime.newStrWithAll(bytes3));
  EXPECT_EQ(str3.length(), 3);
  EXPECT_TRUE(str3.equalsCStr("ABC"));

  const byte bytes10[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'};
  Str str10(&scope, runtime.newStrWithAll(bytes10));
  EXPECT_EQ(str10.length(), 10);
  EXPECT_TRUE(str10.equalsCStr("ABCDEFGHIJ"));
}

TEST(RuntimeTest, NewStrFromUTF32WithZeroSizeReturnsEmpty) {
  Runtime runtime;
  HandleScope scope;
  int32_t str[2] = {'a', 's'};
  Str empty(&scope, runtime.newStrFromUTF32(View<int32_t>(str, 0)));
  EXPECT_EQ(empty.length(), 0);
}

TEST(RuntimeTest, NewStrFromUTF32WithLargeASCIIStringReturnsString) {
  Runtime runtime;
  HandleScope scope;
  int32_t str[7] = {'a', 'b', 'c', '1', '2', '3', '-'};
  Str unicode(&scope, runtime.newStrFromUTF32(View<int32_t>(str, 7)));
  EXPECT_EQ(unicode.length(), 7);
  EXPECT_TRUE(unicode.equalsCStr("abc123-"));
}

TEST(RuntimeTest, NewStrFromUTF32WithSmallASCIIStringReturnsString) {
  Runtime runtime;
  HandleScope scope;
  int32_t str[7] = {'a', 'b'};
  Str unicode(&scope, runtime.newStrFromUTF32(View<int32_t>(str, 2)));
  EXPECT_EQ(unicode.length(), 2);
  EXPECT_TRUE(unicode.equalsCStr("ab"));
}

TEST(RuntimeTest, NewStrFromUTF32WithSmallNonASCIIReturnsString) {
  Runtime runtime;
  HandleScope scope;
  const int32_t codepoints[] = {0xC4};
  Str unicode(&scope, runtime.newStrFromUTF32(codepoints));
  EXPECT_TRUE(unicode.equals(SmallStr::fromCodePoint(0xC4)));
}

TEST(RuntimeTest, NewStrFromUTF32WithLargeNonASCIIReturnsString) {
  Runtime runtime;
  HandleScope scope;
  const int32_t codepoints[] = {0x3041, ' ', 'c', 0xF6,
                                0xF6,   'l', ' ', 0x1F192};
  Str unicode(&scope, runtime.newStrFromUTF32(codepoints));
  Str expected(&scope, runtime.newStrFromCStr(
                           "\xe3\x81\x81 c\xC3\xB6\xC3\xB6l \xF0\x9F\x86\x92"));
  EXPECT_TRUE(unicode.equals(*expected));
}

TEST(RuntimeTest, HashBools) {
  Runtime runtime;

  // In CPython, False hashes to 0 and True hashes to 1.
  EXPECT_TRUE(isIntEqualsWord(runtime.hash(Bool::falseObj()), 0));
  EXPECT_TRUE(isIntEqualsWord(runtime.hash(Bool::trueObj()), 1));
}

TEST(RuntimeTest, HashLargeBytes) {
  Runtime runtime;
  HandleScope scope;

  // LargeBytes have their hash codes computed lazily.
  const byte src1[] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8};
  LargeBytes arr1(&scope, runtime.newBytesWithAll(src1));
  EXPECT_EQ(arr1.header().hashCode(), 0);
  word hash1 = SmallInt::cast(runtime.hash(*arr1)).value();
  EXPECT_NE(arr1.header().hashCode(), 0);
  EXPECT_EQ(arr1.header().hashCode(), hash1);

  word code1 = runtime.siphash24(src1);
  EXPECT_EQ(code1 & RawHeader::kHashCodeMask, static_cast<uword>(hash1));

  // LargeBytes with different values should (ideally) hash differently.
  const byte src2[] = {0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1};
  LargeBytes arr2(&scope, runtime.newBytesWithAll(src2));
  word hash2 = SmallInt::cast(runtime.hash(*arr2)).value();
  EXPECT_NE(hash1, hash2);

  word code2 = runtime.siphash24(src2);
  EXPECT_EQ(code2 & RawHeader::kHashCodeMask, static_cast<uword>(hash2));

  // LargeBytes with the same value should hash the same.
  LargeBytes arr3(&scope, runtime.newBytesWithAll(src1));
  EXPECT_NE(arr3, arr1);
  word hash3 = SmallInt::cast(runtime.hash(*arr3)).value();
  EXPECT_EQ(hash1, hash3);
}

TEST(RuntimeTest, HashSmallInts) {
  Runtime runtime;

  // In CPython, Ints hash to themselves.
  EXPECT_TRUE(isIntEqualsWord(runtime.hash(SmallInt::fromWord(123)), 123));
  EXPECT_TRUE(isIntEqualsWord(runtime.hash(SmallInt::fromWord(456)), 456));
}

TEST(RuntimeTest, HashSingletonImmediates) {
  Runtime runtime;

  // In CPython, these objects hash to arbitrary values.
  word none_value = NoneType::object().raw();
  RawSmallInt hash_none = SmallInt::cast(runtime.hash(NoneType::object()));
  EXPECT_EQ(hash_none.value(), none_value);

  word error_value = Error::error().raw();
  RawSmallInt hash_error = SmallInt::cast(runtime.hash(Error::error()));
  EXPECT_EQ(hash_error.value(), error_value);
}

TEST(RuntimeTest, HashStr) {
  Runtime runtime;
  HandleScope scope;

  // LargeStr instances have their hash codes computed lazily.
  Object str1(&scope, runtime.newStrFromCStr("testing 123"));
  EXPECT_EQ(HeapObject::cast(*str1).header().hashCode(), 0);
  RawSmallInt hash1 = SmallInt::cast(runtime.hash(*str1));
  EXPECT_NE(HeapObject::cast(*str1).header().hashCode(), 0);
  EXPECT_EQ(HeapObject::cast(*str1).header().hashCode(), hash1.value());

  // Str with different values should (ideally) hash differently.
  Str str2(&scope, runtime.newStrFromCStr("321 testing"));
  RawSmallInt hash2 = SmallInt::cast(runtime.hash(*str2));
  EXPECT_NE(hash1, hash2);

  // Strings with the same value should hash the same.
  Str str3(&scope, runtime.newStrFromCStr("testing 123"));
  RawSmallInt hash3 = SmallInt::cast(runtime.hash(*str3));
  EXPECT_EQ(hash1, hash3);
}

TEST(RuntimeTest, InitializeRandomSetsRandomRandomRNGSeed) {
  ::unsetenv("PYTHONHASHSEED");
  Runtime runtime0;
  uword r0 = runtime0.random();
  Runtime runtime1;
  uword r1 = runtime1.random();
  Runtime runtime2;
  uword r2 = runtime2.random();
  // Having 3 random numbers be the same will practically never happen.
  EXPECT_TRUE(r0 != r1 || r0 != r2);
}

TEST(RuntimeTest,
     InitializeRandomWithPyroHashSeedEnvVarSetsDeterministicRNGSeed) {
  ::setenv("PYTHONHASHSEED", "0", 1);
  Runtime runtime0;
  uword r0_a = runtime0.random();
  uword r0_b = runtime0.random();
  Runtime runtime1;
  uword r1_a = runtime1.random();
  uword r1_b = runtime1.random();
  EXPECT_EQ(r0_a, r1_a);
  EXPECT_EQ(r0_b, r1_b);
  ::unsetenv("PYTHONHASHSEED");
}

TEST(RuntimeTest, Random) {
  Runtime runtime;
  uword r1 = runtime.random();
  uword r2 = runtime.random();
  EXPECT_NE(r1, r2);
  uword r3 = runtime.random();
  EXPECT_NE(r2, r3);
  uword r4 = runtime.random();
  EXPECT_NE(r3, r4);
}

TEST(RuntimeTest, TrackObjectAndUnTrackObject) {
  Runtime runtime;
  ListEntry entry0{nullptr, nullptr};
  ListEntry entry1{nullptr, nullptr};

  EXPECT_TRUE(runtime.trackObject(&entry0));
  EXPECT_TRUE(runtime.trackObject(&entry1));
  // Trying to track an already tracked object returns false.
  EXPECT_FALSE(runtime.trackObject(&entry0));
  EXPECT_FALSE(runtime.trackObject(&entry1));

  EXPECT_TRUE(runtime.untrackObject(&entry0));
  EXPECT_TRUE(runtime.untrackObject(&entry1));

  // Trying to untrack an already untracked object returns false.
  EXPECT_FALSE(runtime.untrackObject(&entry0));
  EXPECT_FALSE(runtime.untrackObject(&entry1));

  // Verify untracked entires are reset to nullptr.
  EXPECT_EQ(entry0.prev, nullptr);
  EXPECT_EQ(entry0.next, nullptr);
  EXPECT_EQ(entry1.prev, nullptr);
  EXPECT_EQ(entry1.next, nullptr);
}

TEST(RuntimeTest, HashCodeSizeCheck) {
  Runtime runtime;
  HandleScope scope;
  Object name(&scope, Str::empty());
  RawObject code = runtime.newEmptyCode(name);
  ASSERT_TRUE(code.isHeapObject());
  EXPECT_EQ(HeapObject::cast(code).header().hashCode(), 0);
  // Verify that large-magnitude random numbers are properly
  // truncated to somethat which fits in a SmallInt

  // Conspire based on knoledge of the random number genrated to
  // create a high-magnitude result from Runtime::random
  // which is truncated to 0 for storage in the header and
  // replaced with "1" so no hash code has value 0.
  uword high = static_cast<uword>(1) << (8 * sizeof(uword) - 1);
  uword state[2] = {0, high};
  uword secret[2] = {0, 0};
  runtime.seedRandom(state, secret);
  uword first = runtime.random();
  EXPECT_EQ(first, high);
  runtime.seedRandom(state, secret);
  EXPECT_TRUE(isIntEqualsWord(runtime.hash(code), 1));
}

TEST(RuntimeTest, NewCapacity) {
  Runtime runtime;
  // ensure initial capacity
  EXPECT_GE(runtime.newCapacity(1, 0), 16);

  // grow by factor of 1.5, rounding down
  EXPECT_EQ(runtime.newCapacity(20, 22), 30);
  EXPECT_EQ(runtime.newCapacity(64, 77), 96);
  EXPECT_EQ(runtime.newCapacity(25, 30), 37);

  // ensure growth
  EXPECT_EQ(runtime.newCapacity(20, 17), 30);
  EXPECT_EQ(runtime.newCapacity(20, 20), 30);

  // if factor of 1.5 is insufficient, grow exactly to minimum capacity
  EXPECT_EQ(runtime.newCapacity(20, 40), 40);
  EXPECT_EQ(runtime.newCapacity(20, 70), 70);

  // capacity has ceiling of SmallInt::kMaxValue
  EXPECT_EQ(runtime.newCapacity(SmallInt::kMaxValue - 1, SmallInt::kMaxValue),
            SmallInt::kMaxValue);
}

TEST(RuntimeTest, InternLargeStr) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Set interned(&scope, runtime.interned());

  // Creating an ordinary large string should not affect on the intern table.
  word num_interned = interned.numItems();
  Object str1(&scope, runtime.newStrFromCStr("hello, world"));
  ASSERT_TRUE(str1.isLargeStr());
  EXPECT_EQ(num_interned, interned.numItems());
  EXPECT_FALSE(runtime.setIncludes(thread, interned, str1));
  EXPECT_FALSE(runtime.isInternedStr(str1));

  // Interning the string should add it to the intern table and increase the
  // size of the intern table by one.
  num_interned = interned.numItems();
  Object sym1(&scope, runtime.internStr(str1));
  EXPECT_TRUE(runtime.setIncludes(thread, interned, str1));
  EXPECT_EQ(*sym1, *str1);
  EXPECT_EQ(num_interned + 1, interned.numItems());
  EXPECT_TRUE(runtime.isInternedStr(str1));

  Object str2(&scope, runtime.newStrFromCStr("goodbye, world"));
  ASSERT_TRUE(str2.isLargeStr());
  EXPECT_NE(*str1, *str2);

  // Intern another string and make sure we get it back (as opposed to the
  // previously interned string).
  num_interned = interned.numItems();
  Object sym2(&scope, runtime.internStr(str2));
  EXPECT_EQ(num_interned + 1, interned.numItems());
  EXPECT_TRUE(runtime.setIncludes(thread, interned, str2));
  EXPECT_EQ(*sym2, *str2);
  EXPECT_NE(*sym1, *sym2);

  // Create a unique copy of a previously created string.
  Object str3(&scope, runtime.newStrFromCStr("hello, world"));
  ASSERT_TRUE(str3.isLargeStr());
  EXPECT_NE(*str1, *str3);
  EXPECT_TRUE(runtime.setIncludes(thread, interned, str3));
  EXPECT_FALSE(runtime.isInternedStr(str3));

  // Interning a duplicate string should not affecct the intern table.
  num_interned = interned.numItems();
  Object sym3(&scope, runtime.internStr(str3));
  EXPECT_EQ(num_interned, interned.numItems());
  EXPECT_NE(*sym3, *str3);
  EXPECT_EQ(*sym3, *sym1);
}

TEST(RuntimeTest, InternSmallStr) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Set interned(&scope, runtime.interned());

  // Creating a small string should not affect the intern table.
  word num_interned = interned.numItems();
  Object str(&scope, runtime.newStrFromCStr("a"));
  ASSERT_TRUE(str.isSmallStr());
  EXPECT_FALSE(runtime.setIncludes(thread, interned, str));
  EXPECT_EQ(num_interned, interned.numItems());

  // Interning a small string should have no affect on the intern table.
  Object sym(&scope, runtime.internStr(str));
  EXPECT_TRUE(sym.isSmallStr());
  EXPECT_FALSE(runtime.setIncludes(thread, interned, str));
  EXPECT_EQ(num_interned, interned.numItems());
  EXPECT_EQ(*sym, *str);
  EXPECT_TRUE(runtime.isInternedStr(str));
}

TEST(RuntimeTest, InternCStr) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Set interned(&scope, runtime.interned());

  word num_interned = interned.numItems();
  Object sym(&scope, runtime.internStrFromCStr("hello, world"));
  EXPECT_TRUE(sym.isStr());
  EXPECT_TRUE(runtime.setIncludes(thread, interned, sym));
  EXPECT_EQ(num_interned + 1, interned.numItems());
  EXPECT_TRUE(runtime.isInternedStr(sym));
}

TEST(RuntimeTest, IsInternWithInternedStrReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.internStrFromCStr("hello world"));
  EXPECT_TRUE(runtime.isInternedStr(str));
}

TEST(RuntimeTest, IsInternWithStrReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("hello world"));
  EXPECT_FALSE(runtime.isInternedStr(str));
}

TEST(RuntimeTest, CollectAttributes) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Object foo(&scope, runtime.newStrFromCStr("foo"));
  Object bar(&scope, runtime.newStrFromCStr("bar"));
  Object baz(&scope, runtime.newStrFromCStr("baz"));

  Tuple names(&scope, runtime.newTuple(3));
  names.atPut(0, *foo);
  names.atPut(1, *bar);
  names.atPut(2, *baz);

  Tuple consts(&scope, runtime.newTuple(4));
  consts.atPut(0, SmallInt::fromWord(100));
  consts.atPut(1, SmallInt::fromWord(200));
  consts.atPut(2, SmallInt::fromWord(300));
  consts.atPut(3, NoneType::object());

  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  code.setNames(*names);
  // Bytecode for the snippet:
  //
  //   def __init__(self):
  //       self.foo = 100
  //       self.foo = 200
  //
  // The assignment to self.foo is intentionally duplicated to ensure that we
  // only record a single attribute name.
  const byte bytecode[] = {LOAD_CONST,   0, LOAD_FAST, 0, STORE_ATTR, 0,
                           LOAD_CONST,   1, LOAD_FAST, 0, STORE_ATTR, 0,
                           RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));

  Dict attributes(&scope, runtime.newDict());
  runtime.collectAttributes(code, attributes);

  // We should have collected a single attribute: 'foo'
  EXPECT_EQ(attributes.numItems(), 1);

  // Check that we collected 'foo'
  Object result(&scope, runtime.dictAt(thread, attributes, foo));
  ASSERT_TRUE(result.isStr());
  EXPECT_TRUE(Str::cast(*result).equals(*foo));

  // Bytecode for the snippet:
  //
  //   def __init__(self):
  //       self.bar = 200
  //       self.baz = 300
  const byte bc2[] = {LOAD_CONST,   1, LOAD_FAST, 0, STORE_ATTR, 1,
                      LOAD_CONST,   2, LOAD_FAST, 0, STORE_ATTR, 2,
                      RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bc2));
  runtime.collectAttributes(code, attributes);

  // We should have collected a two more attributes: 'bar' and 'baz'
  EXPECT_EQ(attributes.numItems(), 3);

  // Check that we collected 'bar'
  result = runtime.dictAt(thread, attributes, bar);
  ASSERT_TRUE(result.isStr());
  EXPECT_TRUE(Str::cast(*result).equals(*bar));

  // Check that we collected 'baz'
  result = runtime.dictAt(thread, attributes, baz);
  ASSERT_TRUE(result.isStr());
  EXPECT_TRUE(Str::cast(*result).equals(*baz));
}

TEST(RuntimeTest, CollectAttributesWithExtendedArg) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Object foo(&scope, runtime.newStrFromCStr("foo"));
  Object bar(&scope, runtime.newStrFromCStr("bar"));

  Tuple names(&scope, runtime.newTuple(2));
  names.atPut(0, *foo);
  names.atPut(1, *bar);

  Tuple consts(&scope, runtime.newTuple(1));
  consts.atPut(0, NoneType::object());

  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  code.setNames(*names);
  // Bytecode for the snippet:
  //
  //   def __init__(self):
  //       self.foo = None
  //
  // There is an additional LOAD_FAST that is preceded by an EXTENDED_ARG
  // that must be skipped.
  const byte bytecode[] = {LOAD_CONST, 0, EXTENDED_ARG, 10, LOAD_FAST, 0,
                           STORE_ATTR, 1, LOAD_CONST,   0,  LOAD_FAST, 0,
                           STORE_ATTR, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));

  Dict attributes(&scope, runtime.newDict());
  runtime.collectAttributes(code, attributes);

  // We should have collected a single attribute: 'foo'
  EXPECT_EQ(attributes.numItems(), 1);

  // Check that we collected 'foo'
  Object result(&scope, runtime.dictAt(thread, attributes, foo));
  ASSERT_TRUE(result.isStr());
  EXPECT_TRUE(Str::cast(*result).equals(*foo));
}

TEST(RuntimeTest, GetTypeConstructor) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type type(&scope, runtime.newType());
  Dict type_dict(&scope, runtime.newDict());
  type.setDict(*type_dict);

  EXPECT_EQ(runtime.classConstructor(type), NoneType::object());

  Object init(&scope, runtime.symbols()->DunderInit());
  Object func(&scope, makeTestFunction());
  runtime.dictAtPutInValueCell(thread, type_dict, init, func);

  EXPECT_EQ(runtime.classConstructor(type), *func);
}

TEST(RuntimeTest, NewInstanceEmptyClass) {
  Runtime runtime;
  HandleScope scope;

  ASSERT_FALSE(runFromCStr(&runtime, "class MyEmptyClass: pass").isError());

  Module main(&scope, findModule(&runtime, "__main__"));
  Type type(&scope, moduleAt(&runtime, main, "MyEmptyClass"));
  Layout layout(&scope, type.instanceLayout());
  EXPECT_EQ(layout.instanceSize(), 1 * kPointerSize);

  Type cls(&scope, layout.describedType());
  EXPECT_TRUE(isStrEqualsCStr(cls.name(), "MyEmptyClass"));

  Instance instance(&scope, runtime.newInstance(layout));
  EXPECT_TRUE(instance.isInstance());
  EXPECT_EQ(instance.header().layoutId(), layout.id());
}

TEST(RuntimeTest, NewInstanceManyAttributes) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class MyTypeWithAttributes():
  def __init__(self):
    self.a = 1
    self.b = 2
    self.c = 3
)";
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());

  Module main(&scope, findModule(&runtime, "__main__"));
  Type type(&scope, moduleAt(&runtime, main, "MyTypeWithAttributes"));
  Layout layout(&scope, type.instanceLayout());
  ASSERT_EQ(layout.instanceSize(), 4 * kPointerSize);

  Type cls(&scope, layout.describedType());
  EXPECT_TRUE(isStrEqualsCStr(cls.name(), "MyTypeWithAttributes"));

  Instance instance(&scope, runtime.newInstance(layout));
  EXPECT_TRUE(instance.isInstance());
  EXPECT_EQ(instance.header().layoutId(), layout.id());
}

TEST(RuntimeTest, VerifySymbols) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Symbols* symbols = runtime.symbols();
  Object value(&scope, NoneType::object());
  for (int i = 0; i < static_cast<int>(SymbolId::kMaxId); i++) {
    SymbolId id = static_cast<SymbolId>(i);
    value = symbols->at(id);
    ASSERT_TRUE(value.isStr());
    const char* expected = symbols->literalAt(id);
    EXPECT_TRUE(runtime.isInternedStr(value)) << "at symbol " << expected;
    EXPECT_TRUE(Str::cast(*value).equalsCStr(expected))
        << "Incorrect symbol value for " << expected;
  }
}

static RawStr className(Runtime* runtime, RawObject o) {
  auto cls = Type::cast(runtime->typeOf(o));
  auto name = Str::cast(cls.name());
  return name;
}

TEST(RuntimeTest, TypeIds) {
  Runtime runtime;

  EXPECT_TRUE(isStrEqualsCStr(className(&runtime, Bool::trueObj()), "bool"));
  EXPECT_TRUE(
      isStrEqualsCStr(className(&runtime, NoneType::object()), "NoneType"));
  EXPECT_TRUE(isStrEqualsCStr(
      className(&runtime, runtime.newStrFromCStr("abc")), "smallstr"));

  for (word i = 0; i < 16; i++) {
    auto small_int = SmallInt::fromWord(i);
    EXPECT_TRUE(isStrEqualsCStr(className(&runtime, small_int), "smallint"));
  }
}

TEST(RuntimeTest, CallRunTwice) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "x = 42").isError());
  ASSERT_FALSE(runFromCStr(&runtime, "y = 1764").isError());

  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Object x(&scope, moduleAt(&runtime, main, "x"));
  EXPECT_TRUE(isIntEqualsWord(*x, 42));
  Object y(&scope, moduleAt(&runtime, main, "y"));
  EXPECT_TRUE(isIntEqualsWord(*y, 1764));
}

TEST(RuntimeStrTest, StrConcat) {
  Runtime runtime;
  HandleScope scope;

  Str str1(&scope, runtime.newStrFromCStr("abc"));
  Str str2(&scope, runtime.newStrFromCStr("def"));

  // Large strings.
  Str str3(&scope, runtime.newStrFromCStr("0123456789abcdef"));
  Str str4(&scope, runtime.newStrFromCStr("fedbca9876543210"));

  Thread* thread = Thread::current();
  Object concat12(&scope, runtime.strConcat(thread, str1, str2));
  Object concat34(&scope, runtime.strConcat(thread, str3, str4));

  Object concat13(&scope, runtime.strConcat(thread, str1, str3));
  Object concat31(&scope, runtime.strConcat(thread, str3, str1));

  // Test that we don't make large strings when small srings would suffice.
  EXPECT_TRUE(isStrEqualsCStr(*concat12, "abcdef"));
  EXPECT_TRUE(isStrEqualsCStr(*concat34, "0123456789abcdeffedbca9876543210"));
  EXPECT_TRUE(isStrEqualsCStr(*concat13, "abc0123456789abcdef"));
  EXPECT_TRUE(isStrEqualsCStr(*concat31, "0123456789abcdefabc"));

  EXPECT_TRUE(concat12.isSmallStr());
  EXPECT_TRUE(concat34.isLargeStr());
  EXPECT_TRUE(concat13.isLargeStr());
  EXPECT_TRUE(concat31.isLargeStr());
}

TEST(RuntimeTypeCallTest, TypeCallNoInitMethod) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class MyTypeWithNoInitMethod():
  def m(self):
    pass

c = MyTypeWithNoInitMethod()
)";
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());

  Module main(&scope, findModule(&runtime, "__main__"));
  Object instance(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(instance.isInstance());
  LayoutId layout_id = instance.layoutId();
  Layout layout(&scope, runtime.layoutAt(layout_id));
  EXPECT_EQ(layout.instanceSize(), 1 * kPointerSize);

  Type cls(&scope, layout.describedType());
  EXPECT_TRUE(isStrEqualsCStr(cls.name(), "MyTypeWithNoInitMethod"));
}

TEST(RuntimeTypeCallTest, TypeCallEmptyInitMethod) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class MyTypeWithEmptyInitMethod():
  def __init__(self):
    pass
  def m(self):
    pass

c = MyTypeWithEmptyInitMethod()
)";
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());

  Module main(&scope, findModule(&runtime, "__main__"));
  Object instance(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(instance.isInstance());
  LayoutId layout_id = instance.layoutId();
  Layout layout(&scope, runtime.layoutAt(layout_id));
  EXPECT_EQ(layout.instanceSize(), 1 * kPointerSize);

  Type cls(&scope, layout.describedType());
  EXPECT_TRUE(isStrEqualsCStr(cls.name(), "MyTypeWithEmptyInitMethod"));
}

TEST(RuntimeTypeCallTest, TypeCallWithArguments) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class MyTypeWithAttributes():
  def __init__(self, x):
    self.x = x
  def m(self):
    pass

c = MyTypeWithAttributes(1)
)";
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());

  Module main(&scope, findModule(&runtime, "__main__"));
  Type type(&scope, moduleAt(&runtime, main, "MyTypeWithAttributes"));
  Object instance(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(instance.isInstance());
  LayoutId layout_id = instance.layoutId();
  // Since this class has extra attributes, its layout id should be greater than
  // the layout id from the type.
  ASSERT_GT(layout_id, Layout::cast(type.instanceLayout()).id());
  Layout layout(&scope, runtime.layoutAt(layout_id));
  ASSERT_EQ(layout.instanceSize(), 2 * kPointerSize);

  Type cls(&scope, layout.describedType());
  EXPECT_TRUE(isStrEqualsCStr(cls.name(), "MyTypeWithAttributes"));

  Object name(&scope, runtime.newStrFromCStr("x"));
  Object value(&scope, runtime.attributeAt(Thread::current(), instance, name));
  EXPECT_FALSE(value.isError());
  EXPECT_EQ(*value, SmallInt::fromWord(1));
}

TEST(RuntimeTest, ComputeLineNumberForBytecodeOffset) {
  Runtime runtime;
  const char* src = R"(
def func():
  a = 1
  b = 2
  print(a, b)
)";
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  HandleScope scope;
  Object dunder_main(&scope, runtime.symbols()->DunderMain());
  Module main(&scope, runtime.findModule(dunder_main));

  // The bytecode for func is roughly:
  // LOAD_CONST     # a = 1
  // STORE_FAST
  //
  // LOAD_CONST     # b = 2
  // STORE_FAST
  //
  // LOAD_GLOBAL    # print(a, b)
  // LOAD_FAST
  // LOAD_FAST
  // CALL_FUNCTION

  Object name(&scope, runtime.newStrFromCStr("func"));
  Function func(&scope, runtime.moduleAt(main, name));
  Code code(&scope, func.code());
  ASSERT_EQ(code.firstlineno(), 2);

  // a = 1
  Thread* thread = Thread::current();
  EXPECT_EQ(runtime.codeOffsetToLineNum(thread, code, 0), 3);
  EXPECT_EQ(runtime.codeOffsetToLineNum(thread, code, 2), 3);

  // b = 2
  EXPECT_EQ(runtime.codeOffsetToLineNum(thread, code, 4), 4);
  EXPECT_EQ(runtime.codeOffsetToLineNum(thread, code, 6), 4);

  // print(a, b)
  for (word i = 8; i < Bytes::cast(code.code()).length(); i++) {
    EXPECT_EQ(runtime.codeOffsetToLineNum(thread, code, i), 5);
  }
}

TEST(RuntimeTest, IsInstanceOf) {
  Runtime runtime;
  HandleScope scope;
  EXPECT_FALSE(runtime.isInstanceOfInt(NoneType::object()));

  Object i(&scope, runtime.newInt(123));
  EXPECT_TRUE(i.isInt());
  EXPECT_FALSE(runtime.isInstanceOfStr(*i));

  Object str(&scope, runtime.newStrFromCStr("this is a long string"));
  EXPECT_TRUE(runtime.isInstanceOfStr(*str));
  EXPECT_FALSE(str.isInt());

  ASSERT_FALSE(runFromCStr(&runtime, R"(
class StopIterationSub(StopIteration):
  pass
stop_iteration = StopIterationSub()
  )")
                   .isError());
  Object stop_iteration(&scope,
                        moduleAt(&runtime, "__main__", "stop_iteration"));
  EXPECT_TRUE(runtime.isInstanceOfStopIteration(*stop_iteration));
  EXPECT_TRUE(runtime.isInstanceOfBaseException(*stop_iteration));
  EXPECT_FALSE(runtime.isInstanceOfSystemExit(*stop_iteration));
}

TEST(RuntimeTupleTest, Create) {
  Runtime runtime;

  RawObject obj0 = runtime.newTuple(0);
  ASSERT_TRUE(obj0.isTuple());
  RawTuple array0 = Tuple::cast(obj0);
  EXPECT_EQ(array0.length(), 0);

  RawObject obj1 = runtime.newTuple(1);
  ASSERT_TRUE(obj1.isTuple());
  RawTuple array1 = Tuple::cast(obj1);
  EXPECT_EQ(array1.length(), 1);

  RawObject obj7 = runtime.newTuple(7);
  ASSERT_TRUE(obj7.isTuple());
  RawTuple array7 = Tuple::cast(obj7);
  EXPECT_EQ(array7.length(), 7);

  RawObject obj8 = runtime.newTuple(8);
  ASSERT_TRUE(obj8.isTuple());
  RawTuple array8 = Tuple::cast(obj8);
  EXPECT_EQ(array8.length(), 8);
}

TEST(RuntimeSetTest, EmptySetInvariants) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());

  EXPECT_EQ(set.numItems(), 0);
  ASSERT_TRUE(set.isSet());
  ASSERT_TRUE(set.data().isTuple());
  EXPECT_EQ(Tuple::cast(set.data()).length(), 0);
}

TEST(RuntimeSetTest, Add) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Object value(&scope, SmallInt::fromWord(12345));

  // Store a value
  runtime.setAdd(thread, set, value);
  EXPECT_EQ(set.numItems(), 1);

  // Retrieve the stored value
  ASSERT_TRUE(runtime.setIncludes(thread, set, value));

  // Add a new value
  Object new_value(&scope, SmallInt::fromWord(5555));
  runtime.setAdd(thread, set, new_value);
  EXPECT_EQ(set.numItems(), 2);

  // Get the new value
  ASSERT_TRUE(runtime.setIncludes(thread, set, new_value));

  // Add a existing value
  Object same_value(&scope, SmallInt::fromWord(12345));
  RawObject old_value = runtime.setAdd(thread, set, same_value);
  EXPECT_EQ(set.numItems(), 2);
  EXPECT_EQ(old_value, *value);
}

TEST(RuntimeSetTest, Remove) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Object value(&scope, SmallInt::fromWord(12345));

  // Removing a key that doesn't exist should fail
  EXPECT_FALSE(runtime.setRemove(thread, set, value));

  runtime.setAdd(thread, set, value);
  EXPECT_EQ(set.numItems(), 1);

  ASSERT_TRUE(runtime.setRemove(thread, set, value));
  EXPECT_EQ(set.numItems(), 0);

  // Looking up a key that was deleted should fail
  ASSERT_FALSE(runtime.setIncludes(thread, set, value));
}

TEST(RuntimeSetTest, Grow) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());

  // Fill up the dict - we insert an initial key to force the allocation of the
  // backing Tuple.
  Object init_key(&scope, SmallInt::fromWord(0));
  runtime.setAdd(thread, set, init_key);
  ASSERT_TRUE(set.data().isTuple());
  word init_data_size = Tuple::cast(set.data()).length();

  auto make_key = [&runtime](int i) {
    byte text[]{"0123456789abcdeghiklmn"};
    return runtime.newStrWithAll(View<byte>(text + i % 10, 10));
  };

  // Fill in one fewer keys than would require growing the underlying object
  // array again
  word num_keys = Runtime::kInitialSetCapacity;
  for (int i = 1; i < num_keys; i++) {
    Object key(&scope, make_key(i));
    runtime.setAdd(thread, set, key);
  }

  // Add another key which should force us to double the capacity
  Object straw(&scope, make_key(num_keys));
  runtime.setAdd(thread, set, straw);
  ASSERT_TRUE(set.data().isTuple());
  word new_data_size = Tuple::cast(set.data()).length();
  EXPECT_EQ(new_data_size, Runtime::kSetGrowthFactor * init_data_size);

  // Make sure we can still read all the stored keys
  for (int i = 1; i <= num_keys; i++) {
    Object key(&scope, make_key(i));
    bool found = runtime.setIncludes(thread, set, key);
    ASSERT_TRUE(found);
  }
}

TEST(RuntimeSetTest, UpdateSet) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Set set1(&scope, runtime.newSet());
  Object set1_handle(&scope, *set1);
  for (word i = 0; i < 8; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(thread, set, value);
  }
  runtime.setUpdate(Thread::current(), set, set1_handle);
  ASSERT_EQ(set.numItems(), 8);
  for (word i = 4; i < 12; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(thread, set1, value);
  }
  runtime.setUpdate(Thread::current(), set, set1_handle);
  ASSERT_EQ(set.numItems(), 12);
  runtime.setUpdate(Thread::current(), set, set1_handle);
  ASSERT_EQ(set.numItems(), 12);
}

TEST(RuntimeSetTest, UpdateList) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list(&scope, runtime.newList());
  Set set(&scope, runtime.newSet());
  for (word i = 0; i < 8; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.listAdd(thread, list, value);
  }
  for (word i = 4; i < 12; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(thread, set, value);
  }
  ASSERT_EQ(set.numItems(), 8);
  Object list_handle(&scope, *list);
  runtime.setUpdate(Thread::current(), set, list_handle);
  ASSERT_EQ(set.numItems(), 12);
  runtime.setUpdate(Thread::current(), set, list_handle);
  ASSERT_EQ(set.numItems(), 12);
}

TEST(RuntimeSetTest, UpdateListIterator) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list(&scope, runtime.newList());
  Set set(&scope, runtime.newSet());
  for (word i = 0; i < 8; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.listAdd(thread, list, value);
  }
  for (word i = 4; i < 12; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(thread, set, value);
  }
  ASSERT_EQ(set.numItems(), 8);
  Object list_handle(&scope, *list);
  Object list_iterator(&scope, runtime.newListIterator(list_handle));
  runtime.setUpdate(Thread::current(), set, list_iterator);
  ASSERT_EQ(set.numItems(), 12);
}

TEST(RuntimeSetTest, UpdateTuple) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple object_array(&scope, runtime.newTuple(8));
  Set set(&scope, runtime.newSet());
  for (word i = 0; i < 8; i++) {
    object_array.atPut(i, SmallInt::fromWord(i));
  }
  for (word i = 4; i < 12; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(thread, set, value);
  }
  ASSERT_EQ(set.numItems(), 8);
  Object object_array_handle(&scope, *object_array);
  runtime.setUpdate(Thread::current(), set, object_array_handle);
  ASSERT_EQ(set.numItems(), 12);
}

TEST(RuntimeSetTest, UpdateIterator) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Object iterable(&scope, runtime.newRange(1, 4, 1));
  runtime.setUpdate(Thread::current(), set, iterable);

  ASSERT_EQ(set.numItems(), 3);
}

TEST(RuntimeSetTest, UpdateWithNonIterable) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Object non_iterable(&scope, NoneType::object());
  Object result(&scope,
                runtime.setUpdate(Thread::current(), set, non_iterable));
  ASSERT_TRUE(result.isError());
}

TEST(RuntimeSetTest, EmptySetItersectionReturnsEmptySet) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Set set1(&scope, runtime.newSet());

  // set() & set()
  Object result(&scope, runtime.setIntersection(thread, set, set1));
  ASSERT_TRUE(result.isSet());
  EXPECT_EQ(Set::cast(*result).numItems(), 0);
}

TEST(RuntimeSetTest, ItersectionWithEmptySetReturnsEmptySet) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Set set1(&scope, runtime.newSet());

  for (word i = 0; i < 8; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(thread, set1, value);
  }

  // set() & {0, 1, 2, 3, 4, 5, 6, 7}
  Object result(&scope, runtime.setIntersection(thread, set, set1));
  ASSERT_TRUE(result.isSet());
  EXPECT_EQ(Set::cast(*result).numItems(), 0);

  // {0, 1, 2, 3, 4, 5, 6, 7} & set()
  Object result1(&scope, runtime.setIntersection(thread, set1, set));
  ASSERT_TRUE(result1.isSet());
  EXPECT_EQ(Set::cast(*result1).numItems(), 0);
}

TEST(RuntimeSetTest, IntersectionReturnsSetWithCommonElements) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Set set1(&scope, runtime.newSet());
  Object key(&scope, NoneType::object());

  for (word i = 0; i < 8; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(thread, set1, value);
  }

  for (word i = 0; i < 4; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(thread, set, value);
  }

  // {0, 1, 2, 3} & {0, 1, 2, 3, 4, 5, 6, 7}
  Set result(&scope, runtime.setIntersection(thread, set, set1));
  EXPECT_EQ(Set::cast(*result).numItems(), 4);
  key = SmallInt::fromWord(0);
  EXPECT_TRUE(runtime.setIncludes(thread, result, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(runtime.setIncludes(thread, result, key));
  key = SmallInt::fromWord(2);
  EXPECT_TRUE(runtime.setIncludes(thread, result, key));
  key = SmallInt::fromWord(3);
  EXPECT_TRUE(runtime.setIncludes(thread, result, key));

  // {0, 1, 2, 3, 4, 5, 6, 7} & {0, 1, 2, 3}
  Set result1(&scope, runtime.setIntersection(thread, set, set1));
  EXPECT_EQ(Set::cast(*result1).numItems(), 4);
  key = SmallInt::fromWord(0);
  EXPECT_TRUE(runtime.setIncludes(thread, result1, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(runtime.setIncludes(thread, result1, key));
  key = SmallInt::fromWord(2);
  EXPECT_TRUE(runtime.setIncludes(thread, result1, key));
  key = SmallInt::fromWord(3);
  EXPECT_TRUE(runtime.setIncludes(thread, result1, key));
}

TEST(RuntimeSetTest, IntersectIterator) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Object iterable(&scope, runtime.newRange(1, 4, 1));
  Set result(&scope, runtime.setIntersection(thread, set, iterable));
  EXPECT_EQ(result.numItems(), 0);

  Object key(&scope, SmallInt::fromWord(1));
  runtime.setAdd(thread, set, key);
  key = SmallInt::fromWord(2);
  runtime.setAdd(thread, set, key);
  Object iterable1(&scope, runtime.newRange(1, 4, 1));
  Set result1(&scope, runtime.setIntersection(thread, set, iterable1));
  EXPECT_EQ(result1.numItems(), 2);
  EXPECT_TRUE(runtime.setIncludes(thread, result1, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(runtime.setIncludes(thread, result1, key));
}

TEST(RuntimeSetTest, IntersectWithNonIterable) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Object non_iterable(&scope, NoneType::object());

  Object result(&scope, runtime.setIntersection(thread, set, non_iterable));
  ASSERT_TRUE(result.isError());
}

// Attribute tests

// Set an attribute defined in __init__
TEST(InstanceAttributeTest, SetInstanceAttribute) {
  Runtime runtime;
  const char* src = R"(
class Foo:
  def __init__(self):
    self.attr = 'testing 123'

def test(x):
  Foo.__init__(x)
  print(x.attr)
  x.attr = '321 testing'
  print(x.attr)
)";
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());

  // Create the instance
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Type type(&scope, moduleAt(&runtime, main, "Foo"));
  Tuple args(&scope, runtime.newTuple(1));
  Layout layout(&scope, type.instanceLayout());
  args.atPut(0, runtime.newInstance(layout));

  // Run __init__ then RMW the attribute
  Function test(&scope, moduleAt(&runtime, main, "test"));
  EXPECT_EQ(callFunctionToString(test, args), "testing 123\n321 testing\n");
}

TEST(InstanceAttributeTest, AddOverflowAttributes) {
  Runtime runtime;
  const char* src = R"(
class Foo:
  pass

def test(x):
  x.foo = 100
  x.bar = 200
  x.baz = 'hello'
  print(x.foo, x.bar, x.baz)

  x.foo = 'aaa'
  x.bar = 'bbb'
  x.baz = 'ccc'
  print(x.foo, x.bar, x.baz)
)";
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());

  // Create an instance of Foo
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Type type(&scope, moduleAt(&runtime, main, "Foo"));
  Layout layout(&scope, type.instanceLayout());
  Instance foo1(&scope, runtime.newInstance(layout));
  LayoutId original_layout_id = layout.id();

  // Add overflow attributes that should force layout transitions
  Tuple args(&scope, runtime.newTuple(1));
  args.atPut(0, *foo1);
  Function test(&scope, moduleAt(&runtime, main, "test"));
  EXPECT_EQ(callFunctionToString(test, args), "100 200 hello\naaa bbb ccc\n");
  EXPECT_NE(foo1.layoutId(), original_layout_id);

  // Add the same set of attributes to a new instance, should arrive at the
  // same layout
  Instance foo2(&scope, runtime.newInstance(layout));
  args.atPut(0, *foo2);
  EXPECT_EQ(callFunctionToString(test, args), "100 200 hello\naaa bbb ccc\n");
}

TEST(InstanceAttributeTest, ManipulateMultipleAttributes) {
  Runtime runtime;
  const char* src = R"(
class Foo:
  def __init__(self):
    self.foo = 'foo'
    self.bar = 'bar'
    self.baz = 'baz'

def test(x):
  Foo.__init__(x)
  print(x.foo, x.bar, x.baz)
  x.foo = 'aaa'
  x.bar = 'bbb'
  x.baz = 'ccc'
  print(x.foo, x.bar, x.baz)
)";
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());

  // Create the instance
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Type type(&scope, moduleAt(&runtime, main, "Foo"));
  Tuple args(&scope, runtime.newTuple(1));
  Layout layout(&scope, type.instanceLayout());
  args.atPut(0, runtime.newInstance(layout));

  // Run the test
  Function test(&scope, moduleAt(&runtime, main, "test"));
  EXPECT_EQ(callFunctionToString(test, args), "foo bar baz\naaa bbb ccc\n");
}

TEST(InstanceAttributeTest, FetchConditionalInstanceAttribute) {
  Runtime runtime;
  const char* src = R"(
def false():
  return False

class Foo:
  def __init__(self):
    self.foo = 'foo'
    if false():
      self.bar = 'bar'

foo = Foo()
print(foo.bar)
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src),
                            LayoutId::kAttributeError,
                            "'Foo' object has no attribute 'bar'"));
}

TEST(InstanceAttributeTest, DunderClass) {
  Runtime runtime;
  const char* src = R"(
class Foo: pass
class Bar(Foo): pass
class Hello(Bar, list): pass
print(list().__class__ is list)
print(Foo().__class__ is Foo)
print(Bar().__class__ is Bar)
print(Hello().__class__ is Hello)
print(Foo.__class__ is type)
print(super(Bar, Bar()).__class__ is super)
)";
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "True\nTrue\nTrue\nTrue\nTrue\nTrue\n");
}

TEST(InstanceAttributeTest, DunderNew) {
  Runtime runtime;
  const char* src = R"(
class Foo:
    def __new__(cls):
        print("New")
        return object.__new__(cls)
    def __init__(self):
        print("Init")
a = Foo()
)";
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "New\nInit\n");
}

TEST(InstanceAttributeTest, NoInstanceDictReturnsClassAttribute) {
  Runtime runtime;
  HandleScope scope;
  Object immediate(&scope, SmallInt::fromWord(-1));
  Object name(&scope, runtime.symbols()->DunderNeg());
  RawObject attr = runtime.attributeAt(Thread::current(), immediate, name);
  ASSERT_TRUE(attr.isBoundMethod());
}

TEST(InstanceAttributeDeletionTest, DeleteKnownAttribute) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
class Foo:
    def __init__(self):
      self.foo = 'foo'
      self.bar = 'bar'

def test():
    foo = Foo()
    del foo.bar
)";
  compileAndRunToString(&runtime, src);

  Module main(&scope, findModule(&runtime, "__main__"));
  Function test(&scope, moduleAt(&runtime, main, "test"));
  Tuple args(&scope, runtime.newTuple(0));
  Object result(&scope, callFunction(test, args));
  EXPECT_EQ(*result, NoneType::object());
}

TEST(InstanceAttributeDeletionTest, DeleteDescriptor) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
result = None

class DeleteDescriptor:
    def __delete__(self, instance):
        global result
        result = self, instance
descr = DeleteDescriptor()

class Foo:
    bar = descr

foo = Foo()
del foo.bar
)";
  compileAndRunToString(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object data(&scope, moduleAt(&runtime, main, "result"));
  ASSERT_TRUE(data.isTuple());

  Tuple result(&scope, *data);
  ASSERT_EQ(result.length(), 2);

  Object descr(&scope, moduleAt(&runtime, main, "descr"));
  EXPECT_EQ(result.at(0), *descr);

  Object foo(&scope, moduleAt(&runtime, main, "foo"));
  EXPECT_EQ(result.at(1), *foo);
}

TEST(InstanceAttributeDeletionTest, DeleteUnknownAttribute) {
  Runtime runtime;
  const char* src = R"(
class Foo:
    pass

foo = Foo()
del foo.bar
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src),
                            LayoutId::kAttributeError,
                            "'Foo' object has no attribute 'bar'"));
}

TEST(InstanceAttributeDeletionTest, DeleteAttributeWithDunderDelattr) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
result = None

class Foo:
    def __delattr__(self, name):
        global result
        result = self, name

foo = Foo()
del foo.bar
)";
  compileAndRunToString(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object data(&scope, moduleAt(&runtime, main, "result"));
  ASSERT_TRUE(data.isTuple());

  Tuple result(&scope, *data);
  ASSERT_EQ(result.length(), 2);

  Object foo(&scope, moduleAt(&runtime, main, "foo"));
  EXPECT_EQ(result.at(0), *foo);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "bar"));
}

TEST(InstanceAttributeDeletionTest,
     DeleteAttributeWithDunderDelattrOnSuperclass) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
result = None

class Foo:
    def __delattr__(self, name):
        global result
        result = self, name

class Bar(Foo):
    pass

bar = Bar()
del bar.baz
)";
  compileAndRunToString(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object data(&scope, moduleAt(&runtime, main, "result"));
  ASSERT_TRUE(data.isTuple());

  Tuple result(&scope, *data);
  ASSERT_EQ(result.length(), 2);

  Object bar(&scope, moduleAt(&runtime, main, "bar"));
  EXPECT_EQ(result.at(0), *bar);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "baz"));
}

TEST(ClassAttributeDeletionTest, DeleteKnownAttribute) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
class Foo:
    foo = 'foo'
    bar = 'bar'

def test():
    del Foo.bar
)";
  compileAndRunToString(&runtime, src);

  Module main(&scope, findModule(&runtime, "__main__"));
  Function test(&scope, moduleAt(&runtime, main, "test"));
  Tuple args(&scope, runtime.newTuple(0));
  Object result(&scope, callFunction(test, args));
  EXPECT_EQ(*result, NoneType::object());
}

TEST(ClassAttributeDeletionTest, DeleteDescriptorOnMetaclass) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
args = None

class DeleteDescriptor:
    def __delete__(self, instance):
        global args
        args = (self, instance)

descr = DeleteDescriptor()

class FooMeta(type):
    attr = descr

class Foo(metaclass=FooMeta):
    pass

del Foo.attr
)";
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object data(&scope, moduleAt(&runtime, main, "args"));
  ASSERT_TRUE(data.isTuple());

  Tuple args(&scope, *data);
  ASSERT_EQ(args.length(), 2);

  Object descr(&scope, moduleAt(&runtime, main, "descr"));
  EXPECT_EQ(args.at(0), *descr);

  Object foo(&scope, moduleAt(&runtime, main, "Foo"));
  EXPECT_EQ(args.at(1), *foo);
}

TEST(ClassAttributeDeletionTest, DeleteUnknownAttribute) {
  Runtime runtime;
  const char* src = R"(
class Foo:
    pass

del Foo.bar
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src),
                            LayoutId::kAttributeError,
                            "type object 'Foo' has no attribute 'bar'"));
}

TEST(ClassAttributeDeletionTest, DeleteAttributeWithDunderDelattrOnMetaclass) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
args = None

class FooMeta(type):
    def __delattr__(self, name):
        global args
        args = self, name

class Foo(metaclass=FooMeta):
    pass

del Foo.bar
)";
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object data(&scope, moduleAt(&runtime, main, "args"));
  ASSERT_TRUE(data.isTuple());

  Tuple args(&scope, *data);
  ASSERT_EQ(args.length(), 2);

  Object foo(&scope, moduleAt(&runtime, main, "Foo"));
  EXPECT_EQ(args.at(0), *foo);

  Object attr(&scope, runtime.internStrFromCStr("bar"));
  EXPECT_EQ(args.at(1), *attr);
}

TEST(ModuleAttributeDeletionTest, DeleteUnknownAttribute) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
def test(module):
    del module.foo
)";
  compileAndRunToString(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Function test(&scope, moduleAt(&runtime, main, "test"));
  Tuple args(&scope, runtime.newTuple(1));
  args.atPut(0, *main);
  EXPECT_TRUE(raised(callFunction(test, args), LayoutId::kAttributeError));
}

TEST(ModuleAttributeDeletionTest, DeleteKnownAttribute) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
foo = 'testing 123'

def test(module):
    del module.foo
    return 123
)";
  compileAndRunToString(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Function test(&scope, moduleAt(&runtime, main, "test"));
  Tuple args(&scope, runtime.newTuple(1));
  args.atPut(0, *main);
  EXPECT_EQ(callFunction(test, args), SmallInt::fromWord(123));

  Object attr(&scope, runtime.newStrFromCStr("foo"));
  Object module(&scope, *main);
  EXPECT_TRUE(runtime.attributeAt(Thread::current(), module, attr).isError());
}

TEST(RuntimeIntTest, NewSmallIntWithDigits) {
  Runtime runtime;
  HandleScope scope;

  Int zero(&scope, runtime.newIntWithDigits(View<uword>(nullptr, 0)));
  EXPECT_TRUE(isIntEqualsWord(*zero, 0));

  uword digit = 1;
  RawObject one = runtime.newIntWithDigits(View<uword>(&digit, 1));
  EXPECT_TRUE(isIntEqualsWord(one, 1));

  digit = kMaxUword;
  RawObject negative_one = runtime.newIntWithDigits(View<uword>(&digit, 1));
  EXPECT_TRUE(isIntEqualsWord(negative_one, -1));

  word min_small_int = RawSmallInt::kMaxValue;
  digit = static_cast<uword>(min_small_int);
  Int min_smallint(&scope, runtime.newIntWithDigits(View<uword>(&digit, 1)));
  EXPECT_TRUE(isIntEqualsWord(*min_smallint, min_small_int));

  word max_small_int = RawSmallInt::kMaxValue;
  digit = static_cast<uword>(max_small_int);
  Int max_smallint(&scope, runtime.newIntWithDigits(View<uword>(&digit, 1)));
  EXPECT_TRUE(isIntEqualsWord(*max_smallint, max_small_int));
}

TEST(RuntimeIntTest, NewLargeIntWithDigits) {
  Runtime runtime;
  HandleScope scope;

  word negative_large_int = RawSmallInt::kMinValue - 1;
  uword digit = static_cast<uword>(negative_large_int);
  Int negative_largeint(&scope,
                        runtime.newIntWithDigits(View<uword>(&digit, 1)));
  EXPECT_TRUE(isIntEqualsWord(*negative_largeint, negative_large_int));

  word positive_large_int = RawSmallInt::kMaxValue + 1;
  digit = static_cast<uword>(positive_large_int);
  Int positive_largeint(&scope,
                        runtime.newIntWithDigits(View<uword>(&digit, 1)));
  EXPECT_TRUE(isIntEqualsWord(*positive_largeint, positive_large_int));
}

TEST(RuntimeIntTest, BinaryAndWithSmallInts) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, SmallInt::fromWord(0xEA));   // 0b11101010
  Int right(&scope, SmallInt::fromWord(0xDC));  // 0b11011100
  Object result(&scope, runtime.intBinaryAnd(Thread::current(), left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xC8));  // 0b11001000
}

TEST(RuntimeIntTest, BinaryAndWithLargeInts) {
  Runtime runtime;
  HandleScope scope;
  // {0b00001111, 0b00110000, 0b00000001}
  const uword digits_left[] = {0x0F, 0x30, 0x1};
  Int left(&scope, newIntWithDigits(&runtime, digits_left));
  // {0b00000011, 0b11110000, 0b00000010, 0b00000111}
  const uword digits_right[] = {0x03, 0xF0, 0x2, 0x7};
  Int right(&scope, newIntWithDigits(&runtime, digits_right));
  Object result(&scope, runtime.intBinaryAnd(Thread::current(), left, right));
  // {0b00000111, 0b01110000}
  const uword expected_digits[] = {0x03, 0x30};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));

  Object result_commuted(&scope,
                         runtime.intBinaryAnd(Thread::current(), right, left));
  EXPECT_TRUE(isIntEqualsDigits(*result_commuted, expected_digits));
}

TEST(RuntimeIntTest, BinaryAndWithNegativeLargeInts) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, SmallInt::fromWord(-42));  // 0b11010110
  const uword digits[] = {static_cast<uword>(-1), 0xF0, 0x2, 0x7};
  Int right(&scope, newIntWithDigits(&runtime, digits));
  Object result(&scope, runtime.intBinaryAnd(Thread::current(), left, right));
  const uword expected_digits[] = {static_cast<uword>(-42), 0xF0, 0x2, 0x7};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST(RuntimeIntTest, BinaryOrWithSmallInts) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, SmallInt::fromWord(0xAA));   // 0b10101010
  Int right(&scope, SmallInt::fromWord(0x9C));  // 0b10011100
  Object result(&scope, runtime.intBinaryOr(Thread::current(), left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xBE));  // 0b10111110
}

TEST(RuntimeIntTest, BinaryOrWithLargeInts) {
  Runtime runtime;
  HandleScope scope;
  // {0b00001100, 0b00110000, 0b00000001}
  const uword digits_left[] = {0x0C, 0x30, 0x1};
  Int left(&scope, newIntWithDigits(&runtime, digits_left));
  // {0b00000011, 0b11010000, 0b00000010, 0b00000111}
  const uword digits_right[] = {0x03, 0xD0, 0x2, 0x7};
  Int right(&scope, newIntWithDigits(&runtime, digits_right));
  Object result(&scope, runtime.intBinaryOr(Thread::current(), left, right));
  // {0b00001111, 0b11110000, 0b00000011, 0b00000111}
  const uword expected_digits[] = {0x0F, 0xF0, 0x3, 0x7};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));

  Object result_commuted(&scope,
                         runtime.intBinaryOr(Thread::current(), right, left));
  EXPECT_TRUE(isIntEqualsDigits(*result_commuted, expected_digits));
}

TEST(RuntimeIntTest, BinaryOrWithNegativeLargeInts) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, SmallInt::fromWord(-42));  // 0b11010110
  const uword digits[] = {static_cast<uword>(-4), 0xF0, 0x2,
                          static_cast<uword>(-1)};
  Int right(&scope, newIntWithDigits(&runtime, digits));
  Object result(&scope, runtime.intBinaryOr(Thread::current(), left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, -2));
}

TEST(RuntimeIntTest, BinaryXorWithSmallInts) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, SmallInt::fromWord(0xAA));   // 0b10101010
  Int right(&scope, SmallInt::fromWord(0x9C));  // 0b10011100
  Object result(&scope, runtime.intBinaryXor(Thread::current(), left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0x36));  // 0b00110110
}

TEST(RuntimeIntTest, BinaryXorWithLargeInts) {
  Runtime runtime;
  HandleScope scope;
  // {0b00001100, 0b00110000, 0b00000001}
  const uword digits_left[] = {0x0C, 0x30, 0x1};
  Int left(&scope, newIntWithDigits(&runtime, digits_left));
  // {0b00000011, 0b11010000, 0b00000010, 0b00000111}
  const uword digits_right[] = {0x03, 0xD0, 0x2, 0x7};
  Int right(&scope, newIntWithDigits(&runtime, digits_right));
  Object result(&scope, runtime.intBinaryXor(Thread::current(), left, right));
  // {0b00001111, 0b11100000, 0b00000011, 0b00000111}
  const uword expected_digits[] = {0x0F, 0xE0, 0x3, 0x7};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));

  Object result_commuted(&scope,
                         runtime.intBinaryXor(Thread::current(), right, left));
  EXPECT_TRUE(isIntEqualsDigits(*result_commuted, expected_digits));
}

TEST(RuntimeIntTest, BinaryXorWithNegativeLargeInts) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, SmallInt::fromWord(-42));  // 0b11010110
  const uword digits[] = {static_cast<uword>(-1), 0xf0, 0x2,
                          static_cast<uword>(-1)};
  Int right(&scope, newIntWithDigits(&runtime, digits));
  Object result(&scope, runtime.intBinaryXor(Thread::current(), left, right));
  const uword expected_digits[] = {0x29, ~static_cast<uword>(0xF0),
                                   ~static_cast<uword>(0x2), 0};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST(RuntimeIntTest, NormalizeLargeIntToSmallInt) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  const uword digits[] = {42};
  LargeInt lint_42(&scope, newLargeIntWithDigits(digits));
  Object norm_42(&scope, runtime.normalizeLargeInt(thread, lint_42));
  EXPECT_TRUE(isIntEqualsWord(*norm_42, 42));

  const uword digits2[] = {uword(-1)};
  LargeInt lint_neg1(&scope, newLargeIntWithDigits(digits2));
  Object norm_neg1(&scope, runtime.normalizeLargeInt(thread, lint_neg1));
  EXPECT_TRUE(isIntEqualsWord(*norm_neg1, -1));

  const uword digits3[] = {uword(RawSmallInt::kMinValue)};
  LargeInt lint_min(&scope, newLargeIntWithDigits(digits3));
  Object norm_min(&scope, runtime.normalizeLargeInt(thread, lint_min));
  EXPECT_TRUE(isIntEqualsWord(*norm_min, RawSmallInt::kMinValue));

  const uword digits4[] = {RawSmallInt::kMaxValue};
  LargeInt lint_max(&scope, newLargeIntWithDigits(digits4));
  Object norm_max(&scope, runtime.normalizeLargeInt(thread, lint_max));
  EXPECT_TRUE(isIntEqualsWord(*norm_max, RawSmallInt::kMaxValue));

  const uword digits5[] = {uword(-4), kMaxUword};
  LargeInt lint_sext_neg_4(&scope, newLargeIntWithDigits(digits5));
  Object norm_neg_4(&scope, runtime.normalizeLargeInt(thread, lint_sext_neg_4));
  EXPECT_TRUE(isIntEqualsWord(*norm_neg_4, -4));

  const uword digits6[] = {uword(-13), kMaxUword, kMaxUword, kMaxUword};
  LargeInt lint_sext_neg_13(&scope, newLargeIntWithDigits(digits6));
  Object norm_neg_13(&scope,
                     runtime.normalizeLargeInt(thread, lint_sext_neg_13));
  EXPECT_TRUE(isIntEqualsWord(*norm_neg_13, -13));

  const uword digits7[] = {66, 0};
  LargeInt lint_zext_66(&scope, newLargeIntWithDigits(digits7));
  Object norm_66(&scope, runtime.normalizeLargeInt(thread, lint_zext_66));
  EXPECT_TRUE(isIntEqualsWord(*norm_66, 66));
}

TEST(RuntimeIntTest, NormalizeLargeIntToLargeInt) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  const uword digits[] = {kMaxWord};
  LargeInt lint_max(&scope, newLargeIntWithDigits(digits));
  Object norm_max(&scope, runtime.normalizeLargeInt(thread, lint_max));
  EXPECT_TRUE(isIntEqualsWord(*norm_max, kMaxWord));

  const uword digits2[] = {uword(kMinWord)};
  LargeInt lint_min(&scope, newLargeIntWithDigits(digits2));
  Object norm_min(&scope, runtime.normalizeLargeInt(thread, lint_min));
  EXPECT_TRUE(isIntEqualsWord(*norm_min, kMinWord));

  const uword digits3[] = {kMaxWord - 7, 0, 0};
  LargeInt lint_max_sub_7_zext(&scope, newLargeIntWithDigits(digits3));
  Object norm_max_sub_7(&scope,
                        runtime.normalizeLargeInt(thread, lint_max_sub_7_zext));
  EXPECT_TRUE(isIntEqualsWord(*norm_max_sub_7, kMaxWord - 7));

  const uword digits4[] = {uword(kMinWord) + 9, kMaxUword};
  LargeInt lint_min_plus_9_sext(&scope, newLargeIntWithDigits(digits4));
  Object norm_min_plus_9(
      &scope, runtime.normalizeLargeInt(thread, lint_min_plus_9_sext));
  EXPECT_TRUE(isIntEqualsWord(*norm_min_plus_9, kMinWord + 9));

  const uword digits5[] = {0, kMaxUword};
  LargeInt lint_no_sext(&scope, newLargeIntWithDigits(digits5));
  Object norm_no_sext(&scope, runtime.normalizeLargeInt(thread, lint_no_sext));
  const uword expected_digits1[] = {0, kMaxUword};
  EXPECT_TRUE(isIntEqualsDigits(*norm_no_sext, expected_digits1));

  const uword digits6[] = {kMaxUword, 0};
  LargeInt lint_no_zext(&scope, newLargeIntWithDigits(digits6));
  Object norm_no_zext(&scope, runtime.normalizeLargeInt(thread, lint_no_zext));
  const uword expected_digits2[] = {kMaxUword, 0};
  EXPECT_TRUE(isIntEqualsDigits(*norm_no_zext, expected_digits2));
}

TEST(InstanceDelTest, DeleteUnknownAttribute) {
  const char* src = R"(
class Foo:
    pass
)";
  Runtime runtime;
  compileAndRunToString(&runtime, src);

  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Type type(&scope, moduleAt(&runtime, main, "Foo"));
  Layout layout(&scope, type.instanceLayout());
  HeapObject instance(&scope, runtime.newInstance(layout));
  Object attr(&scope, runtime.newStrFromCStr("unknown"));
  EXPECT_TRUE(runtime.instanceDel(Thread::current(), instance, attr).isError());
}

TEST(InstanceDelTest, DeleteInObjectAttribute) {
  const char* src = R"(
class Foo:
    def __init__(self):
        self.bar = 'bar'
        self.baz = 'baz'

def new_foo():
    return Foo()
)";
  Runtime runtime;
  compileAndRunToString(&runtime, src);

  // Create an instance of Foo
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Function new_foo(&scope, moduleAt(&runtime, main, "new_foo"));
  Tuple args(&scope, runtime.newTuple(0));
  HeapObject instance(&scope, callFunction(new_foo, args));

  // Verify that 'bar' is an in-object property
  Layout layout(&scope, runtime.layoutAt(instance.header().layoutId()));
  Object attr(&scope, runtime.internStrFromCStr("bar"));
  AttributeInfo info;
  Thread* thread = Thread::current();
  ASSERT_TRUE(runtime.layoutFindAttribute(thread, layout, attr, &info));
  ASSERT_TRUE(info.isInObject());

  // After successful deletion, the instance should have a new layout and should
  // no longer reference the previous value
  EXPECT_EQ(runtime.instanceDel(thread, instance, attr), NoneType::object());
  Layout new_layout(&scope, runtime.layoutAt(instance.header().layoutId()));
  EXPECT_NE(*layout, *new_layout);
  EXPECT_FALSE(runtime.layoutFindAttribute(thread, new_layout, attr, &info));
}

TEST(InstanceDelTest, DeleteOverflowAttribute) {
  const char* src = R"(
class Foo:
    pass

def new_foo():
    foo = Foo()
    foo.bar = 'bar'
    return foo
)";
  Runtime runtime;
  compileAndRunToString(&runtime, src);

  // Create an instance of Foo
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Function new_foo(&scope, moduleAt(&runtime, main, "new_foo"));
  Tuple args(&scope, runtime.newTuple(0));
  HeapObject instance(&scope, callFunction(new_foo, args));

  // Verify that 'bar' is an overflow property
  Layout layout(&scope, runtime.layoutAt(instance.header().layoutId()));
  Object attr(&scope, runtime.internStrFromCStr("bar"));
  AttributeInfo info;
  Thread* thread = Thread::current();
  ASSERT_TRUE(runtime.layoutFindAttribute(thread, layout, attr, &info));
  ASSERT_TRUE(info.isOverflow());

  // After successful deletion, the instance should have a new layout and should
  // no longer reference the previous value
  EXPECT_EQ(runtime.instanceDel(thread, instance, attr), NoneType::object());
  Layout new_layout(&scope, runtime.layoutAt(instance.header().layoutId()));
  EXPECT_NE(*layout, *new_layout);
  EXPECT_FALSE(runtime.layoutFindAttribute(thread, new_layout, attr, &info));
}

TEST(RuntimeTest, InstanceDelWithReadOnlyAttributeRaisesAttributeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  BuiltinAttribute attrs[] = {
      {SymbolId::kDunderGlobals, 0, AttributeFlags::kReadOnly},
      {SymbolId::kSentinelId, -1},
  };
  BuiltinMethod builtins[] = {
      {SymbolId::kSentinelId, nullptr},
  };
  LayoutId layout_id = runtime.reserveLayoutId(thread);
  Type type(&scope, runtime.addBuiltinType(SymbolId::kVersion, layout_id,
                                           LayoutId::kObject, attrs, builtins));
  Layout layout(&scope, type.instanceLayout());
  runtime.layoutAtPut(layout_id, *layout);
  HeapObject instance(&scope, runtime.newInstance(layout));
  Object attribute_name(&scope, runtime.newStrFromCStr("__globals__"));
  EXPECT_TRUE(raisedWithStr(
      runtime.instanceDel(thread, instance, attribute_name),
      LayoutId::kAttributeError, "'__globals__' attribute is read-only"));
}

TEST(MetaclassTest, ClassWithTypeMetaclassIsConcreteType) {
  const char* src = R"(
# This is equivalent to `class Foo(type)`
class Foo(type, metaclass=type):
    pass

class Bar(Foo):
    pass
)";
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  Module main(&scope, findModule(&runtime, "__main__"));

  Object foo(&scope, moduleAt(&runtime, main, "Foo"));
  EXPECT_TRUE(foo.isType());

  Object bar(&scope, moduleAt(&runtime, main, "Bar"));
  EXPECT_TRUE(bar.isType());
}

TEST(MetaclassTest, ClassWithCustomMetaclassIsntConcreteType) {
  const char* src = R"(
class MyMeta(type):
    pass

class Foo(type, metaclass=MyMeta):
    pass
)";
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  Module main(&scope, findModule(&runtime, "__main__"));

  Object foo(&scope, moduleAt(&runtime, main, "Foo"));
  EXPECT_FALSE(foo.isType());
}

TEST(MetaclassTest, ClassWithTypeMetaclassIsInstanceOfType) {
  const char* src = R"(
class Foo(type):
    pass

class Bar(Foo):
    pass
)";
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  Module main(&scope, findModule(&runtime, "__main__"));

  Object foo(&scope, moduleAt(&runtime, main, "Foo"));
  EXPECT_TRUE(runtime.isInstanceOfType(*foo));

  Object bar(&scope, moduleAt(&runtime, main, "Bar"));
  EXPECT_TRUE(runtime.isInstanceOfType(*bar));
}

TEST(MetaclassTest, ClassWithCustomMetaclassIsInstanceOfType) {
  const char* src = R"(
class MyMeta(type):
    pass

class Foo(type, metaclass=MyMeta):
    pass
)";
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object foo(&scope, moduleAt(&runtime, main, "Foo"));
  EXPECT_TRUE(runtime.isInstanceOfType(*foo));
}

TEST(MetaclassTest, VerifyMetaclassHierarchy) {
  const char* src = R"(
class GrandMeta(type):
    pass

class ParentMeta(type, metaclass=GrandMeta):
    pass

class ChildMeta(type, metaclass=ParentMeta):
    pass
)";
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object type(&scope, runtime.typeAt(LayoutId::kType));

  Object grand_meta(&scope, moduleAt(&runtime, main, "GrandMeta"));
  EXPECT_EQ(runtime.typeOf(*grand_meta), *type);

  Object parent_meta(&scope, moduleAt(&runtime, main, "ParentMeta"));
  EXPECT_EQ(runtime.typeOf(*parent_meta), *grand_meta);

  Object child_meta(&scope, moduleAt(&runtime, main, "ChildMeta"));
  EXPECT_EQ(runtime.typeOf(*child_meta), *parent_meta);
}

TEST(MetaclassTest, CallMetaclass) {
  const char* src = R"(
class MyMeta(type):
    pass

Foo = MyMeta('Foo', (), {})
)";
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object mymeta(&scope, moduleAt(&runtime, main, "MyMeta"));
  Object foo(&scope, moduleAt(&runtime, main, "Foo"));
  EXPECT_EQ(runtime.typeOf(*foo), *mymeta);
  EXPECT_FALSE(foo.isType());
  EXPECT_TRUE(runtime.isInstanceOfType(*foo));
}

TEST(SubclassingTest, SubclassBuiltinSubclass) {
  const char* src = R"(
class Test(Exception):
  pass
)";
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object value(&scope, moduleAt(&runtime, main, "Test"));
  ASSERT_TRUE(value.isType());

  Type type(&scope, *value);
  ASSERT_TRUE(type.mro().isTuple());

  Tuple mro(&scope, type.mro());
  ASSERT_EQ(mro.length(), 4);
  EXPECT_EQ(mro.at(0), *type);
  EXPECT_EQ(mro.at(1), runtime.typeAt(LayoutId::kException));
  EXPECT_EQ(mro.at(2), runtime.typeAt(LayoutId::kBaseException));
  EXPECT_EQ(mro.at(3), runtime.typeAt(LayoutId::kObject));
}

TEST(ModuleImportTest, ModuleImportsAllPublicSymbols) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  // Create Module
  Object name(&scope, runtime.newStrFromCStr("foo"));
  Module module(&scope, runtime.newModule(name));

  // Add symbols
  Dict module_dict(&scope, module.dict());
  Object symbol_str1(&scope, runtime.newStrFromCStr("public_symbol"));
  Object symbol_str2(&scope, runtime.newStrFromCStr("_private_symbol"));
  runtime.dictAtPutInValueCell(thread, module_dict, symbol_str1, symbol_str1);
  runtime.dictAtPutInValueCell(thread, module_dict, symbol_str2, symbol_str2);

  // Import public symbols to dictionary
  Dict symbols_dict(&scope, runtime.newDict());
  runtime.moduleImportAllFrom(symbols_dict, module);
  EXPECT_EQ(symbols_dict.numItems(), 1);

  ValueCell result(&scope, runtime.dictAt(thread, symbols_dict, symbol_str1));
  EXPECT_TRUE(isStrEqualsCStr(result.value(), "public_symbol"));
}

TEST(HeapFrameTest, Create) {
  const char* src = R"(
def gen():
  yield 12
)";

  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  Object gen(&scope, moduleAt(&runtime, "__main__", "gen"));
  ASSERT_TRUE(gen.isFunction());
  Code code(&scope, Function::cast(*gen).code());
  Object frame_obj(&scope, runtime.newHeapFrame(code));
  ASSERT_TRUE(frame_obj.isHeapFrame());
  HeapFrame heap_frame(&scope, *frame_obj);
  EXPECT_EQ(heap_frame.maxStackSize(), code.stacksize());
}

extern "C" struct _inittab _PyImport_Inittab[];

TEST(ModuleImportTest, ImportModuleFromInitTab) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "import _empty").isError());
  HandleScope scope;
  Object mod(&scope, moduleAt(&runtime, "__main__", "_empty"));
  EXPECT_TRUE(mod.isModule());
}

TEST(ModuleTest, NewModuleSetsDictValues) {
  Runtime runtime;
  HandleScope scope;

  // Create Module
  Object name(&scope, runtime.newStrFromCStr("mymodule"));
  Module module(&scope, runtime.newModule(name));

  Str mod_name(&scope, moduleAt(&runtime, module, "__name__"));
  EXPECT_TRUE(mod_name.equalsCStr("mymodule"));
  EXPECT_EQ(moduleAt(&runtime, module, "__doc__"), NoneType::object());
  EXPECT_EQ(moduleAt(&runtime, module, "__package__"), NoneType::object());
  EXPECT_EQ(moduleAt(&runtime, module, "__loader__"), NoneType::object());
  EXPECT_EQ(moduleAt(&runtime, module, "__spec__"), NoneType::object());
}

TEST(FunctionAttrTest, SetAttribute) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def foo(): pass
foo.x = 3
)")
                   .isError());
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Function function(&scope, moduleAt(&runtime, "__main__", "foo"));
  Dict function_dict(&scope, function.dict());
  Object key(&scope, runtime.newStrFromCStr("x"));
  Object value(&scope, runtime.dictAt(thread, function_dict, key));
  EXPECT_TRUE(isIntEqualsWord(*value, 3));
}

TEST(RuntimeTest, LazyInitializationOfFunctionDictWithAttribute) {
  Runtime runtime;
  HandleScope scope;
  Function function(&scope, makeTestFunction());
  ASSERT_TRUE(function.dict().isNoneType());

  Object key(&scope, runtime.newStrFromCStr("bar"));
  runtime.attributeAt(Thread::current(), function, key);
  EXPECT_TRUE(function.dict().isDict());
}

TEST(RuntimeTest, LazyInitializationOfFunctionDict) {
  Runtime runtime;
  HandleScope scope;
  Function function(&scope, makeTestFunction());
  ASSERT_TRUE(function.dict().isNoneType());

  Object key(&scope, runtime.newStrFromCStr("__dict__"));
  runtime.attributeAt(Thread::current(), function, key);
  EXPECT_TRUE(function.dict().isDict());
}

TEST(RuntimeTest, NotMatchingCellAndVarNamesSetsCell2ArgToNone) {
  Runtime runtime;
  HandleScope scope;
  word argcount = 3;
  word kwargcount = 0;
  Tuple varnames(&scope, runtime.newTuple(argcount + kwargcount));
  Tuple cellvars(&scope, runtime.newTuple(2));
  Str foo(&scope, runtime.internStrFromCStr("foo"));
  Str bar(&scope, runtime.internStrFromCStr("bar"));
  Str baz(&scope, runtime.internStrFromCStr("baz"));
  Str foobar(&scope, runtime.internStrFromCStr("foobar"));
  Str foobaz(&scope, runtime.internStrFromCStr("foobaz"));
  varnames.atPut(0, *foo);
  varnames.atPut(1, *bar);
  varnames.atPut(2, *baz);
  cellvars.atPut(0, *foobar);
  cellvars.atPut(1, *foobaz);
  Object none(&scope, NoneType::object());
  Tuple empty_tuple(&scope, runtime.newTuple(0));
  Object empty_bytes(&scope, Bytes::empty());
  Object empty_str(&scope, Str::empty());
  Code code(&scope,
            runtime.newCode(argcount, kwargcount, 0, 0, 0, none, empty_tuple,
                            empty_tuple, varnames, empty_tuple, cellvars,
                            empty_str, empty_str, 0, empty_bytes));
  EXPECT_TRUE(code.cell2arg().isNoneType());
}

TEST(RuntimeTest, MatchingCellAndVarNamesCreatesCell2Arg) {
  Runtime runtime;
  HandleScope scope;
  word argcount = 3;
  word kwargcount = 0;
  Tuple varnames(&scope, runtime.newTuple(argcount + kwargcount));
  Tuple cellvars(&scope, runtime.newTuple(2));
  Str foo(&scope, runtime.internStrFromCStr("foo"));
  Str bar(&scope, runtime.internStrFromCStr("bar"));
  Str baz(&scope, runtime.internStrFromCStr("baz"));
  Str foobar(&scope, runtime.internStrFromCStr("foobar"));
  varnames.atPut(0, *foo);
  varnames.atPut(1, *bar);
  varnames.atPut(2, *baz);
  cellvars.atPut(0, *baz);
  cellvars.atPut(1, *foobar);
  Object none(&scope, NoneType::object());
  Tuple empty_tuple(&scope, runtime.newTuple(0));
  Object empty_bytes(&scope, Bytes::empty());
  Object empty_str(&scope, Str::empty());
  Code code(&scope,
            runtime.newCode(argcount, kwargcount, 0, 0, 0, none, empty_tuple,
                            empty_tuple, varnames, empty_tuple, cellvars,
                            empty_str, empty_str, 0, empty_bytes));
  ASSERT_FALSE(code.cell2arg().isNoneType());
  Tuple cell2arg(&scope, code.cell2arg());
  ASSERT_EQ(cell2arg.length(), 2);

  Object cell2arg_value(&scope, cell2arg.at(0));
  EXPECT_TRUE(isIntEqualsWord(*cell2arg_value, 2));
  EXPECT_EQ(cell2arg.at(1), NoneType::object());
}

// Set is not special except that it is a builtin type with sealed attributes.
TEST(RuntimeTest, SetHasSameSizeCreatedTwoDifferentWays) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Layout layout(&scope, runtime.layoutAt(LayoutId::kSet));
  Set set1(&scope, runtime.newInstance(layout));
  Set set2(&scope, runtime.newSet());
  EXPECT_EQ(set1.size(), set2.size());
}

// Set is not special except that it is a builtin type with sealed attributes.
TEST(RuntimeTest, SealedClassLayoutDoesNotHaveSpaceForOverflowAttributes) {
  Runtime runtime;
  HandleScope scope;
  Layout layout(&scope, runtime.layoutAt(LayoutId::kSet));
  EXPECT_TRUE(layout.overflowAttributes().isNoneType());
  word expected_set_size = kPointerSize * layout.numInObjectAttributes();
  EXPECT_EQ(layout.instanceSize(), expected_set_size);
}

TEST(RuntimeTest, SettingNewAttributeOnSealedClassRaisesAttributeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Str attr(&scope, runtime.newStrFromCStr("attr"));
  Str value(&scope, runtime.newStrFromCStr("value"));
  Object result(&scope, instanceSetAttr(thread, set, attr, value));
  EXPECT_TRUE(raised(*result, LayoutId::kAttributeError));
}

TEST(RuntimeTest, InstanceAtPutWithReadOnlyAttributeRaisesAttributeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  BuiltinAttribute attrs[] = {
      {SymbolId::kDunderGlobals, 0, AttributeFlags::kReadOnly},
      {SymbolId::kSentinelId, -1},
  };
  BuiltinMethod builtins[] = {
      {SymbolId::kSentinelId, nullptr},
  };
  LayoutId layout_id = runtime.reserveLayoutId(thread);
  Type type(&scope, runtime.addBuiltinType(SymbolId::kVersion, layout_id,
                                           LayoutId::kObject, attrs, builtins));
  Layout layout(&scope, type.instanceLayout());
  runtime.layoutAtPut(layout_id, *layout);
  HeapObject instance(&scope, runtime.newInstance(layout));
  Object attribute_name(&scope, runtime.newStrFromCStr("__globals__"));
  Object value(&scope, NoneType::object());
  EXPECT_TRUE(raisedWithStr(
      instanceSetAttr(thread, instance, attribute_name, value),
      LayoutId::kAttributeError, "'__globals__' attribute is read-only"));
}

// Exception attributes can be set on the fly.
TEST(RuntimeTest, NonSealedClassHasSpaceForOverflowAttrbutes) {
  Runtime runtime;
  HandleScope scope;
  Layout layout(&scope, runtime.layoutAt(LayoutId::kMemoryError));
  EXPECT_TRUE(layout.overflowAttributes().isTuple());
  EXPECT_EQ(layout.instanceSize(),
            (layout.numInObjectAttributes() + 1) * kPointerSize);  // 1=overflow
}

// User-defined class attributes can be set on the fly.
TEST(RuntimeTest, UserCanSetOverflowAttributeOnUserDefinedClass) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C(): pass
a = C()
)")
                   .isError());
  HeapObject a(&scope, moduleAt(&runtime, "__main__", "a"));
  Str attr(&scope, runtime.newStrFromCStr("attr"));
  Str value(&scope, runtime.newStrFromCStr("value"));
  Object result(&scope, instanceSetAttr(thread, a, attr, value));
  ASSERT_FALSE(result.isError());
  EXPECT_EQ(instanceGetAttribute(thread, a, attr), *value);
}

TEST(RuntimeTest, IsMappingReturnsFalseOnSet) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  EXPECT_FALSE(runtime.isMapping(thread, set));
}

TEST(RuntimeTest, IsMappingReturnsTrueOnDict) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  EXPECT_TRUE(runtime.isMapping(thread, dict));
}

TEST(RuntimeTest, IsMappingReturnsTrueOnList) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, runtime.newList());
  EXPECT_TRUE(runtime.isMapping(Thread::current(), list));
}

TEST(RuntimeTest, IsMappingReturnsTrueOnCustomClassWithMethod) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C():
  def __getitem__(self, key):
    pass
o = C()
)")
                   .isError());
  HandleScope scope;
  Object obj(&scope, moduleAt(&runtime, "__main__", "o"));
  EXPECT_TRUE(runtime.isMapping(Thread::current(), obj));
}

TEST(RuntimeTest, IsMappingWithClassAttrNotCallableReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C():
  __getitem__ = 4
o = C()
)")
                   .isError());
  HandleScope scope;
  Object obj(&scope, moduleAt(&runtime, "__main__", "o"));
  EXPECT_TRUE(runtime.isMapping(Thread::current(), obj));
}

TEST(RuntimeTest, IsMappingReturnsFalseOnCustomClassWithoutMethod) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C():
  pass
o = C()
)")
                   .isError());
  HandleScope scope;
  Object obj(&scope, moduleAt(&runtime, "__main__", "o"));
  EXPECT_FALSE(runtime.isMapping(Thread::current(), obj));
}

TEST(RuntimeTest, IsMappingWithInstanceAttrReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C():
  pass
o = C()
o.__getitem__ = 4
)")
                   .isError());
  HandleScope scope;
  Object obj(&scope, moduleAt(&runtime, "__main__", "o"));
  EXPECT_FALSE(runtime.isMapping(Thread::current(), obj));
}

TEST(RuntimeTest, ModuleBuiltinsExists) {
  Runtime runtime;
  ASSERT_FALSE(moduleAt(&runtime, "builtins", "__name__").isError());
}

TEST(RuntimeTest, ModuleItertoolsExists) {
  Runtime runtime;
  ASSERT_FALSE(moduleAt(&runtime, "itertools", "__name__").isError());
}

TEST(RuntimeTest, ModuleUnderFunctoolsExists) {
  Runtime runtime;
  ASSERT_FALSE(moduleAt(&runtime, "_functools", "__name__").isError());
}

TEST(RuntimeTest, NewBuiltinFunctionAddsQualname) {
  Runtime runtime;
  HandleScope scope;
  Str name(&scope, runtime.newStrFromCStr("Foo.bar"));
  Function fn(&scope, runtime.newNativeFunction(
                          SymbolId::kDummy, name, unimplementedTrampoline,
                          unimplementedTrampoline, unimplementedTrampoline));
  EXPECT_EQ(fn.qualname(), *name);
}

TEST(RuntimeStrTest, StrReplaceWithSmallStrResult) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str str(&scope, runtime.newStrFromCStr("1212"));
  Str old(&scope, runtime.newStrFromCStr("2"));
  Str newstr(&scope, runtime.newStrFromCStr("*"));
  Object result(&scope, runtime.strReplace(thread, str, old, newstr, -1));
  EXPECT_TRUE(isStrEqualsCStr(*result, "1*1*"));
}

TEST(RuntimeStrTest, StrReplaceWithSmallStrAndNegativeReplacesAll) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str str(&scope, runtime.newStrFromCStr("122"));
  Str old(&scope, runtime.newStrFromCStr("2"));
  Str newstr(&scope, runtime.newStrFromCStr("*"));
  Object result(&scope, runtime.strReplace(thread, str, old, newstr, -1));
  EXPECT_TRUE(isStrEqualsCStr(*result, "1**"));
}

TEST(RuntimeStrTest, StrReplaceWithLargeStrAndNegativeReplacesAll) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str str(&scope, runtime.newStrFromCStr("111111121111111111211"));
  Str old(&scope, runtime.newStrFromCStr("2"));
  Str newstr(&scope, runtime.newStrFromCStr("*"));
  Object result(&scope, runtime.strReplace(thread, str, old, newstr, -1));
  EXPECT_TRUE(isStrEqualsCStr(*result, "1111111*1111111111*11"));
}

TEST(RuntimeStrTest, StrReplaceWithLargeStrAndCountReplacesSome) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str str(&scope, runtime.newStrFromCStr("11112111111111111211"));
  Str old(&scope, runtime.newStrFromCStr("2"));
  Str newstr(&scope, runtime.newStrFromCStr("*"));
  Object result(&scope, runtime.strReplace(thread, str, old, newstr, 1));
  EXPECT_TRUE(isStrEqualsCStr(*result, "1111*111111111111211"));
}

TEST(RuntimeStrTest, StrReplaceWithSameLengthReplacesSubstr) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str str(&scope, runtime.newStrFromCStr("12"));
  Str old(&scope, runtime.newStrFromCStr("2"));
  Str newstr(&scope, runtime.newStrFromCStr("*"));
  Object result(&scope, runtime.strReplace(thread, str, old, newstr, -1));
  EXPECT_TRUE(isStrEqualsCStr(*result, "1*"));
}

TEST(RuntimeStrTest, StrReplaceWithLongerNewReturnsLonger) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str str(&scope, runtime.newStrFromCStr("12"));
  Str old(&scope, runtime.newStrFromCStr("2"));
  Str newstr(&scope, runtime.newStrFromCStr("**"));
  Object result(&scope, runtime.strReplace(thread, str, old, newstr, -1));
  EXPECT_TRUE(isStrEqualsCStr(*result, "1**"));
}

TEST(RuntimeStrTest, StrReplaceWithShorterNewReturnsShorter) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str str(&scope, runtime.newStrFromCStr("12"));
  Str old(&scope, runtime.newStrFromCStr("12"));
  Str newstr(&scope, runtime.newStrFromCStr("*"));
  Object result(&scope, runtime.strReplace(thread, str, old, newstr, -1));
  EXPECT_TRUE(isStrEqualsCStr(*result, "*"));
}

TEST(RuntimeStrTest, StrReplaceWithPrefixReplacesBeginning) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str str(&scope, runtime.newStrFromCStr("12"));
  Str old(&scope, runtime.newStrFromCStr("1"));
  Str newstr(&scope, runtime.newStrFromCStr("*"));
  Object result(&scope, runtime.strReplace(thread, str, old, newstr, -1));
  EXPECT_TRUE(isStrEqualsCStr(*result, "*2"));
}

TEST(RuntimeStrTest, StrReplaceWithInfixReplacesMiddle) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str str(&scope, runtime.newStrFromCStr("121"));
  Str old(&scope, runtime.newStrFromCStr("2"));
  Str newstr(&scope, runtime.newStrFromCStr("*"));
  Object result(&scope, runtime.strReplace(thread, str, old, newstr, -1));
  EXPECT_TRUE(isStrEqualsCStr(*result, "1*1"));
}

TEST(RuntimeStrTest, StrReplaceWithPostfixReplacesEnd) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str str(&scope, runtime.newStrFromCStr("112"));
  Str old(&scope, runtime.newStrFromCStr("2"));
  Str newstr(&scope, runtime.newStrFromCStr("*"));
  Object result(&scope, runtime.strReplace(thread, str, old, newstr, -1));
  EXPECT_TRUE(isStrEqualsCStr(*result, "11*"));
}

TEST(RuntimeTest, BuiltinBaseOfNonEmptyTypeIsTypeItself) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  BuiltinAttribute attrs[] = {
      {SymbolId::kDunderGlobals, 0, AttributeFlags::kReadOnly},
      {SymbolId::kSentinelId, -1},
  };
  BuiltinMethod builtins[] = {
      {SymbolId::kSentinelId, nullptr},
  };
  LayoutId layout_id = runtime.reserveLayoutId(thread);
  Type type(&scope, runtime.addBuiltinType(SymbolId::kVersion, layout_id,
                                           LayoutId::kObject, attrs, builtins));
  EXPECT_EQ(type.builtinBase(), layout_id);
}

TEST(RuntimeTest, BuiltinBaseOfEmptyTypeIsSuperclass) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  BuiltinAttribute attrs[] = {
      {SymbolId::kSentinelId, -1},
  };
  BuiltinMethod builtins[] = {
      {SymbolId::kSentinelId, nullptr},
  };
  LayoutId layout_id = runtime.reserveLayoutId(thread);
  Type type(&scope, runtime.addBuiltinType(SymbolId::kVersion, layout_id,
                                           LayoutId::kObject, attrs, builtins));
  EXPECT_EQ(type.builtinBase(), LayoutId::kObject);
}

}  // namespace python
