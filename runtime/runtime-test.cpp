#include "gtest/gtest.h"

#include <cstdlib>
#include <memory>

#include "bytecode.h"
#include "frame.h"
#include "layout.h"
#include "module-builtins.h"
#include "object-builtins.h"
#include "runtime.h"
#include "symbols.h"
#include "test-utils.h"
#include "trampolines.h"

namespace python {
using namespace testing;

using RuntimeAttributeTest = RuntimeFixture;
using RuntimeByteArrayTest = RuntimeFixture;
using RuntimeBytesTest = RuntimeFixture;
using RuntimeClassAttrTest = RuntimeFixture;
using RuntimeDictTest = RuntimeFixture;
using RuntimeFunctionAttrTest = RuntimeFixture;
using RuntimeInstanceAttrTest = RuntimeFixture;
using RuntimeIntTest = RuntimeFixture;
using RuntimeListTest = RuntimeFixture;
using RuntimeMetaclassTest = RuntimeFixture;
using RuntimeModuleAttrTest = RuntimeFixture;
using RuntimeModuleTest = RuntimeFixture;
using RuntimeSetTest = RuntimeFixture;
using RuntimeStrArrayTest = RuntimeFixture;
using RuntimeStrTest = RuntimeFixture;
using RuntimeTest = RuntimeFixture;
using RuntimeTupleTest = RuntimeFixture;
using RuntimeTypeCallTest = RuntimeFixture;

RawObject makeTestFunction() {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Code code(&scope, newEmptyCode());
  const byte bytecode[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  code.setCode(runtime->newBytesWithAll(bytecode));
  Tuple consts(&scope, runtime->newTuple(1));
  consts.atPut(0, NoneType::object());
  code.setConsts(*consts);
  Object qualname(&scope, runtime->newStrFromCStr("foo"));
  Dict globals(&scope, runtime->newDict());
  return runtime->newFunctionWithCode(thread, qualname, code, globals);
}

TEST_F(RuntimeTest, CollectGarbage) {
  ASSERT_TRUE(runtime_.heap()->verify());
  runtime_.collectGarbage();
  ASSERT_TRUE(runtime_.heap()->verify());
}

TEST_F(RuntimeTest, ComputeBuiltinBaseChecksBuiltinBaseOfMROTypes) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class OneFromLeastUpperBound(LookupError, SystemError):
  pass
class TwoFromLeastUpperBound(UnicodeDecodeError, LookupError):
  pass
)")
                   .isError());
  HandleScope scope;
  Object lookup_sub(&scope, mainModuleAt(&runtime_, "OneFromLeastUpperBound"));
  Object unic_dec_sub(&scope,
                      mainModuleAt(&runtime_, "TwoFromLeastUpperBound"));
  ASSERT_TRUE(lookup_sub.isType());
  ASSERT_TRUE(unic_dec_sub.isType());
  Type lookup_sub_type(&scope, *lookup_sub);
  Type unic_dec_sub_type(&scope, *unic_dec_sub);
  EXPECT_EQ(lookup_sub_type.builtinBase(), LayoutId::kLookupError);
  EXPECT_EQ(unic_dec_sub_type.builtinBase(), LayoutId::kUnicodeDecodeError);

  // Ensure that the subclass has its superclasses in its mro
  Type lookup_type(&scope, runtime_.typeAt(LayoutId::kLookupError));
  Type system_type(&scope, runtime_.typeAt(LayoutId::kSystemError));
  Type unic_dec_type(&scope, runtime_.typeAt(LayoutId::kUnicodeDecodeError));
  EXPECT_TRUE(runtime_.isSubclass(lookup_sub_type, lookup_type));
  EXPECT_TRUE(runtime_.isSubclass(lookup_sub_type, system_type));
  EXPECT_TRUE(runtime_.isSubclass(unic_dec_sub_type, unic_dec_type));
  EXPECT_TRUE(runtime_.isSubclass(unic_dec_sub_type, lookup_type));
}

TEST_F(RuntimeTest, ComputeBuiltinBaseWithConflictingBasesRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class FailingMultiClass(UnicodeDecodeError, UnicodeEncodeError):
  pass
)"),
                            LayoutId::kTypeError,
                            "multiple bases have instance lay-out conflict"));
}

TEST(RuntimeTestNoFixture, AllocateAndCollectGarbage) {
  const word heap_size = 32 * kMiB;
  const word array_length = 1024;
  const word allocation_size = Utils::roundUp(
      array_length + HeapObject::headerSize(array_length), kPointerSize);
  const word total_allocation_size = heap_size * 10;
  Runtime runtime(heap_size, /*cache_enabled=*/false);
  ASSERT_TRUE(runtime.heap()->verify());
  for (word i = 0; i < total_allocation_size; i += allocation_size) {
    runtime.newBytes(array_length, 0);
  }
  ASSERT_TRUE(runtime.heap()->verify());
}

TEST_F(RuntimeTest, AttributeAtCallsDunderGetattribute) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  foo = None
  def __getattribute__(self, name):
    return (self, name)
c = C()
)")
                   .isError());
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  Object name(&scope, runtime_.newStrFromCStr("foo"));
  Object result_obj(&scope, runtime_.attributeAt(thread_, c, name));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), c);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "foo"));
}

TEST_F(RuntimeTest, AttributeAtPropagatesExceptionFromDunderGetAttribute) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __getattribute__(self, name):
    raise UserWarning()
c = C()
)")
                   .isError());
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  Object name(&scope, runtime_.newStrFromCStr("foo"));
  EXPECT_TRUE(
      raised(runtime_.attributeAt(thread_, c, name), LayoutId::kUserWarning));
}

TEST_F(RuntimeTest, AttributeAtCallsDunderGetattr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  foo = 10
  def __getattr__(self, name):
    return (self, name)
c = C()
)")
                   .isError());
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  Object foo(&scope, runtime_.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(runtime_.attributeAt(thread_, c, foo), 10));
  Object bar(&scope, runtime_.newStrFromCStr("bar"));
  Object result_obj(&scope, runtime_.attributeAt(thread_, c, bar));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), c);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "bar"));
}

TEST_F(RuntimeTest, AttributeAtDoesNotCallDunderGetattrOnNonAttributeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __getattribute__(self, name):
    raise UserWarning()
  def __getattr__(self, name):
    _unimplemented()
c = C()
)")
                   .isError());
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  Object foo(&scope, runtime_.newStrFromCStr("foo"));
  EXPECT_TRUE(
      raised(runtime_.attributeAt(thread_, c, foo), LayoutId::kUserWarning));
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
  Object elt(&scope, runtime.concreteTypeAt(id));
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

TEST_F(RuntimeTest, ConcreteTypeBaseIsUserType) {
  HandleScope scope(thread_);
  Object smallint(&scope, SmallInt::fromWord(42));
  Object largeint(&scope, runtime_.newIntFromUnsigned(kMaxUword));
  Type smallint_type(&scope, runtime_.concreteTypeOf(*smallint));
  Type largeint_type(&scope, runtime_.concreteTypeOf(*largeint));
  EXPECT_EQ(smallint_type.instanceLayout(),
            runtime_.layoutAt(LayoutId::kSmallInt));
  EXPECT_EQ(largeint_type.instanceLayout(),
            runtime_.layoutAt(LayoutId::kLargeInt));
  EXPECT_EQ(smallint_type.builtinBase(), LayoutId::kInt);
  EXPECT_EQ(largeint_type.builtinBase(), LayoutId::kInt);
}

TEST_F(RuntimeByteArrayTest, EnsureCapacity) {
  HandleScope scope(thread_);

  ByteArray array(&scope, runtime_.newByteArray());
  word length = 1;
  word expected_capacity = 16;
  runtime_.byteArrayEnsureCapacity(thread_, array, length);
  EXPECT_EQ(array.capacity(), expected_capacity);

  length = 17;
  expected_capacity = 24;
  runtime_.byteArrayEnsureCapacity(thread_, array, length);
  EXPECT_EQ(array.capacity(), expected_capacity);

  length = 40;
  expected_capacity = 40;
  runtime_.byteArrayEnsureCapacity(thread_, array, length);
  EXPECT_EQ(array.capacity(), expected_capacity);
}

TEST_F(RuntimeByteArrayTest, Extend) {
  HandleScope scope(thread_);

  ByteArray array(&scope, runtime_.newByteArray());
  View<byte> hello(reinterpret_cast<const byte*>("Hello world!"), 5);
  runtime_.byteArrayExtend(thread_, array, hello);
  EXPECT_GE(array.capacity(), 5);
  EXPECT_EQ(array.numItems(), 5);

  Bytes bytes(&scope, array.bytes());
  bytes = runtime_.bytesSubseq(thread_, bytes, 0, 5);
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "Hello"));
}

TEST_F(RuntimeBytesTest, Concat) {
  HandleScope scope(thread_);

  View<byte> foo(reinterpret_cast<const byte*>("foo"), 3);
  Bytes self(&scope, runtime_.newBytesWithAll(foo));
  View<byte> bar(reinterpret_cast<const byte*>("bar"), 3);
  Bytes other(&scope, runtime_.newBytesWithAll(bar));
  Bytes result(&scope, runtime_.bytesConcat(thread_, self, other));
  EXPECT_TRUE(isBytesEqualsCStr(result, "foobar"));
}

TEST_F(RuntimeBytesTest, FromTupleWithSizeReturnsBytesMatchingSize) {
  HandleScope scope(thread_);
  Tuple tuple(&scope, runtime_.newTuple(3));
  tuple.atPut(0, SmallInt::fromWord(42));
  tuple.atPut(1, SmallInt::fromWord(123));
  Object result(&scope, runtime_.bytesFromTuple(thread_, tuple, 2));
  const byte bytes[] = {42, 123};
  EXPECT_TRUE(isBytesEqualsBytes(result, bytes));
}

TEST_F(RuntimeBytesTest, FromTupleWithNonIndexReturnsNone) {
  HandleScope scope(thread_);
  Tuple tuple(&scope, runtime_.newTuple(1));
  tuple.atPut(0, runtime_.newFloat(1));
  EXPECT_EQ(runtime_.bytesFromTuple(thread_, tuple, 1), NoneType::object());
}

TEST_F(RuntimeBytesTest, FromTupleWithNegativeIntRaisesValueError) {
  HandleScope scope(thread_);
  Tuple tuple(&scope, runtime_.newTuple(1));
  tuple.atPut(0, SmallInt::fromWord(-1));
  Object result(&scope, runtime_.bytesFromTuple(thread_, tuple, 1));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kValueError,
                            "bytes must be in range(0, 256)"));
}

TEST_F(RuntimeBytesTest, FromTupleWithBigIntRaisesValueError) {
  HandleScope scope(thread_);
  Tuple tuple(&scope, runtime_.newTuple(1));
  tuple.atPut(0, SmallInt::fromWord(256));
  Object result(&scope, runtime_.bytesFromTuple(thread_, tuple, 1));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kValueError,
                            "bytes must be in range(0, 256)"));
}

TEST_F(RuntimeBytesTest, FromTupleWithIntSubclassReturnsBytes) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C(int): pass
a = C(97)
b = C(98)
c = C(99)
)")
                   .isError());
  HandleScope scope(thread_);
  Tuple tuple(&scope, runtime_.newTuple(3));
  tuple.atPut(0, mainModuleAt(&runtime_, "a"));
  tuple.atPut(1, mainModuleAt(&runtime_, "b"));
  tuple.atPut(2, mainModuleAt(&runtime_, "c"));
  Object result(&scope, runtime_.bytesFromTuple(thread_, tuple, 3));
  EXPECT_TRUE(isBytesEqualsCStr(result, "abc"));
}

TEST_F(RuntimeBytesTest, Subseq) {
  HandleScope scope(thread_);

  View<byte> hello(reinterpret_cast<const byte*>("Hello world!"), 12);
  Bytes bytes(&scope, runtime_.newBytesWithAll(hello));
  ASSERT_EQ(bytes.length(), 12);

  Bytes copy(&scope, runtime_.bytesSubseq(thread_, bytes, 6, 5));
  EXPECT_TRUE(isBytesEqualsCStr(copy, "world"));
}

TEST_F(RuntimeDictTest, EmptyDictInvariants) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());

  EXPECT_EQ(dict.numItems(), 0);
  ASSERT_TRUE(dict.data().isTuple());
  EXPECT_EQ(Tuple::cast(dict.data()).length(), 0);
}

TEST_F(RuntimeDictTest, DictAtPutRetainsExistingKeyObject) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  Str key0(&scope, runtime_.newStrFromCStr("foobarbazbam"));
  Object key0_hash(&scope, Interpreter::hash(thread_, key0));
  ASSERT_FALSE(key0_hash.isErrorException());
  Object value0(&scope, SmallInt::fromWord(123));
  Str key1(&scope, runtime_.newStrFromCStr("foobarbazbam"));
  Object key1_hash(&scope, Interpreter::hash(thread_, key1));
  ASSERT_FALSE(key1_hash.isErrorException());
  Object value1(&scope, SmallInt::fromWord(456));
  ASSERT_NE(key0, key1);
  ASSERT_EQ(key0_hash, key1_hash);

  runtime_.dictAtPut(thread_, dict, key0, key0_hash, value0);
  ASSERT_EQ(dict.numItems(), 1);
  ASSERT_EQ(runtime_.dictAt(thread_, dict, key0, key0_hash), *value0);

  // Overwrite the stored value
  runtime_.dictAtPut(thread_, dict, key1, key1_hash, value1);
  ASSERT_EQ(dict.numItems(), 1);
  ASSERT_EQ(runtime_.dictAt(thread_, dict, key1, key1_hash), *value1);

  Tuple data(&scope, dict.data());
  word i = Dict::Bucket::kFirst;
  Dict::Bucket::nextItem(*data, &i);
  EXPECT_EQ(Dict::Bucket::key(*data, i), key0);
}

TEST_F(RuntimeDictTest, GetSet) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  Object key(&scope, SmallInt::fromWord(12345));
  Object key_hash(&scope, Interpreter::hash(thread_, key));
  ASSERT_FALSE(key_hash.isErrorException());

  // Looking up a key that doesn't exist should fail
  EXPECT_TRUE(runtime_.dictAt(thread_, dict, key, key_hash).isError());

  // Store a value
  Object stored(&scope, SmallInt::fromWord(67890));
  runtime_.dictAtPut(thread_, dict, key, key_hash, stored);
  EXPECT_EQ(dict.numItems(), 1);

  // Retrieve the stored value
  RawObject retrieved = runtime_.dictAt(thread_, dict, key, key_hash);
  EXPECT_EQ(retrieved, *stored);

  // Overwrite the stored value
  Object new_value(&scope, SmallInt::fromWord(5555));
  runtime_.dictAtPut(thread_, dict, key, key_hash, new_value);
  EXPECT_EQ(dict.numItems(), 1);

  // Get the new value
  retrieved = runtime_.dictAt(thread_, dict, key, key_hash);
  EXPECT_EQ(retrieved, *new_value);
}

TEST_F(RuntimeDictTest, Remove) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  Object key(&scope, SmallInt::fromWord(12345));
  Object key_hash(&scope, Interpreter::hash(thread_, key));
  ASSERT_FALSE(key_hash.isErrorException());

  // Removing a key that doesn't exist should fail
  bool is_missing = runtime_.dictRemove(thread_, dict, key, key_hash).isError();
  EXPECT_TRUE(is_missing);

  // Removing a key that exists should succeed and return the value that was
  // stored.
  Object stored(&scope, SmallInt::fromWord(54321));

  runtime_.dictAtPut(thread_, dict, key, key_hash, stored);
  EXPECT_EQ(dict.numItems(), 1);

  RawObject retrieved = runtime_.dictRemove(thread_, dict, key, key_hash);
  ASSERT_FALSE(retrieved.isError());
  ASSERT_EQ(SmallInt::cast(retrieved).value(), SmallInt::cast(*stored).value());

  // Looking up a key that was deleted should fail
  EXPECT_TRUE(runtime_.dictAt(thread_, dict, key, key_hash).isError());
  EXPECT_EQ(dict.numItems(), 0);
}

TEST_F(RuntimeDictTest, Length) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());

  // Add 10 items and make sure length reflects it
  for (int i = 0; i < 10; i++) {
    Object key(&scope, SmallInt::fromWord(i));
    Object key_hash(&scope, Interpreter::hash(thread_, key));
    ASSERT_FALSE(key_hash.isErrorException());
    runtime_.dictAtPut(thread_, dict, key, key_hash, key);
  }
  EXPECT_EQ(dict.numItems(), 10);

  // Remove half the items
  for (int i = 0; i < 5; i++) {
    Object key(&scope, SmallInt::fromWord(i));
    Object key_hash(&scope, Interpreter::hash(thread_, key));
    ASSERT_FALSE(key_hash.isErrorException());
    ASSERT_FALSE(runtime_.dictRemove(thread_, dict, key, key_hash).isError());
  }
  EXPECT_EQ(dict.numItems(), 5);
}

TEST_F(RuntimeDictTest, AtIfAbsentPutLength) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());

  Object k1(&scope, SmallInt::fromWord(1));
  Object k1_hash(&scope, Interpreter::hash(thread_, k1));
  ASSERT_FALSE(k1_hash.isErrorException());
  Object v1(&scope, SmallInt::fromWord(111));
  runtime_.dictAtPut(thread_, dict, k1, k1_hash, v1);
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
  Object k2_hash(&scope, Interpreter::hash(thread_, k2));
  ASSERT_FALSE(k2_hash.isErrorException());
  SmallIntCallback cb(222);
  runtime_.dictAtIfAbsentPut(thread_, dict, k2, k2_hash, &cb);
  EXPECT_EQ(dict.numItems(), 2);
  RawObject retrieved = runtime_.dictAt(thread_, dict, k2, k2_hash);
  EXPECT_TRUE(isIntEqualsWord(retrieved, 222));

  // Don't overrwite existing item 1 -> v1
  Object k3(&scope, SmallInt::fromWord(1));
  Object k3_hash(&scope, Interpreter::hash(thread_, k3));
  ASSERT_FALSE(k3_hash.isErrorException());
  SmallIntCallback cb3(333);
  runtime_.dictAtIfAbsentPut(thread_, dict, k3, k3_hash, &cb3);
  EXPECT_EQ(dict.numItems(), 2);
  retrieved = runtime_.dictAt(thread_, dict, k3, k3_hash);
  EXPECT_EQ(retrieved, *v1);
}

TEST_F(RuntimeDictTest, DictAtPutGrowsDictWhenDictIsEmpty) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  EXPECT_EQ(dict.capacity(), 0);

  Object first_key(&scope, SmallInt::fromWord(0));
  Object first_key_hash(&scope, Interpreter::hash(thread_, first_key));
  ASSERT_FALSE(first_key_hash.isErrorException());
  Object first_value(&scope, SmallInt::fromWord(1));
  runtime_.dictAtPut(thread_, dict, first_key, first_key_hash, first_value);

  word initial_capacity = Runtime::kInitialDictCapacity;
  EXPECT_EQ(dict.numItems(), 1);
  EXPECT_EQ(dict.capacity(), initial_capacity);
}

TEST_F(RuntimeDictTest, DictAtPutGrowsDictWhenTwoThirdsUsed) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());

  // Fill in one fewer keys than would require growing the underlying object
  // array again.
  word threshold = ((Runtime::kInitialDictCapacity * 2) / 3) - 1;
  for (word i = 0; i < threshold; i++) {
    Object key(&scope, SmallInt::fromWord(i));
    Object key_hash(&scope, Interpreter::hash(thread_, key));
    ASSERT_FALSE(key_hash.isErrorException());
    Object value(&scope, SmallInt::fromWord(-i));
    runtime_.dictAtPut(thread_, dict, key, key_hash, value);
  }
  EXPECT_EQ(dict.numItems(), threshold);
  EXPECT_EQ(dict.numUsableItems(), 1);
  word initial_capacity = Runtime::kInitialDictCapacity;
  EXPECT_EQ(dict.capacity(), initial_capacity);

  // Add another key which should force us to double the capacity
  Object last_key(&scope, SmallInt::fromWord(threshold));
  Object last_key_hash(&scope, Interpreter::hash(thread_, last_key));
  ASSERT_FALSE(last_key_hash.isErrorException());
  Object last_value(&scope, SmallInt::fromWord(-threshold));
  runtime_.dictAtPut(thread_, dict, last_key, last_key_hash, last_value);
  EXPECT_EQ(dict.numItems(), threshold + 1);
  EXPECT_EQ(dict.capacity(), initial_capacity * Runtime::kDictGrowthFactor);
  EXPECT_EQ(dict.numUsableItems(),
            ((dict.capacity() * 2) / 3) - dict.numItems());

  // Make sure we can still read all the stored keys/values.
  for (word i = 0; i <= threshold; i++) {
    Object key(&scope, SmallInt::fromWord(i));
    Object key_hash(&scope, Interpreter::hash(thread_, key));
    ASSERT_FALSE(key_hash.isErrorException());
    RawObject value = runtime_.dictAt(thread_, dict, key, key_hash);
    ASSERT_FALSE(value.isError());
    EXPECT_TRUE(isIntEqualsWord(value, -i));
  }
}

TEST_F(RuntimeDictTest, CollidingKeys) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __eq__(self, other):
    return self is other
  def __hash__(self):
    return 0
i0 = C()
i1 = C()
)")
                   .isError());
  Object i0(&scope, mainModuleAt(&runtime_, "i0"));
  Object i0_hash(&scope, Interpreter::hash(thread_, i0));
  ASSERT_FALSE(i0_hash.isErrorException());
  Object i1(&scope, mainModuleAt(&runtime_, "i1"));
  Object i1_hash(&scope, Interpreter::hash(thread_, i1));
  ASSERT_FALSE(i1_hash.isErrorException());
  ASSERT_EQ(i0_hash, i1_hash);

  Dict dict(&scope, runtime_.newDict());

  // Add two different keys with different values using the same hash
  runtime_.dictAtPut(thread_, dict, i0, i0_hash, i0);
  runtime_.dictAtPut(thread_, dict, i1, i1_hash, i1);

  // Make sure we get both back
  Object retrieved(&scope, runtime_.dictAt(thread_, dict, i0, i0_hash));
  EXPECT_EQ(retrieved, i0);

  retrieved = runtime_.dictAt(thread_, dict, i1, i1_hash);
  EXPECT_EQ(retrieved, i1);
}

TEST_F(RuntimeDictTest, MixedKeys) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());

  // Add keys of different type
  Object int_key(&scope, SmallInt::fromWord(100));
  Object int_key_hash(&scope, Interpreter::hash(thread_, int_key));
  ASSERT_FALSE(int_key_hash.isErrorException());
  runtime_.dictAtPut(thread_, dict, int_key, int_key_hash, int_key);

  Object str_key(&scope, runtime_.newStrFromCStr("testing 123"));
  Object str_key_hash(&scope, Interpreter::hash(thread_, str_key));
  ASSERT_FALSE(str_key_hash.isErrorException());
  runtime_.dictAtPut(thread_, dict, str_key, str_key_hash, str_key);

  // Make sure we get the appropriate values back out
  RawObject retrieved = runtime_.dictAt(thread_, dict, int_key, int_key_hash);
  EXPECT_EQ(retrieved, *int_key);

  retrieved = runtime_.dictAt(thread_, dict, str_key, str_key_hash);
  ASSERT_TRUE(retrieved.isStr());
  EXPECT_EQ(*str_key, retrieved);
}

TEST_F(RuntimeDictTest, GetKeys) {
  HandleScope scope(thread_);

  // Create keys
  Tuple keys(&scope, runtime_.newTuple(4));
  keys.atPut(0, SmallInt::fromWord(100));
  keys.atPut(1, runtime_.newStrFromCStr("testing 123"));
  keys.atPut(2, Bool::trueObj());
  keys.atPut(3, NoneType::object());

  // Add keys to dict
  Dict dict(&scope, runtime_.newDict());
  for (word i = 0; i < keys.length(); i++) {
    Object key(&scope, keys.at(i));
    Object key_hash(&scope, Interpreter::hash(thread_, key));
    ASSERT_FALSE(key_hash.isErrorException());
    runtime_.dictAtPut(thread_, dict, key, key_hash, key);
  }

  // Grab the keys and verify everything is there
  List retrieved(&scope, runtime_.dictKeys(thread_, dict));
  ASSERT_EQ(retrieved.numItems(), keys.length());
  for (word i = 0; i < keys.length(); i++) {
    Object key(&scope, keys.at(i));
    EXPECT_TRUE(listContains(retrieved, key)) << " missing key " << i;
  }
}

TEST_F(RuntimeDictTest, CanCreateDictItems) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  RawObject iter = runtime_.newDictItemIterator(thread_, dict);
  ASSERT_TRUE(iter.isDictItemIterator());
}

TEST_F(RuntimeListTest, ListGrowth) {
  HandleScope scope(thread_);
  List list(&scope, runtime_.newList());
  Tuple array1(&scope, runtime_.newMutableTuple(1));
  list.setItems(*array1);
  EXPECT_EQ(array1.length(), 1);
  runtime_.listEnsureCapacity(thread_, list, 2);
  Tuple array2(&scope, list.items());
  EXPECT_NE(*array1, *array2);
  EXPECT_GE(array2.length(), 2);

  Tuple array4(&scope, runtime_.newMutableTuple(4));
  list.setItems(*array4);
  runtime_.listEnsureCapacity(thread_, list, 5);
  Tuple array16(&scope, list.items());
  EXPECT_NE(*array4, *array16);
  EXPECT_EQ(array16.length(), 16);
  runtime_.listEnsureCapacity(thread_, list, 17);
  Tuple array24(&scope, list.items());
  EXPECT_NE(*array16, *array24);
  EXPECT_EQ(array24.length(), 24);
  runtime_.listEnsureCapacity(thread_, list, 40);
  EXPECT_EQ(list.capacity(), 40);
}

TEST_F(RuntimeListTest, EmptyListInvariants) {
  RawList list = List::cast(runtime_.newList());
  ASSERT_EQ(list.capacity(), 0);
  ASSERT_EQ(list.numItems(), 0);
}

TEST_F(RuntimeListTest, AppendToList) {
  HandleScope scope(thread_);
  List list(&scope, runtime_.newList());

  // Check that list capacity grows by 1.5
  word expected_capacity[] = {16, 16, 16, 16, 16, 16, 16, 16, 16,
                              16, 16, 16, 16, 16, 16, 16, 24, 24,
                              24, 24, 24, 24, 24, 24, 36};
  for (int i = 0; i < 25; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime_.listAdd(thread_, list, value);
    ASSERT_EQ(list.capacity(), expected_capacity[i]) << i;
    ASSERT_EQ(list.numItems(), i + 1) << i;
  }

  // Sanity check list contents
  for (int i = 0; i < 25; i++) {
    EXPECT_TRUE(isIntEqualsWord(list.at(i), i)) << i;
  }
}

TEST_F(RuntimeTest, NewByteArray) {
  HandleScope scope(thread_);

  ByteArray array(&scope, runtime_.newByteArray());
  EXPECT_EQ(array.numItems(), 0);
  EXPECT_EQ(array.capacity(), 0);
}

TEST_F(RuntimeTest, NewBytes) {
  HandleScope scope(thread_);

  Bytes len0(&scope, Bytes::empty());
  EXPECT_EQ(len0.length(), 0);

  Bytes len3(&scope, runtime_.newBytes(3, 9));
  EXPECT_EQ(len3.length(), 3);
  EXPECT_EQ(len3.byteAt(0), 9);
  EXPECT_EQ(len3.byteAt(1), 9);
  EXPECT_EQ(len3.byteAt(2), 9);

  Bytes len254(&scope, runtime_.newBytes(254, 0));
  EXPECT_EQ(len254.length(), 254);

  Bytes len255(&scope, runtime_.newBytes(255, 0));
  EXPECT_EQ(len255.length(), 255);
}

TEST_F(RuntimeTest, NewBytesWithAll) {
  HandleScope scope(thread_);

  Bytes len0(&scope, runtime_.newBytesWithAll(View<byte>(nullptr, 0)));
  EXPECT_EQ(len0.length(), 0);

  const byte src1[] = {0x42};
  Bytes len1(&scope, runtime_.newBytesWithAll(src1));
  EXPECT_EQ(len1.length(), 1);
  EXPECT_EQ(len1.byteAt(0), 0x42);

  const byte src3[] = {0xAA, 0xBB, 0xCC};
  Bytes len3(&scope, runtime_.newBytesWithAll(src3));
  EXPECT_EQ(len3.length(), 3);
  EXPECT_EQ(len3.byteAt(0), 0xAA);
  EXPECT_EQ(len3.byteAt(1), 0xBB);
  EXPECT_EQ(len3.byteAt(2), 0xCC);
}

TEST_F(RuntimeTest, NewMemoryViewFromCPtrCreatesMemoryView) {
  HandleScope scope(thread_);
  word length = 5;
  std::unique_ptr<byte[]> memory(new byte[length]);
  for (word i = 0; i < length; i++) {
    memory[i] = i;
  }
  MemoryView view(&scope,
                  runtime_.newMemoryViewFromCPtr(thread_, memory.get(), length,
                                                 ReadOnly::ReadOnly));
  Int buffer(&scope, view.buffer());
  EXPECT_EQ(view.length(), length);
  byte* ptr = reinterpret_cast<byte*>(buffer.asCPtr());
  EXPECT_EQ(ptr[0], 0);
  EXPECT_EQ(ptr[1], 1);
  EXPECT_EQ(ptr[2], 2);
  EXPECT_EQ(ptr[3], 3);
  EXPECT_EQ(ptr[4], 4);
}

TEST_F(RuntimeTest, LargeBytesSizeRoundedUpToPointerSizeMultiple) {
  HandleScope scope(thread_);

  LargeBytes len10(&scope, runtime_.newBytes(10, 0));
  EXPECT_EQ(len10.size(), Utils::roundUp(kPointerSize + 10, kPointerSize));

  LargeBytes len254(&scope, runtime_.newBytes(254, 0));
  EXPECT_EQ(len254.size(), Utils::roundUp(kPointerSize + 254, kPointerSize));

  LargeBytes len255(&scope, runtime_.newBytes(255, 0));
  EXPECT_EQ(len255.size(),
            Utils::roundUp(kPointerSize * 2 + 255, kPointerSize));
}

TEST_F(RuntimeTest, NewTuple) {
  HandleScope scope(thread_);

  Tuple a0(&scope, runtime_.newTuple(0));
  EXPECT_EQ(a0.length(), 0);

  Tuple a1(&scope, runtime_.newTuple(1));
  ASSERT_EQ(a1.length(), 1);
  EXPECT_EQ(a1.at(0), NoneType::object());
  a1.atPut(0, SmallInt::fromWord(42));
  EXPECT_EQ(a1.at(0), SmallInt::fromWord(42));

  Tuple a300(&scope, runtime_.newTuple(300));
  ASSERT_EQ(a300.length(), 300);
}

TEST_F(RuntimeTest, NewStr) {
  HandleScope scope(thread_);
  Str empty0(&scope, runtime_.newStrWithAll(View<byte>(nullptr, 0)));
  ASSERT_TRUE(empty0.isSmallStr());
  EXPECT_EQ(empty0.charLength(), 0);

  Str empty1(&scope, runtime_.newStrWithAll(View<byte>(nullptr, 0)));
  ASSERT_TRUE(empty1.isSmallStr());
  EXPECT_EQ(*empty0, *empty1);

  Str empty2(&scope, runtime_.newStrFromCStr("\0"));
  ASSERT_TRUE(empty2.isSmallStr());
  EXPECT_EQ(*empty0, *empty2);

  const byte bytes1[1] = {0};
  Str s1(&scope, runtime_.newStrWithAll(bytes1));
  ASSERT_TRUE(s1.isSmallStr());
  EXPECT_EQ(s1.charLength(), 1);

  const byte bytes254[254] = {0};
  Str s254(&scope, runtime_.newStrWithAll(bytes254));
  EXPECT_EQ(s254.charLength(), 254);
  ASSERT_TRUE(s254.isLargeStr());
  EXPECT_EQ(HeapObject::cast(*s254).size(),
            Utils::roundUp(kPointerSize + 254, kPointerSize));

  const byte bytes255[255] = {0};
  Str s255(&scope, runtime_.newStrWithAll(bytes255));
  EXPECT_EQ(s255.charLength(), 255);
  ASSERT_TRUE(s255.isLargeStr());
  EXPECT_EQ(HeapObject::cast(*s255).size(),
            Utils::roundUp(kPointerSize * 2 + 255, kPointerSize));

  const byte bytes300[300] = {0};
  Str s300(&scope, runtime_.newStrWithAll(bytes300));
  ASSERT_EQ(s300.charLength(), 300);
}

TEST_F(RuntimeTest, NewStrFromByteArrayCopiesByteArray) {
  HandleScope scope(thread_);

  ByteArray array(&scope, runtime_.newByteArray());
  Object result(&scope, runtime_.newStrFromByteArray(array));
  EXPECT_TRUE(isStrEqualsCStr(*result, ""));

  const byte byte_array[] = {'h', 'e', 'l', 'l', 'o'};
  runtime_.byteArrayExtend(thread_, array, byte_array);
  result = runtime_.newStrFromByteArray(array);
  EXPECT_TRUE(isStrEqualsCStr(*result, "hello"));

  const byte byte_array2[] = {' ', 'w', 'o', 'r', 'l', 'd'};
  runtime_.byteArrayExtend(thread_, array, byte_array2);
  result = runtime_.newStrFromByteArray(array);
  EXPECT_TRUE(isStrEqualsCStr(*result, "hello world"));
}

TEST_F(RuntimeTest, NewStrFromFmtFormatsWord) {
  word x = 5;
  HandleScope scope(thread_);
  Object result(&scope, runtime_.newStrFromFmt("hello %w world", x));
  EXPECT_TRUE(isStrEqualsCStr(*result, "hello 5 world"));
}

TEST_F(RuntimeTest, NewStrFromFmtWithStrArg) {
  HandleScope scope(thread_);

  Object str(&scope, runtime_.newStrFromCStr("hello"));
  Object result(&scope, runtime_.newStrFromFmt("%S", &str));
  EXPECT_EQ(*result, str);
}

TEST_F(RuntimeTest, NewStrFromFmtWithStrSubclassArg) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C(str):
  pass
value = C("foo")
)")
                   .isError());
  Object value(&scope, mainModuleAt(&runtime_, "value"));
  Object result(&scope, runtime_.newStrFromFmt("hello %S", &value));
  EXPECT_TRUE(isStrEqualsCStr(*result, "hello foo"));
}

TEST_F(RuntimeStrTest, NewStrFromFmtFormatsFunctionName) {
  HandleScope scope(thread_);
  Function function(&scope, newEmptyFunction());
  function.setQualname(runtime_.newStrFromCStr("foo"));
  Object str(&scope, runtime_.newStrFromFmt("hello %F", &function));
  EXPECT_TRUE(isStrEqualsCStr(*str, "hello foo"));
}

TEST_F(RuntimeStrTest, NewStrFromFmtFormatsTypeName) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_.newDict());
  Object str(&scope, runtime_.newStrFromFmt("hello %T", &obj));
  EXPECT_TRUE(isStrEqualsCStr(*str, "hello dict"));
}

TEST_F(RuntimeStrTest, NewStrFromFmtFormatsSymbolid) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_.newStrFromFmt("hello %Y", SymbolId::kDict));
  EXPECT_TRUE(isStrEqualsCStr(*str, "hello dict"));
}

TEST_F(RuntimeStrTest, NewStrFromFmtFormatsASCIIChar) {
  EXPECT_TRUE(isStrEqualsCStr(runtime_.newStrFromFmt("'%c'", 124), "'|'"));
}

TEST_F(RuntimeStrTest, NewStrFromFmtFormatsNonASCIIAsReplacementChar) {
  EXPECT_TRUE(isStrEqualsCStr(runtime_.newStrFromFmt("'%c'", kMaxASCII + 1),
                              "'\xef\xbf\xbd'"));
}

TEST_F(RuntimeStrTest, NewStrFromFmtFormatsCodePoint) {
  EXPECT_TRUE(isStrEqualsCStr(runtime_.newStrFromFmt("'%C'", 124), "'|'"));
  EXPECT_TRUE(isStrEqualsCStr(runtime_.newStrFromFmt("'%C'", 0x1F40D),
                              "'\xf0\x9f\x90\x8d'"));
}

TEST_F(RuntimeStrTest, NewStrFromFormatFormatsString) {
  EXPECT_TRUE(
      isStrEqualsCStr(runtime_.newStrFromFmt("'%s'", "hello"), "'hello'"));
}

TEST_F(RuntimeStrTest, NewStrFromFormatFormatsInt) {
  EXPECT_TRUE(isStrEqualsCStr(runtime_.newStrFromFmt("'%d'", -321), "'-321'"));
}

TEST_F(RuntimeStrTest, NewStrFromFormatFormatsFloat) {
  EXPECT_TRUE(isStrEqualsCStr(runtime_.newStrFromFmt("'%g'", 3.5), "'3.5'"));
}

TEST_F(RuntimeStrTest, NewStrFromFormatFormatsHexadecimalInt) {
  EXPECT_TRUE(isStrEqualsCStr(runtime_.newStrFromFmt("'%x'", 0x2AB), "'2ab'"));
}

TEST_F(RuntimeStrTest, NewStrFromFormatFormatsPercent) {
  EXPECT_TRUE(isStrEqualsCStr(runtime_.newStrFromFmt("'%%'"), "'%'"));
}

TEST_F(RuntimeStrTest, NewStrFromFmtFormatsReplacesNonUnicodeWithReplacement) {
  EXPECT_TRUE(
      isStrEqualsCStr(runtime_.newStrFromFmt("'%C'", -1), "'\xef\xbf\xbd'"));
}

TEST_F(RuntimeStrTest, NewStrWithAll) {
  HandleScope scope(thread_);

  Str str0(&scope, runtime_.newStrWithAll(View<byte>(nullptr, 0)));
  EXPECT_EQ(str0.charLength(), 0);
  EXPECT_TRUE(str0.equalsCStr(""));

  const byte bytes3[] = {'A', 'B', 'C'};
  Str str3(&scope, runtime_.newStrWithAll(bytes3));
  EXPECT_EQ(str3.charLength(), 3);
  EXPECT_TRUE(str3.equalsCStr("ABC"));

  const byte bytes10[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'};
  Str str10(&scope, runtime_.newStrWithAll(bytes10));
  EXPECT_EQ(str10.charLength(), 10);
  EXPECT_TRUE(str10.equalsCStr("ABCDEFGHIJ"));
}

TEST_F(RuntimeStrTest, NewStrFromUTF32WithZeroSizeReturnsEmpty) {
  HandleScope scope(thread_);
  int32_t str[2] = {'a', 's'};
  Str empty(&scope, runtime_.newStrFromUTF32(View<int32_t>(str, 0)));
  EXPECT_EQ(empty.charLength(), 0);
}

TEST_F(RuntimeStrTest, NewStrFromUTF32WithLargeASCIIStringReturnsString) {
  HandleScope scope(thread_);
  int32_t str[7] = {'a', 'b', 'c', '1', '2', '3', '-'};
  Str unicode(&scope, runtime_.newStrFromUTF32(View<int32_t>(str, 7)));
  EXPECT_EQ(unicode.charLength(), 7);
  EXPECT_TRUE(unicode.equalsCStr("abc123-"));
}

TEST_F(RuntimeStrTest, NewStrFromUTF32WithSmallASCIIStringReturnsString) {
  HandleScope scope(thread_);
  int32_t str[7] = {'a', 'b'};
  Str unicode(&scope, runtime_.newStrFromUTF32(View<int32_t>(str, 2)));
  EXPECT_EQ(unicode.charLength(), 2);
  EXPECT_TRUE(unicode.equalsCStr("ab"));
}

TEST_F(RuntimeStrTest, NewStrFromUTF32WithSmallNonASCIIReturnsString) {
  HandleScope scope(thread_);
  const int32_t codepoints[] = {0xC4};
  Str unicode(&scope, runtime_.newStrFromUTF32(codepoints));
  EXPECT_TRUE(unicode.equals(SmallStr::fromCodePoint(0xC4)));
}

TEST_F(RuntimeStrTest, NewStrFromUTF32WithLargeNonASCIIReturnsString) {
  HandleScope scope(thread_);
  const int32_t codepoints[] = {0x3041, ' ', 'c', 0xF6,
                                0xF6,   'l', ' ', 0x1F192};
  Str unicode(&scope, runtime_.newStrFromUTF32(codepoints));
  Str expected(&scope, runtime_.newStrFromCStr(
                           "\xe3\x81\x81 c\xC3\xB6\xC3\xB6l \xF0\x9F\x86\x92"));
  EXPECT_TRUE(unicode.equals(*expected));
}

TEST_F(RuntimeTest, HashBools) {
  // In CPython, False hashes to 0 and True hashes to 1.
  EXPECT_TRUE(isIntEqualsWord(runtime_.hash(Bool::falseObj()), 0));
  EXPECT_TRUE(isIntEqualsWord(runtime_.hash(Bool::trueObj()), 1));
}

TEST_F(RuntimeTest, HashLargeBytes) {
  HandleScope scope(thread_);

  // LargeBytes have their hash codes computed lazily.
  const byte src1[] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8};
  LargeBytes arr1(&scope, runtime_.newBytesWithAll(src1));
  EXPECT_EQ(arr1.header().hashCode(), 0);
  word hash1 = SmallInt::cast(runtime_.hash(*arr1)).value();
  EXPECT_NE(arr1.header().hashCode(), 0);
  EXPECT_EQ(arr1.header().hashCode(), hash1);

  word code1 = runtime_.siphash24(src1);
  EXPECT_EQ(code1 & RawHeader::kHashCodeMask, static_cast<uword>(hash1));

  // LargeBytes with different values should (ideally) hash differently.
  const byte src2[] = {0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1};
  LargeBytes arr2(&scope, runtime_.newBytesWithAll(src2));
  word hash2 = SmallInt::cast(runtime_.hash(*arr2)).value();
  EXPECT_NE(hash1, hash2);

  word code2 = runtime_.siphash24(src2);
  EXPECT_EQ(code2 & RawHeader::kHashCodeMask, static_cast<uword>(hash2));

  // LargeBytes with the same value should hash the same.
  LargeBytes arr3(&scope, runtime_.newBytesWithAll(src1));
  EXPECT_NE(arr3, arr1);
  word hash3 = SmallInt::cast(runtime_.hash(*arr3)).value();
  EXPECT_EQ(hash1, hash3);
}

TEST_F(RuntimeTest, HashSmallInts) {
  // In CPython, Ints hash to themselves.
  EXPECT_TRUE(isIntEqualsWord(runtime_.hash(SmallInt::fromWord(123)), 123));
  EXPECT_TRUE(isIntEqualsWord(runtime_.hash(SmallInt::fromWord(456)), 456));
}

TEST_F(RuntimeTest, HashSingletonImmediates) {
  // In CPython, these objects hash to arbitrary values.
  word none_value = NoneType::object().raw();
  RawSmallInt hash_none = SmallInt::cast(runtime_.hash(NoneType::object()));
  EXPECT_EQ(hash_none.value(), none_value);

  word error_value = Error::error().raw();
  RawSmallInt hash_error = SmallInt::cast(runtime_.hash(Error::error()));
  EXPECT_EQ(hash_error.value(), error_value);
}

TEST_F(RuntimeTest, HashStr) {
  HandleScope scope(thread_);

  // LargeStr instances have their hash codes computed lazily.
  Object str1(&scope, runtime_.newStrFromCStr("testing 123"));
  EXPECT_EQ(HeapObject::cast(*str1).header().hashCode(), 0);
  RawSmallInt hash1 = SmallInt::cast(runtime_.hash(*str1));
  EXPECT_NE(HeapObject::cast(*str1).header().hashCode(), 0);
  EXPECT_EQ(HeapObject::cast(*str1).header().hashCode(), hash1.value());

  // Str with different values should (ideally) hash differently.
  Str str2(&scope, runtime_.newStrFromCStr("321 testing"));
  RawSmallInt hash2 = SmallInt::cast(runtime_.hash(*str2));
  EXPECT_NE(hash1, hash2);

  // Strings with the same value should hash the same.
  Str str3(&scope, runtime_.newStrFromCStr("testing 123"));
  RawSmallInt hash3 = SmallInt::cast(runtime_.hash(*str3));
  EXPECT_EQ(hash1, hash3);
}

TEST(RuntimeTestNoFixture, InitializeRandomSetsRandomRandomRNGSeed) {
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

TEST(RuntimeTestNoFixture,
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

TEST_F(RuntimeTest, Random) {
  uword r1 = runtime_.random();
  uword r2 = runtime_.random();
  EXPECT_NE(r1, r2);
  uword r3 = runtime_.random();
  EXPECT_NE(r2, r3);
  uword r4 = runtime_.random();
  EXPECT_NE(r3, r4);
}

TEST_F(RuntimeTest, TrackNativeGcObjectAndUntrackNativeGcObject) {
  ListEntry entry0{nullptr, nullptr};
  ListEntry entry1{nullptr, nullptr};

  EXPECT_TRUE(runtime_.trackNativeGcObject(&entry0));
  EXPECT_TRUE(runtime_.trackNativeGcObject(&entry1));
  // Trying to track an already tracked object returns false.
  EXPECT_FALSE(runtime_.trackNativeGcObject(&entry0));
  EXPECT_FALSE(runtime_.trackNativeGcObject(&entry1));

  EXPECT_TRUE(runtime_.untrackNativeGcObject(&entry0));
  EXPECT_TRUE(runtime_.untrackNativeGcObject(&entry1));

  // Trying to untrack an already untracked object returns false.
  EXPECT_FALSE(runtime_.untrackNativeGcObject(&entry0));
  EXPECT_FALSE(runtime_.untrackNativeGcObject(&entry1));

  // Verify untracked entires are reset to nullptr.
  EXPECT_EQ(entry0.prev, nullptr);
  EXPECT_EQ(entry0.next, nullptr);
  EXPECT_EQ(entry1.prev, nullptr);
  EXPECT_EQ(entry1.next, nullptr);
}

TEST_F(RuntimeTest, HashCodeSizeCheck) {
  RawObject code = newEmptyCode();
  ASSERT_TRUE(code.isHeapObject());
  EXPECT_EQ(HeapObject::cast(code).header().hashCode(), 0);
  // Verify that large-magnitude random numbers are properly
  // truncated to somethat which fits in a SmallInt

  // Conspire based on knoledge of the random number genrated to
  // create a high-magnitude result from Runtime::random
  // which is truncated to 0 for storage in the header and
  // replaced with "1" so no hash code has value 0.
  uword high = uword{1} << (8 * sizeof(uword) - 1);
  uword state[2] = {0, high};
  uword secret[2] = {0, 0};
  runtime_.seedRandom(state, secret);
  uword first = runtime_.random();
  EXPECT_EQ(first, high);
  runtime_.seedRandom(state, secret);
  EXPECT_TRUE(isIntEqualsWord(runtime_.hash(code), 1));
}

TEST_F(RuntimeTest, NewCapacity) {
  // ensure initial capacity
  EXPECT_GE(runtime_.newCapacity(1, 0), 16);

  // grow by factor of 1.5, rounding down
  EXPECT_EQ(runtime_.newCapacity(20, 22), 30);
  EXPECT_EQ(runtime_.newCapacity(64, 77), 96);
  EXPECT_EQ(runtime_.newCapacity(25, 30), 37);

  // ensure growth
  EXPECT_EQ(runtime_.newCapacity(20, 17), 30);
  EXPECT_EQ(runtime_.newCapacity(20, 20), 30);

  // if factor of 1.5 is insufficient, grow exactly to minimum capacity
  EXPECT_EQ(runtime_.newCapacity(20, 40), 40);
  EXPECT_EQ(runtime_.newCapacity(20, 70), 70);

  // capacity has ceiling of SmallInt::kMaxValue
  EXPECT_EQ(runtime_.newCapacity(SmallInt::kMaxValue - 1, SmallInt::kMaxValue),
            SmallInt::kMaxValue);
}

TEST_F(RuntimeTest, InternLargeStr) {
  HandleScope scope(thread_);

  Set interned(&scope, runtime_.interned());

  // Creating an ordinary large string should not affect on the intern table.
  word num_interned = interned.numItems();
  Object str1(&scope, runtime_.newStrFromCStr("hello, world"));
  ASSERT_TRUE(str1.isLargeStr());
  EXPECT_EQ(num_interned, interned.numItems());
  EXPECT_FALSE(setIncludes(thread_, interned, str1));
  EXPECT_FALSE(runtime_.isInternedStr(thread_, str1));

  // Interning the string should add it to the intern table and increase the
  // size of the intern table by one.
  num_interned = interned.numItems();
  Object sym1(&scope, runtime_.internStr(thread_, str1));
  EXPECT_TRUE(setIncludes(thread_, interned, str1));
  EXPECT_EQ(*sym1, *str1);
  EXPECT_EQ(num_interned + 1, interned.numItems());
  EXPECT_TRUE(runtime_.isInternedStr(thread_, str1));

  Object str2(&scope, runtime_.newStrFromCStr("goodbye, world"));
  ASSERT_TRUE(str2.isLargeStr());
  EXPECT_NE(*str1, *str2);

  // Intern another string and make sure we get it back (as opposed to the
  // previously interned string).
  num_interned = interned.numItems();
  Object sym2(&scope, runtime_.internStr(thread_, str2));
  EXPECT_EQ(num_interned + 1, interned.numItems());
  EXPECT_TRUE(setIncludes(thread_, interned, str2));
  EXPECT_EQ(*sym2, *str2);
  EXPECT_NE(*sym1, *sym2);

  // Create a unique copy of a previously created string.
  Object str3(&scope, runtime_.newStrFromCStr("hello, world"));
  ASSERT_TRUE(str3.isLargeStr());
  EXPECT_NE(*str1, *str3);
  EXPECT_TRUE(setIncludes(thread_, interned, str3));
  EXPECT_FALSE(runtime_.isInternedStr(thread_, str3));

  // Interning a duplicate string should not affecct the intern table.
  num_interned = interned.numItems();
  Object sym3(&scope, runtime_.internStr(thread_, str3));
  EXPECT_EQ(num_interned, interned.numItems());
  EXPECT_NE(*sym3, *str3);
  EXPECT_EQ(*sym3, *sym1);
}

TEST_F(RuntimeTest, InternSmallStr) {
  HandleScope scope(thread_);

  Set interned(&scope, runtime_.interned());

  // Creating a small string should not affect the intern table.
  word num_interned = interned.numItems();
  Object str(&scope, runtime_.newStrFromCStr("a"));
  ASSERT_TRUE(str.isSmallStr());
  EXPECT_FALSE(setIncludes(thread_, interned, str));
  EXPECT_EQ(num_interned, interned.numItems());

  // Interning a small string should have no affect on the intern table.
  Object sym(&scope, runtime_.internStr(thread_, str));
  EXPECT_TRUE(sym.isSmallStr());
  EXPECT_FALSE(setIncludes(thread_, interned, str));
  EXPECT_EQ(num_interned, interned.numItems());
  EXPECT_EQ(*sym, *str);
  EXPECT_TRUE(runtime_.isInternedStr(thread_, str));
}

TEST_F(RuntimeTest, InternCStr) {
  HandleScope scope(thread_);

  Set interned(&scope, runtime_.interned());

  word num_interned = interned.numItems();
  Object sym(&scope, runtime_.internStrFromCStr(thread_, "hello, world"));
  EXPECT_TRUE(sym.isStr());
  EXPECT_TRUE(setIncludes(thread_, interned, sym));
  EXPECT_EQ(num_interned + 1, interned.numItems());
  EXPECT_TRUE(runtime_.isInternedStr(thread_, sym));
}

TEST_F(RuntimeTest, IsInternWithInternedStrReturnsTrue) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_.internStrFromCStr(thread_, "hello world"));
  EXPECT_TRUE(runtime_.isInternedStr(thread_, str));
}

TEST_F(RuntimeTest, IsInternWithStrReturnsFalse) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_.newStrFromCStr("hello world"));
  EXPECT_FALSE(runtime_.isInternedStr(thread_, str));
}

TEST_F(RuntimeTest, CollectAttributes) {
  HandleScope scope(thread_);

  Str foo(&scope, runtime_.newStrFromCStr("foo"));
  Str bar(&scope, runtime_.newStrFromCStr("bar"));
  Str baz(&scope, runtime_.newStrFromCStr("baz"));

  Tuple names(&scope, runtime_.newTuple(3));
  names.atPut(0, *foo);
  names.atPut(1, *bar);
  names.atPut(2, *baz);

  Tuple consts(&scope, runtime_.newTuple(4));
  consts.atPut(0, SmallInt::fromWord(100));
  consts.atPut(1, SmallInt::fromWord(200));
  consts.atPut(2, SmallInt::fromWord(300));
  consts.atPut(3, NoneType::object());

  Code code(&scope, newEmptyCode());
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
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Dict attributes(&scope, runtime_.newDict());
  runtime_.collectAttributes(code, attributes);

  // We should have collected a single attribute: 'foo'
  EXPECT_EQ(attributes.numItems(), 1);

  // Check that we collected 'foo'
  Object result(&scope, runtime_.dictAtByStr(thread_, attributes, foo));
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
  code.setCode(runtime_.newBytesWithAll(bc2));
  runtime_.collectAttributes(code, attributes);

  // We should have collected a two more attributes: 'bar' and 'baz'
  EXPECT_EQ(attributes.numItems(), 3);

  // Check that we collected 'bar'
  result = runtime_.dictAtByStr(thread_, attributes, bar);
  ASSERT_TRUE(result.isStr());
  EXPECT_TRUE(Str::cast(*result).equals(*bar));

  // Check that we collected 'baz'
  result = runtime_.dictAtByStr(thread_, attributes, baz);
  ASSERT_TRUE(result.isStr());
  EXPECT_TRUE(Str::cast(*result).equals(*baz));
}

TEST_F(RuntimeTest, CollectAttributesWithExtendedArg) {
  HandleScope scope(thread_);

  Str foo(&scope, runtime_.newStrFromCStr("foo"));
  Str bar(&scope, runtime_.newStrFromCStr("bar"));

  Tuple names(&scope, runtime_.newTuple(2));
  names.atPut(0, *foo);
  names.atPut(1, *bar);

  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, NoneType::object());

  Code code(&scope, newEmptyCode());
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
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Dict attributes(&scope, runtime_.newDict());
  runtime_.collectAttributes(code, attributes);

  // We should have collected a single attribute: 'foo'
  EXPECT_EQ(attributes.numItems(), 1);

  // Check that we collected 'foo'
  Object result(&scope, runtime_.dictAtByStr(thread_, attributes, foo));
  ASSERT_TRUE(result.isStr());
  EXPECT_TRUE(Str::cast(*result).equals(*foo));
}

TEST_F(RuntimeTest, GetTypeConstructor) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_.newType());
  Dict type_dict(&scope, runtime_.newDict());
  type.setDict(*type_dict);

  EXPECT_TRUE(runtime_.classConstructor(type).isErrorNotFound());

  Str init(&scope, runtime_.symbols()->DunderInit());
  Object func(&scope, makeTestFunction());
  runtime_.dictAtPutInValueCellByStr(thread_, type_dict, init, func);

  EXPECT_EQ(runtime_.classConstructor(type), *func);
}

TEST_F(RuntimeTest, NewInstanceEmptyClass) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, "class MyEmptyClass: pass").isError());

  Type type(&scope, mainModuleAt(&runtime_, "MyEmptyClass"));
  Layout layout(&scope, type.instanceLayout());
  EXPECT_EQ(layout.instanceSize(), 1 * kPointerSize);

  Type cls(&scope, layout.describedType());
  EXPECT_TRUE(isStrEqualsCStr(cls.name(), "MyEmptyClass"));

  Instance instance(&scope, runtime_.newInstance(layout));
  EXPECT_TRUE(instance.isInstance());
  EXPECT_EQ(instance.header().layoutId(), layout.id());
}

TEST_F(RuntimeTest, NewInstanceManyAttributes) {
  HandleScope scope(thread_);

  const char* src = R"(
class MyTypeWithAttributes():
  def __init__(self):
    self.a = 1
    self.b = 2
    self.c = 3
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());

  Type type(&scope, mainModuleAt(&runtime_, "MyTypeWithAttributes"));
  Layout layout(&scope, type.instanceLayout());
  ASSERT_EQ(layout.instanceSize(), 4 * kPointerSize);

  Type cls(&scope, layout.describedType());
  EXPECT_TRUE(isStrEqualsCStr(cls.name(), "MyTypeWithAttributes"));

  Instance instance(&scope, runtime_.newInstance(layout));
  EXPECT_TRUE(instance.isInstance());
  EXPECT_EQ(instance.header().layoutId(), layout.id());
}

TEST_F(RuntimeTest, VerifySymbols) {
  HandleScope scope(thread_);
  Symbols* symbols = runtime_.symbols();
  Object value(&scope, NoneType::object());
  for (int i = 0; i < static_cast<int>(SymbolId::kMaxId); i++) {
    SymbolId id = static_cast<SymbolId>(i);
    value = symbols->at(id);
    ASSERT_TRUE(value.isStr());
    const char* expected = Symbols::predefinedSymbolAt(id);
    EXPECT_TRUE(runtime_.isInternedStr(thread_, value))
        << "at symbol " << expected;
    EXPECT_TRUE(Str::cast(*value).equalsCStr(expected))
        << "Incorrect symbol value for " << expected;
  }
}

static RawStr className(Runtime* runtime, RawObject o) {
  auto cls = Type::cast(runtime->typeOf(o));
  auto name = Str::cast(cls.name());
  return name;
}

TEST_F(RuntimeTest, TypeIds) {
  EXPECT_TRUE(isStrEqualsCStr(className(&runtime_, Bool::trueObj()), "bool"));
  EXPECT_TRUE(
      isStrEqualsCStr(className(&runtime_, NoneType::object()), "NoneType"));
  EXPECT_TRUE(isStrEqualsCStr(
      className(&runtime_, runtime_.newStrFromCStr("abc")), "str"));
  for (word i = 0; i < 16; i++) {
    EXPECT_TRUE(
        isStrEqualsCStr(className(&runtime_, SmallInt::fromWord(i)), "int"))
        << i;
  }
}

TEST_F(RuntimeTest, CallRunTwice) {
  ASSERT_FALSE(runFromCStr(&runtime_, "x = 42").isError());
  ASSERT_FALSE(runFromCStr(&runtime_, "y = 1764").isError());

  HandleScope scope(thread_);
  Object x(&scope, mainModuleAt(&runtime_, "x"));
  EXPECT_TRUE(isIntEqualsWord(*x, 42));
  Object y(&scope, mainModuleAt(&runtime_, "y"));
  EXPECT_TRUE(isIntEqualsWord(*y, 1764));
}

TEST_F(RuntimeStrTest, StrConcat) {
  HandleScope scope(thread_);

  Str str1(&scope, runtime_.newStrFromCStr("abc"));
  Str str2(&scope, runtime_.newStrFromCStr("def"));

  // Large strings.
  Str str3(&scope, runtime_.newStrFromCStr("0123456789abcdef"));
  Str str4(&scope, runtime_.newStrFromCStr("fedbca9876543210"));

  Object concat12(&scope, runtime_.strConcat(thread_, str1, str2));
  Object concat34(&scope, runtime_.strConcat(thread_, str3, str4));

  Object concat13(&scope, runtime_.strConcat(thread_, str1, str3));
  Object concat31(&scope, runtime_.strConcat(thread_, str3, str1));

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

TEST_F(RuntimeTypeCallTest, TypeCallNoInitMethod) {
  HandleScope scope(thread_);

  const char* src = R"(
class MyTypeWithNoInitMethod():
  def m(self):
    pass

c = MyTypeWithNoInitMethod()
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());

  Object instance(&scope, mainModuleAt(&runtime_, "c"));
  ASSERT_TRUE(instance.isInstance());
  LayoutId layout_id = instance.layoutId();
  Layout layout(&scope, runtime_.layoutAt(layout_id));
  EXPECT_EQ(layout.instanceSize(), 1 * kPointerSize);

  Type cls(&scope, layout.describedType());
  EXPECT_TRUE(isStrEqualsCStr(cls.name(), "MyTypeWithNoInitMethod"));
}

TEST_F(RuntimeTypeCallTest, TypeCallEmptyInitMethod) {
  HandleScope scope(thread_);

  const char* src = R"(
class MyTypeWithEmptyInitMethod():
  def __init__(self):
    pass
  def m(self):
    pass

c = MyTypeWithEmptyInitMethod()
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());

  Object instance(&scope, mainModuleAt(&runtime_, "c"));
  ASSERT_TRUE(instance.isInstance());
  LayoutId layout_id = instance.layoutId();
  Layout layout(&scope, runtime_.layoutAt(layout_id));
  EXPECT_EQ(layout.instanceSize(), 1 * kPointerSize);

  Type cls(&scope, layout.describedType());
  EXPECT_TRUE(isStrEqualsCStr(cls.name(), "MyTypeWithEmptyInitMethod"));
}

TEST_F(RuntimeTypeCallTest, TypeCallWithArguments) {
  HandleScope scope(thread_);

  const char* src = R"(
class MyTypeWithAttributes():
  def __init__(self, x):
    self.x = x
  def m(self):
    pass

c = MyTypeWithAttributes(1)
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());

  Type type(&scope, mainModuleAt(&runtime_, "MyTypeWithAttributes"));
  Object instance(&scope, mainModuleAt(&runtime_, "c"));
  ASSERT_TRUE(instance.isInstance());
  LayoutId layout_id = instance.layoutId();
  // Since this class has extra attributes, its layout id should be greater than
  // the layout id from the type.
  ASSERT_GT(layout_id, Layout::cast(type.instanceLayout()).id());
  Layout layout(&scope, runtime_.layoutAt(layout_id));
  ASSERT_EQ(layout.instanceSize(), 2 * kPointerSize);

  Type cls(&scope, layout.describedType());
  EXPECT_TRUE(isStrEqualsCStr(cls.name(), "MyTypeWithAttributes"));

  Object name(&scope, runtime_.newStrFromCStr("x"));
  Object value(&scope, runtime_.attributeAt(thread_, instance, name));
  EXPECT_FALSE(value.isError());
  EXPECT_EQ(*value, SmallInt::fromWord(1));
}

TEST_F(RuntimeTest, ComputeLineNumberForBytecodeOffset) {
  const char* src = R"(
def func():
  a = 1
  b = 2
  print(a, b)
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  HandleScope scope(thread_);

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

  Function func(&scope, mainModuleAt(&runtime_, "func"));
  Code code(&scope, func.code());
  ASSERT_EQ(code.firstlineno(), 2);

  // a = 1
  EXPECT_EQ(runtime_.codeOffsetToLineNum(thread_, code, 0), 3);
  EXPECT_EQ(runtime_.codeOffsetToLineNum(thread_, code, 2), 3);

  // b = 2
  EXPECT_EQ(runtime_.codeOffsetToLineNum(thread_, code, 4), 4);
  EXPECT_EQ(runtime_.codeOffsetToLineNum(thread_, code, 6), 4);

  // print(a, b)
  for (word i = 8; i < Bytes::cast(code.code()).length(); i++) {
    EXPECT_EQ(runtime_.codeOffsetToLineNum(thread_, code, i), 5);
  }
}

TEST_F(RuntimeTest, IsInstanceOf) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runtime_.isInstanceOfInt(NoneType::object()));

  Object i(&scope, runtime_.newInt(123));
  EXPECT_TRUE(i.isInt());
  EXPECT_FALSE(runtime_.isInstanceOfStr(*i));

  Object str(&scope, runtime_.newStrFromCStr("this is a long string"));
  EXPECT_TRUE(runtime_.isInstanceOfStr(*str));
  EXPECT_FALSE(str.isInt());

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class StopIterationSub(StopIteration):
  pass
stop_iteration = StopIterationSub()
  )")
                   .isError());
  Object stop_iteration(&scope, mainModuleAt(&runtime_, "stop_iteration"));
  EXPECT_TRUE(runtime_.isInstanceOfStopIteration(*stop_iteration));
  EXPECT_TRUE(runtime_.isInstanceOfBaseException(*stop_iteration));
  EXPECT_FALSE(runtime_.isInstanceOfSystemExit(*stop_iteration));
}

TEST_F(RuntimeTest, IsInstanceOfUserBaseAcceptsMetaclassInstances) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
class M(type):
  pass
class IS(int, metaclass=M):
  pass
i = IS()
)")
                   .isError());
  Object i(&scope, mainModuleAt(&runtime_, "i"));
  EXPECT_TRUE(runtime_.isInstanceOfUserIntBase(*i));
  EXPECT_FALSE(runtime_.isInstanceOfUserStrBase(*i));
}

TEST_F(RuntimeTupleTest, Create) {
  RawObject obj0 = runtime_.newTuple(0);
  ASSERT_TRUE(obj0.isTuple());
  RawTuple array0 = Tuple::cast(obj0);
  EXPECT_EQ(array0.length(), 0);

  RawObject obj1 = runtime_.newTuple(1);
  ASSERT_TRUE(obj1.isTuple());
  RawTuple array1 = Tuple::cast(obj1);
  EXPECT_EQ(array1.length(), 1);

  RawObject obj7 = runtime_.newTuple(7);
  ASSERT_TRUE(obj7.isTuple());
  RawTuple array7 = Tuple::cast(obj7);
  EXPECT_EQ(array7.length(), 7);

  RawObject obj8 = runtime_.newTuple(8);
  ASSERT_TRUE(obj8.isTuple());
  RawTuple array8 = Tuple::cast(obj8);
  EXPECT_EQ(array8.length(), 8);
}

TEST_F(RuntimeSetTest, EmptySetInvariants) {
  HandleScope scope(thread_);
  Set set(&scope, runtime_.newSet());

  EXPECT_EQ(set.numItems(), 0);
  ASSERT_TRUE(set.isSet());
  ASSERT_TRUE(set.data().isTuple());
  EXPECT_EQ(Tuple::cast(set.data()).length(), 0);
}

TEST_F(RuntimeSetTest, Add) {
  HandleScope scope(thread_);
  Set set(&scope, runtime_.newSet());
  Object value(&scope, SmallInt::fromWord(12345));
  Object value_hash(&scope, Interpreter::hash(thread_, value));
  ASSERT_FALSE(value_hash.isErrorException());

  // Store a value
  runtime_.setAdd(thread_, set, value, value_hash);
  EXPECT_EQ(set.numItems(), 1);

  // Retrieve the stored value
  ASSERT_TRUE(setIncludes(thread_, set, value));

  // Add a new value
  Object new_value(&scope, SmallInt::fromWord(5555));
  Object new_value_hash(&scope, Interpreter::hash(thread_, new_value));
  ASSERT_FALSE(new_value_hash.isErrorException());
  runtime_.setAdd(thread_, set, new_value, new_value_hash);
  EXPECT_EQ(set.numItems(), 2);

  // Get the new value
  ASSERT_TRUE(setIncludes(thread_, set, new_value));

  // Add a existing value
  Object same_value(&scope, SmallInt::fromWord(12345));
  Object same_value_hash(&scope, Interpreter::hash(thread_, same_value));
  ASSERT_FALSE(same_value_hash.isErrorException());
  RawObject old_value =
      runtime_.setAdd(thread_, set, same_value, same_value_hash);
  EXPECT_EQ(set.numItems(), 2);
  EXPECT_EQ(old_value, *value);
}

TEST_F(RuntimeSetTest, Remove) {
  HandleScope scope(thread_);
  Set set(&scope, runtime_.newSet());
  Object value(&scope, SmallInt::fromWord(12345));
  Object value_hash(&scope, Interpreter::hash(thread_, value));

  // Removing a key that doesn't exist should fail
  EXPECT_FALSE(runtime_.setRemove(thread_, set, value, value_hash));

  setHashAndAdd(thread_, set, value);
  EXPECT_EQ(set.numItems(), 1);

  ASSERT_TRUE(runtime_.setRemove(thread_, set, value, value_hash));
  EXPECT_EQ(set.numItems(), 0);

  // Looking up a key that was deleted should fail
  ASSERT_FALSE(setIncludes(thread_, set, value));
}

static RawObject makeKey(Runtime* runtime, int i) {
  byte text[]{"0123456789abcdeghiklmn"};
  return runtime->newStrWithAll(View<byte>(text + i % 10, 10));
}

TEST_F(RuntimeSetTest, Grow) {
  HandleScope scope(thread_);
  Set set(&scope, runtime_.newSet());

  // Fill up the dict - we insert an initial key to force the allocation of the
  // backing Tuple.
  Object init_key(&scope, SmallInt::fromWord(0));
  setHashAndAdd(thread_, set, init_key);
  ASSERT_TRUE(set.data().isTuple());
  word init_data_size = Tuple::cast(set.data()).length();

  // Fill in one fewer keys than would require growing the underlying object
  // array again
  word num_keys = Runtime::kInitialSetCapacity;
  for (int i = 1; i < num_keys; i++) {
    Object key(&scope, makeKey(&runtime_, i));
    setHashAndAdd(thread_, set, key);
  }

  // Add another key which should force us to double the capacity
  Object straw(&scope, makeKey(&runtime_, num_keys));
  setHashAndAdd(thread_, set, straw);
  ASSERT_TRUE(set.data().isTuple());
  word new_data_size = Tuple::cast(set.data()).length();
  EXPECT_EQ(new_data_size, Runtime::kSetGrowthFactor * init_data_size);

  // Make sure we can still read all the stored keys
  for (int i = 1; i <= num_keys; i++) {
    Object key(&scope, makeKey(&runtime_, i));
    bool found = setIncludes(thread_, set, key);
    ASSERT_TRUE(found);
  }
}

TEST_F(RuntimeSetTest, UpdateSet) {
  HandleScope scope(thread_);
  Set set(&scope, runtime_.newSet());
  Set set1(&scope, runtime_.newSet());
  Object set1_handle(&scope, *set1);
  for (word i = 0; i < 8; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    setHashAndAdd(thread_, set, value);
  }
  runtime_.setUpdate(thread_, set, set1_handle);
  ASSERT_EQ(set.numItems(), 8);
  for (word i = 4; i < 12; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    setHashAndAdd(thread_, set1, value);
  }
  runtime_.setUpdate(thread_, set, set1_handle);
  ASSERT_EQ(set.numItems(), 12);
  runtime_.setUpdate(thread_, set, set1_handle);
  ASSERT_EQ(set.numItems(), 12);
}

TEST_F(RuntimeSetTest, UpdateList) {
  HandleScope scope(thread_);
  List list(&scope, runtime_.newList());
  Set set(&scope, runtime_.newSet());
  for (word i = 0; i < 8; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime_.listAdd(thread_, list, value);
  }
  for (word i = 4; i < 12; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    setHashAndAdd(thread_, set, value);
  }
  ASSERT_EQ(set.numItems(), 8);
  Object list_handle(&scope, *list);
  runtime_.setUpdate(thread_, set, list_handle);
  ASSERT_EQ(set.numItems(), 12);
  runtime_.setUpdate(thread_, set, list_handle);
  ASSERT_EQ(set.numItems(), 12);
}

TEST_F(RuntimeSetTest, UpdateListIterator) {
  HandleScope scope(thread_);
  List list(&scope, runtime_.newList());
  Set set(&scope, runtime_.newSet());
  for (word i = 0; i < 8; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime_.listAdd(thread_, list, value);
  }
  for (word i = 4; i < 12; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    setHashAndAdd(thread_, set, value);
  }
  ASSERT_EQ(set.numItems(), 8);
  Object list_handle(&scope, *list);
  Object list_iterator(&scope, runtime_.newListIterator(list_handle));
  runtime_.setUpdate(thread_, set, list_iterator);
  ASSERT_EQ(set.numItems(), 12);
}

TEST_F(RuntimeSetTest, UpdateTuple) {
  HandleScope scope(thread_);
  Tuple object_array(&scope, runtime_.newTuple(8));
  Set set(&scope, runtime_.newSet());
  for (word i = 0; i < 8; i++) {
    object_array.atPut(i, SmallInt::fromWord(i));
  }
  for (word i = 4; i < 12; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    setHashAndAdd(thread_, set, value);
  }
  ASSERT_EQ(set.numItems(), 8);
  Object object_array_handle(&scope, *object_array);
  runtime_.setUpdate(thread_, set, object_array_handle);
  ASSERT_EQ(set.numItems(), 12);
}

TEST_F(RuntimeSetTest, UpdateIterator) {
  HandleScope scope(thread_);
  Set set(&scope, runtime_.newSet());
  Int one(&scope, SmallInt::fromWord(1));
  Int four(&scope, SmallInt::fromWord(4));
  Object iterable(&scope, runtime_.newRange(one, four, one));
  runtime_.setUpdate(thread_, set, iterable);

  ASSERT_EQ(set.numItems(), 3);
}

TEST_F(RuntimeSetTest, UpdateWithNonIterable) {
  HandleScope scope(thread_);
  Set set(&scope, runtime_.newSet());
  Object non_iterable(&scope, NoneType::object());
  Object result(&scope, runtime_.setUpdate(thread_, set, non_iterable));
  ASSERT_TRUE(result.isError());
}

TEST_F(RuntimeSetTest, EmptySetItersectionReturnsEmptySet) {
  HandleScope scope(thread_);
  Set set(&scope, runtime_.newSet());
  Set set1(&scope, runtime_.newSet());

  // set() & set()
  Object result(&scope, runtime_.setIntersection(thread_, set, set1));
  ASSERT_TRUE(result.isSet());
  EXPECT_EQ(Set::cast(*result).numItems(), 0);
}

TEST_F(RuntimeSetTest, ItersectionWithEmptySetReturnsEmptySet) {
  HandleScope scope(thread_);
  Set set(&scope, runtime_.newSet());
  Set set1(&scope, runtime_.newSet());

  for (word i = 0; i < 8; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    setHashAndAdd(thread_, set1, value);
  }

  // set() & {0, 1, 2, 3, 4, 5, 6, 7}
  Object result(&scope, runtime_.setIntersection(thread_, set, set1));
  ASSERT_TRUE(result.isSet());
  EXPECT_EQ(Set::cast(*result).numItems(), 0);

  // {0, 1, 2, 3, 4, 5, 6, 7} & set()
  Object result1(&scope, runtime_.setIntersection(thread_, set1, set));
  ASSERT_TRUE(result1.isSet());
  EXPECT_EQ(Set::cast(*result1).numItems(), 0);
}

TEST_F(RuntimeSetTest, IntersectionReturnsSetWithCommonElements) {
  HandleScope scope(thread_);
  Set set(&scope, runtime_.newSet());
  Set set1(&scope, runtime_.newSet());
  Object key(&scope, NoneType::object());

  for (word i = 0; i < 8; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    setHashAndAdd(thread_, set1, value);
  }

  for (word i = 0; i < 4; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    setHashAndAdd(thread_, set, value);
  }

  // {0, 1, 2, 3} & {0, 1, 2, 3, 4, 5, 6, 7}
  Set result(&scope, runtime_.setIntersection(thread_, set, set1));
  EXPECT_EQ(Set::cast(*result).numItems(), 4);
  key = SmallInt::fromWord(0);
  EXPECT_TRUE(setIncludes(thread_, result, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(setIncludes(thread_, result, key));
  key = SmallInt::fromWord(2);
  EXPECT_TRUE(setIncludes(thread_, result, key));
  key = SmallInt::fromWord(3);
  EXPECT_TRUE(setIncludes(thread_, result, key));

  // {0, 1, 2, 3, 4, 5, 6, 7} & {0, 1, 2, 3}
  Set result1(&scope, runtime_.setIntersection(thread_, set, set1));
  EXPECT_EQ(Set::cast(*result1).numItems(), 4);
  key = SmallInt::fromWord(0);
  EXPECT_TRUE(setIncludes(thread_, result1, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(setIncludes(thread_, result1, key));
  key = SmallInt::fromWord(2);
  EXPECT_TRUE(setIncludes(thread_, result1, key));
  key = SmallInt::fromWord(3);
  EXPECT_TRUE(setIncludes(thread_, result1, key));
}

TEST_F(RuntimeSetTest, IntersectIterator) {
  HandleScope scope(thread_);
  Set set(&scope, runtime_.newSet());
  Int one(&scope, SmallInt::fromWord(1));
  Int four(&scope, SmallInt::fromWord(4));
  Object iterable(&scope, runtime_.newRange(one, four, one));
  Set result(&scope, runtime_.setIntersection(thread_, set, iterable));
  EXPECT_EQ(result.numItems(), 0);

  Object key(&scope, SmallInt::fromWord(1));
  setHashAndAdd(thread_, set, key);
  key = SmallInt::fromWord(2);
  setHashAndAdd(thread_, set, key);
  Set result1(&scope, runtime_.setIntersection(thread_, set, iterable));
  EXPECT_EQ(result1.numItems(), 2);
  EXPECT_TRUE(setIncludes(thread_, result1, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(setIncludes(thread_, result1, key));
}

TEST_F(RuntimeSetTest, IntersectWithNonIterable) {
  HandleScope scope(thread_);
  Set set(&scope, runtime_.newSet());
  Object non_iterable(&scope, NoneType::object());

  Object result(&scope, runtime_.setIntersection(thread_, set, non_iterable));
  ASSERT_TRUE(result.isError());
}

// Attribute tests

// Set an attribute defined in __init__
TEST_F(RuntimeAttributeTest, SetInstanceAttribute) {
  const char* src = R"(
class Foo:
  def __init__(self):
    self.attr = 'testing 123'

def test(x):
  result = []
  Foo.__init__(x)
  result.append(x.attr)
  x.attr = '321 testing'
  result.append(x.attr)
  return result
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());

  // Create the instance
  HandleScope scope(thread_);
  Type type(&scope, mainModuleAt(&runtime_, "Foo"));
  Layout layout(&scope, type.instanceLayout());
  Object instance(&scope, runtime_.newInstance(layout));

  // Run __init__ then RMW the attribute
  Function test(&scope, mainModuleAt(&runtime_, "test"));
  Object result(&scope, Interpreter::callFunction1(
                            thread_, thread_->currentFrame(), test, instance));
  EXPECT_PYLIST_EQ(result, {"testing 123", "321 testing"});
}

TEST_F(RuntimeAttributeTest, AddOverflowAttributes) {
  const char* src = R"(
class Foo:
  pass

def test(x):
  result = []
  x.foo = 100
  x.bar = 200
  x.baz = 'hello'
  result.append(x.foo)
  result.append(x.bar)
  result.append(x.baz)

  x.foo = 'aaa'
  x.bar = 'bbb'
  x.baz = 'ccc'
  result.append(x.foo)
  result.append(x.bar)
  result.append(x.baz)
  return result
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());

  // Create an instance of Foo
  HandleScope scope(thread_);
  Type type(&scope, mainModuleAt(&runtime_, "Foo"));
  Layout layout(&scope, type.instanceLayout());
  Instance foo1(&scope, runtime_.newInstance(layout));
  LayoutId original_layout_id = layout.id();

  // Add overflow attributes that should force layout transitions
  Function test(&scope, mainModuleAt(&runtime_, "test"));
  Object result0(&scope, Interpreter::callFunction1(
                             thread_, thread_->currentFrame(), test, foo1));
  EXPECT_PYLIST_EQ(result0, {100, 200, "hello", "aaa", "bbb", "ccc"});
  EXPECT_NE(foo1.layoutId(), original_layout_id);

  // Add the same set of attributes to a new instance, should arrive at the
  // same layout
  Instance foo2(&scope, runtime_.newInstance(layout));
  Object result1(&scope, Interpreter::callFunction1(
                             thread_, thread_->currentFrame(), test, foo2));
  EXPECT_PYLIST_EQ(result1, {100, 200, "hello", "aaa", "bbb", "ccc"});
}

TEST_F(RuntimeAttributeTest, ManipulateMultipleAttributes) {
  const char* src = R"(
class Foo:
  def __init__(self):
    self.foo = 'foo'
    self.bar = 'bar'
    self.baz = 'baz'

def test(x):
  result = []
  Foo.__init__(x)
  result.append(x.foo)
  result.append(x.bar)
  result.append(x.baz)
  x.foo = 'aaa'
  x.bar = 'bbb'
  x.baz = 'ccc'
  result.append(x.foo)
  result.append(x.bar)
  result.append(x.baz)
  return result
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());

  // Create the instance
  HandleScope scope(thread_);
  Type type(&scope, mainModuleAt(&runtime_, "Foo"));
  Layout layout(&scope, type.instanceLayout());
  Object instance(&scope, runtime_.newInstance(layout));

  // Run the test
  Function test(&scope, mainModuleAt(&runtime_, "test"));
  Object result(&scope, Interpreter::callFunction1(
                            thread_, thread_->currentFrame(), test, instance));
  EXPECT_PYLIST_EQ(result, {"foo", "bar", "baz", "aaa", "bbb", "ccc"});
}

TEST_F(RuntimeAttributeTest, FetchConditionalInstanceAttribute) {
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
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src),
                            LayoutId::kAttributeError,
                            "'Foo' object has no attribute 'bar'"));
}

TEST_F(RuntimeAttributeTest, DunderNewOnInstance) {
  const char* src = R"(
result = []
class Foo:
    def __new__(cls):
        result.append("New")
        return object.__new__(cls)
    def __init__(self):
        result.append("Init")
Foo()
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {"New", "Init"});
}

TEST_F(RuntimeAttributeTest, NoInstanceDictReturnsClassAttribute) {
  HandleScope scope(thread_);
  Object immediate(&scope, SmallInt::fromWord(-1));
  Object name(&scope, runtime_.symbols()->DunderNeg());
  RawObject attr = runtime_.attributeAt(thread_, immediate, name);
  ASSERT_TRUE(attr.isBoundMethod());
}

TEST_F(RuntimeAttributeTest, DeleteKnownAttribute) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
    def __init__(self):
      self.foo = 'foo'
      self.bar = 'bar'

def test():
    foo = Foo()
    del foo.bar
)")
                   .isError());
  HandleScope scope(thread_);
  Function test(&scope, mainModuleAt(&runtime_, "test"));
  Tuple args(&scope, runtime_.emptyTuple());
  Object result(&scope, callFunction(test, args));
  EXPECT_EQ(*result, NoneType::object());
}

TEST_F(RuntimeAttributeTest, DeleteDescriptor) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
)")
                   .isError());
  HandleScope scope(thread_);
  Object data(&scope, mainModuleAt(&runtime_, "result"));
  ASSERT_TRUE(data.isTuple());

  Tuple result(&scope, *data);
  ASSERT_EQ(result.length(), 2);

  Object descr(&scope, mainModuleAt(&runtime_, "descr"));
  EXPECT_EQ(result.at(0), *descr);

  Object foo(&scope, mainModuleAt(&runtime_, "foo"));
  EXPECT_EQ(result.at(1), *foo);
}

TEST_F(RuntimeAttributeTest, DeleteUnknownAttribute) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class Foo:
    pass

foo = Foo()
del foo.bar
)"),
                            LayoutId::kAttributeError,
                            "'Foo' object has no attribute 'bar'"));
}

TEST_F(RuntimeAttributeTest, DeleteAttributeWithDunderDelattr) {
  HandleScope scope(thread_);
  const char* src = R"(
result = None

class Foo:
    def __delattr__(self, name):
        global result
        result = self, name

foo = Foo()
del foo.bar
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Object data(&scope, mainModuleAt(&runtime_, "result"));
  ASSERT_TRUE(data.isTuple());

  Tuple result(&scope, *data);
  ASSERT_EQ(result.length(), 2);

  Object foo(&scope, mainModuleAt(&runtime_, "foo"));
  EXPECT_EQ(result.at(0), *foo);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "bar"));
}

TEST_F(RuntimeAttributeTest, DeleteAttributeWithDunderDelattrOnSuperclass) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = None

class Foo:
    def __delattr__(self, name):
        global result
        result = self, name

class Bar(Foo):
    pass

bar = Bar()
del bar.baz
)")
                   .isError());
  HandleScope scope(thread_);
  Object data(&scope, mainModuleAt(&runtime_, "result"));
  ASSERT_TRUE(data.isTuple());

  Tuple result(&scope, *data);
  ASSERT_EQ(result.length(), 2);

  Object bar(&scope, mainModuleAt(&runtime_, "bar"));
  EXPECT_EQ(result.at(0), *bar);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "baz"));
}

TEST_F(RuntimeClassAttrTest, DeleteKnownAttribute) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
    foo = 'foo'
    bar = 'bar'

def test():
    del Foo.bar
)")
                   .isError());
  HandleScope scope(thread_);
  Function test(&scope, mainModuleAt(&runtime_, "test"));
  Tuple args(&scope, runtime_.emptyTuple());
  Object result(&scope, callFunction(test, args));
  EXPECT_EQ(*result, NoneType::object());
}

TEST_F(RuntimeAttributeTest, DeleteDescriptorOnMetaclass) {
  HandleScope scope(thread_);
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
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Object data(&scope, mainModuleAt(&runtime_, "args"));
  ASSERT_TRUE(data.isTuple());

  Tuple args(&scope, *data);
  ASSERT_EQ(args.length(), 2);

  Object descr(&scope, mainModuleAt(&runtime_, "descr"));
  EXPECT_EQ(args.at(0), *descr);

  Object foo(&scope, mainModuleAt(&runtime_, "Foo"));
  EXPECT_EQ(args.at(1), *foo);
}

TEST_F(RuntimeAttributeTest, DeleteUnknownClassAttribute) {
  const char* src = R"(
class Foo:
    pass

del Foo.bar
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src),
                            LayoutId::kAttributeError,
                            "type object 'Foo' has no attribute 'bar'"));
}

TEST_F(RuntimeAttributeTest, DeleteClassAttributeWithDunderDelattrOnMetaclass) {
  HandleScope scope(thread_);
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
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Object data(&scope, mainModuleAt(&runtime_, "args"));
  ASSERT_TRUE(data.isTuple());

  Tuple args(&scope, *data);
  ASSERT_EQ(args.length(), 2);

  Object foo(&scope, mainModuleAt(&runtime_, "Foo"));
  EXPECT_EQ(args.at(0), *foo);

  Object attr(&scope, runtime_.internStrFromCStr(thread_, "bar"));
  EXPECT_EQ(args.at(1), *attr);
}

TEST_F(
    RuntimeTest,
    DeleteClassAttributeWithUnimplementedCacheInvalidationTerminatesPyroWhenCacheIsEnabled) {
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __len__(self): return 4

del C.__len__
)")
                   .isError());
  ASSERT_DEATH(static_cast<void>(runFromCStr(&runtime_, R"(
class C:
  def __setattr__(self, other): return 4

del C.__setattr__
)")),
               "unimplemented cache invalidation for type.__setattr__ update");
}

TEST(
    RuntimeTestNoFixture,
    DeleteClassAttributeWithUnimplementedCacheInvalidationDoesNotTerminatesPyroWhenCacheIsDisabled) {
  Runtime runtime(/*cache_enabled=*/false);
  EXPECT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __add__(self, other): return 4

del C.__add__
)")
                   .isError());
}

TEST_F(RuntimeModuleAttrTest, DeleteUnknownAttribute) {
  HandleScope scope(thread_);
  const char* src = R"(
def test(module):
    del module.foo
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Function test(&scope, mainModuleAt(&runtime_, "test"));
  Tuple args(&scope, runtime_.newTuple(1));
  args.atPut(0, findMainModule(&runtime_));
  EXPECT_TRUE(raised(callFunction(test, args), LayoutId::kAttributeError));
}

TEST_F(RuntimeModuleAttrTest, DeleteKnownAttribute) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
foo = 'testing 123'

def test(module):
    del module.foo
    return 123
)")
                   .isError());
  HandleScope scope(thread_);
  Function test(&scope, mainModuleAt(&runtime_, "test"));
  Tuple args(&scope, runtime_.newTuple(1));
  args.atPut(0, findMainModule(&runtime_));
  EXPECT_EQ(callFunction(test, args), SmallInt::fromWord(123));

  Object attr(&scope, runtime_.newStrFromCStr("foo"));
  Object module(&scope, findMainModule(&runtime_));
  EXPECT_TRUE(runtime_.attributeAt(thread_, module, attr).isError());
}

TEST_F(RuntimeIntTest, NewSmallIntWithDigits) {
  HandleScope scope(thread_);

  Int zero(&scope, runtime_.newIntWithDigits(View<uword>(nullptr, 0)));
  EXPECT_TRUE(isIntEqualsWord(*zero, 0));

  uword digit = 1;
  RawObject one = runtime_.newIntWithDigits(View<uword>(&digit, 1));
  EXPECT_TRUE(isIntEqualsWord(one, 1));

  digit = kMaxUword;
  RawObject negative_one = runtime_.newIntWithDigits(View<uword>(&digit, 1));
  EXPECT_TRUE(isIntEqualsWord(negative_one, -1));

  word min_small_int = RawSmallInt::kMaxValue;
  digit = static_cast<uword>(min_small_int);
  Int min_smallint(&scope, runtime_.newIntWithDigits(View<uword>(&digit, 1)));
  EXPECT_TRUE(isIntEqualsWord(*min_smallint, min_small_int));

  word max_small_int = RawSmallInt::kMaxValue;
  digit = static_cast<uword>(max_small_int);
  Int max_smallint(&scope, runtime_.newIntWithDigits(View<uword>(&digit, 1)));
  EXPECT_TRUE(isIntEqualsWord(*max_smallint, max_small_int));
}

TEST_F(RuntimeIntTest, NewLargeIntWithDigits) {
  HandleScope scope(thread_);

  word negative_large_int = RawSmallInt::kMinValue - 1;
  uword digit = static_cast<uword>(negative_large_int);
  Int negative_largeint(&scope,
                        runtime_.newIntWithDigits(View<uword>(&digit, 1)));
  EXPECT_TRUE(isIntEqualsWord(*negative_largeint, negative_large_int));

  word positive_large_int = RawSmallInt::kMaxValue + 1;
  digit = static_cast<uword>(positive_large_int);
  Int positive_largeint(&scope,
                        runtime_.newIntWithDigits(View<uword>(&digit, 1)));
  EXPECT_TRUE(isIntEqualsWord(*positive_largeint, positive_large_int));
}

TEST_F(RuntimeIntTest, BinaryAndWithSmallInts) {
  HandleScope scope(thread_);
  Int left(&scope, SmallInt::fromWord(0xEA));   // 0b11101010
  Int right(&scope, SmallInt::fromWord(0xDC));  // 0b11011100
  Object result(&scope, runtime_.intBinaryAnd(thread_, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xC8));  // 0b11001000
}

TEST_F(RuntimeIntTest, BinaryAndWithLargeInts) {
  HandleScope scope(thread_);
  // {0b00001111, 0b00110000, 0b00000001}
  const uword digits_left[] = {0x0F, 0x30, 0x1};
  Int left(&scope, newIntWithDigits(&runtime_, digits_left));
  // {0b00000011, 0b11110000, 0b00000010, 0b00000111}
  const uword digits_right[] = {0x03, 0xF0, 0x2, 0x7};
  Int right(&scope, newIntWithDigits(&runtime_, digits_right));
  Object result(&scope, runtime_.intBinaryAnd(thread_, left, right));
  // {0b00000111, 0b01110000}
  const uword expected_digits[] = {0x03, 0x30};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));

  Object result_commuted(&scope, runtime_.intBinaryAnd(thread_, right, left));
  EXPECT_TRUE(isIntEqualsDigits(*result_commuted, expected_digits));
}

TEST_F(RuntimeIntTest, BinaryAndWithNegativeLargeInts) {
  HandleScope scope(thread_);

  Int left(&scope, SmallInt::fromWord(-42));  // 0b11010110
  const uword digits[] = {static_cast<uword>(-1), 0xF0, 0x2, 0x7};
  Int right(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runtime_.intBinaryAnd(thread_, left, right));
  const uword expected_digits[] = {static_cast<uword>(-42), 0xF0, 0x2, 0x7};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(RuntimeIntTest, BinaryOrWithSmallInts) {
  HandleScope scope(thread_);
  Int left(&scope, SmallInt::fromWord(0xAA));   // 0b10101010
  Int right(&scope, SmallInt::fromWord(0x9C));  // 0b10011100
  Object result(&scope, runtime_.intBinaryOr(thread_, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xBE));  // 0b10111110
}

TEST_F(RuntimeIntTest, BinaryOrWithLargeInts) {
  HandleScope scope(thread_);
  // {0b00001100, 0b00110000, 0b00000001}
  const uword digits_left[] = {0x0C, 0x30, 0x1};
  Int left(&scope, newIntWithDigits(&runtime_, digits_left));
  // {0b00000011, 0b11010000, 0b00000010, 0b00000111}
  const uword digits_right[] = {0x03, 0xD0, 0x2, 0x7};
  Int right(&scope, newIntWithDigits(&runtime_, digits_right));
  Object result(&scope, runtime_.intBinaryOr(thread_, left, right));
  // {0b00001111, 0b11110000, 0b00000011, 0b00000111}
  const uword expected_digits[] = {0x0F, 0xF0, 0x3, 0x7};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));

  Object result_commuted(&scope, runtime_.intBinaryOr(thread_, right, left));
  EXPECT_TRUE(isIntEqualsDigits(*result_commuted, expected_digits));
}

TEST_F(RuntimeIntTest, BinaryOrWithNegativeLargeInts) {
  HandleScope scope(thread_);

  Int left(&scope, SmallInt::fromWord(-42));  // 0b11010110
  const uword digits[] = {static_cast<uword>(-4), 0xF0, 0x2,
                          static_cast<uword>(-1)};
  Int right(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runtime_.intBinaryOr(thread_, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, -2));
}

TEST_F(RuntimeIntTest, BinaryXorWithSmallInts) {
  HandleScope scope(thread_);
  Int left(&scope, SmallInt::fromWord(0xAA));   // 0b10101010
  Int right(&scope, SmallInt::fromWord(0x9C));  // 0b10011100
  Object result(&scope, runtime_.intBinaryXor(thread_, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0x36));  // 0b00110110
}

TEST_F(RuntimeIntTest, BinaryXorWithLargeInts) {
  HandleScope scope(thread_);
  // {0b00001100, 0b00110000, 0b00000001}
  const uword digits_left[] = {0x0C, 0x30, 0x1};
  Int left(&scope, newIntWithDigits(&runtime_, digits_left));
  // {0b00000011, 0b11010000, 0b00000010, 0b00000111}
  const uword digits_right[] = {0x03, 0xD0, 0x2, 0x7};
  Int right(&scope, newIntWithDigits(&runtime_, digits_right));
  Object result(&scope, runtime_.intBinaryXor(thread_, left, right));
  // {0b00001111, 0b11100000, 0b00000011, 0b00000111}
  const uword expected_digits[] = {0x0F, 0xE0, 0x3, 0x7};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));

  Object result_commuted(&scope, runtime_.intBinaryXor(thread_, right, left));
  EXPECT_TRUE(isIntEqualsDigits(*result_commuted, expected_digits));
}

TEST_F(RuntimeIntTest, BinaryXorWithNegativeLargeInts) {
  HandleScope scope(thread_);

  Int left(&scope, SmallInt::fromWord(-42));  // 0b11010110
  const uword digits[] = {static_cast<uword>(-1), 0xf0, 0x2,
                          static_cast<uword>(-1)};
  Int right(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runtime_.intBinaryXor(thread_, left, right));
  const uword expected_digits[] = {0x29, ~uword{0xF0}, ~uword{0x2}, 0};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(RuntimeIntTest, NormalizeLargeIntToSmallInt) {
  HandleScope scope(thread_);

  const uword digits[] = {42};
  LargeInt lint_42(&scope, newLargeIntWithDigits(digits));
  Object norm_42(&scope, runtime_.normalizeLargeInt(thread_, lint_42));
  EXPECT_TRUE(isIntEqualsWord(*norm_42, 42));

  const uword digits2[] = {uword(-1)};
  LargeInt lint_neg1(&scope, newLargeIntWithDigits(digits2));
  Object norm_neg1(&scope, runtime_.normalizeLargeInt(thread_, lint_neg1));
  EXPECT_TRUE(isIntEqualsWord(*norm_neg1, -1));

  const uword digits3[] = {uword(RawSmallInt::kMinValue)};
  LargeInt lint_min(&scope, newLargeIntWithDigits(digits3));
  Object norm_min(&scope, runtime_.normalizeLargeInt(thread_, lint_min));
  EXPECT_TRUE(isIntEqualsWord(*norm_min, RawSmallInt::kMinValue));

  const uword digits4[] = {RawSmallInt::kMaxValue};
  LargeInt lint_max(&scope, newLargeIntWithDigits(digits4));
  Object norm_max(&scope, runtime_.normalizeLargeInt(thread_, lint_max));
  EXPECT_TRUE(isIntEqualsWord(*norm_max, RawSmallInt::kMaxValue));

  const uword digits5[] = {uword(-4), kMaxUword};
  LargeInt lint_sext_neg_4(&scope, newLargeIntWithDigits(digits5));
  Object norm_neg_4(&scope,
                    runtime_.normalizeLargeInt(thread_, lint_sext_neg_4));
  EXPECT_TRUE(isIntEqualsWord(*norm_neg_4, -4));

  const uword digits6[] = {uword(-13), kMaxUword, kMaxUword, kMaxUword};
  LargeInt lint_sext_neg_13(&scope, newLargeIntWithDigits(digits6));
  Object norm_neg_13(&scope,
                     runtime_.normalizeLargeInt(thread_, lint_sext_neg_13));
  EXPECT_TRUE(isIntEqualsWord(*norm_neg_13, -13));

  const uword digits7[] = {66, 0};
  LargeInt lint_zext_66(&scope, newLargeIntWithDigits(digits7));
  Object norm_66(&scope, runtime_.normalizeLargeInt(thread_, lint_zext_66));
  EXPECT_TRUE(isIntEqualsWord(*norm_66, 66));
}

TEST_F(RuntimeIntTest, NormalizeLargeIntToLargeInt) {
  HandleScope scope(thread_);

  const uword digits[] = {kMaxWord};
  LargeInt lint_max(&scope, newLargeIntWithDigits(digits));
  Object norm_max(&scope, runtime_.normalizeLargeInt(thread_, lint_max));
  EXPECT_TRUE(isIntEqualsWord(*norm_max, kMaxWord));

  const uword digits2[] = {uword(kMinWord)};
  LargeInt lint_min(&scope, newLargeIntWithDigits(digits2));
  Object norm_min(&scope, runtime_.normalizeLargeInt(thread_, lint_min));
  EXPECT_TRUE(isIntEqualsWord(*norm_min, kMinWord));

  const uword digits3[] = {kMaxWord - 7, 0, 0};
  LargeInt lint_max_sub_7_zext(&scope, newLargeIntWithDigits(digits3));
  Object norm_max_sub_7(
      &scope, runtime_.normalizeLargeInt(thread_, lint_max_sub_7_zext));
  EXPECT_TRUE(isIntEqualsWord(*norm_max_sub_7, kMaxWord - 7));

  const uword digits4[] = {uword(kMinWord) + 9, kMaxUword};
  LargeInt lint_min_plus_9_sext(&scope, newLargeIntWithDigits(digits4));
  Object norm_min_plus_9(
      &scope, runtime_.normalizeLargeInt(thread_, lint_min_plus_9_sext));
  EXPECT_TRUE(isIntEqualsWord(*norm_min_plus_9, kMinWord + 9));

  const uword digits5[] = {0, kMaxUword};
  LargeInt lint_no_sext(&scope, newLargeIntWithDigits(digits5));
  Object norm_no_sext(&scope,
                      runtime_.normalizeLargeInt(thread_, lint_no_sext));
  const uword expected_digits1[] = {0, kMaxUword};
  EXPECT_TRUE(isIntEqualsDigits(*norm_no_sext, expected_digits1));

  const uword digits6[] = {kMaxUword, 0};
  LargeInt lint_no_zext(&scope, newLargeIntWithDigits(digits6));
  Object norm_no_zext(&scope,
                      runtime_.normalizeLargeInt(thread_, lint_no_zext));
  const uword expected_digits2[] = {kMaxUword, 0};
  EXPECT_TRUE(isIntEqualsDigits(*norm_no_zext, expected_digits2));
}

TEST_F(RuntimeInstanceAttrTest, DeleteUnknownAttribute) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
    pass
)")
                   .isError());
  HandleScope scope(thread_);
  Type type(&scope, mainModuleAt(&runtime_, "Foo"));
  Layout layout(&scope, type.instanceLayout());
  HeapObject instance(&scope, runtime_.newInstance(layout));
  Str attr(&scope, runtime_.internStrFromCStr(thread_, "unknown"));
  EXPECT_TRUE(runtime_.instanceDel(thread_, instance, attr).isError());
}

TEST_F(RuntimeInstanceAttrTest, DeleteInObjectAttribute) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
    def __init__(self):
        self.bar = 'bar'
        self.baz = 'baz'

def new_foo():
    return Foo()
)")
                   .isError());

  // Create an instance of Foo
  HandleScope scope(thread_);
  Function new_foo(&scope, mainModuleAt(&runtime_, "new_foo"));
  Tuple args(&scope, runtime_.emptyTuple());
  HeapObject instance(&scope, callFunction(new_foo, args));

  // Verify that 'bar' is an in-object property
  Layout layout(&scope, runtime_.layoutAt(instance.header().layoutId()));
  Str attr(&scope, runtime_.internStrFromCStr(thread_, "bar"));
  AttributeInfo info;
  ASSERT_TRUE(runtime_.layoutFindAttribute(thread_, layout, attr, &info));
  ASSERT_TRUE(info.isInObject());

  // After successful deletion, the instance should have a new layout and should
  // no longer reference the previous value
  EXPECT_EQ(runtime_.instanceDel(thread_, instance, attr), NoneType::object());
  Layout new_layout(&scope, runtime_.layoutAt(instance.header().layoutId()));
  EXPECT_NE(*layout, *new_layout);
  EXPECT_FALSE(runtime_.layoutFindAttribute(thread_, new_layout, attr, &info));
}

TEST_F(RuntimeInstanceAttrTest, DeleteOverflowAttribute) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
    pass

def new_foo():
    foo = Foo()
    foo.bar = 'bar'
    return foo
)")
                   .isError());

  // Create an instance of Foo
  HandleScope scope(thread_);
  Function new_foo(&scope, mainModuleAt(&runtime_, "new_foo"));
  Tuple args(&scope, runtime_.emptyTuple());
  HeapObject instance(&scope, callFunction(new_foo, args));

  // Verify that 'bar' is an overflow property
  Layout layout(&scope, runtime_.layoutAt(instance.header().layoutId()));
  Str attr(&scope, runtime_.internStrFromCStr(thread_, "bar"));
  AttributeInfo info;
  ASSERT_TRUE(runtime_.layoutFindAttribute(thread_, layout, attr, &info));
  ASSERT_TRUE(info.isOverflow());

  // After successful deletion, the instance should have a new layout and should
  // no longer reference the previous value
  EXPECT_EQ(runtime_.instanceDel(thread_, instance, attr), NoneType::object());
  Layout new_layout(&scope, runtime_.layoutAt(instance.header().layoutId()));
  EXPECT_NE(*layout, *new_layout);
  EXPECT_FALSE(runtime_.layoutFindAttribute(thread_, new_layout, attr, &info));
}

TEST_F(RuntimeTest, InstanceDelWithReadonlyAttributeRaisesAttributeError) {
  HandleScope scope(thread_);

  BuiltinAttribute attrs[] = {
      {SymbolId::kDunderGlobals, 0, AttributeFlags::kReadOnly},
      {SymbolId::kSentinelId, -1},
  };
  BuiltinMethod builtins[] = {
      {SymbolId::kSentinelId, nullptr},
  };
  LayoutId layout_id = runtime_.reserveLayoutId(thread_);
  Type type(&scope,
            runtime_.addBuiltinType(SymbolId::kVersion, layout_id,
                                    LayoutId::kObject, attrs, builtins));
  Layout layout(&scope, type.instanceLayout());
  runtime_.layoutAtPut(layout_id, *layout);
  HeapObject instance(&scope, runtime_.newInstance(layout));
  Str attribute_name(&scope,
                     runtime_.internStrFromCStr(thread_, "__globals__"));
  EXPECT_TRUE(raisedWithStr(
      runtime_.instanceDel(thread_, instance, attribute_name),
      LayoutId::kAttributeError, "'__globals__' attribute is read-only"));
  EXPECT_EQ(instance.layoutId(), layout.id());
}

TEST_F(RuntimeTest, ClassWithTypeMetaclassIsConcreteType) {
  const char* src = R"(
# This is equivalent to `class Foo(type)`
class Foo(type, metaclass=type):
    pass

class Bar(Foo):
    pass
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());

  Object foo(&scope, mainModuleAt(&runtime_, "Foo"));
  EXPECT_TRUE(foo.isType());

  Object bar(&scope, mainModuleAt(&runtime_, "Bar"));
  EXPECT_TRUE(bar.isType());
}

TEST_F(RuntimeTest, ClassWithCustomMetaclassIsntConcreteType) {
  const char* src = R"(
class MyMeta(type):
    pass

class Foo(type, metaclass=MyMeta):
    pass
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());

  Object foo(&scope, mainModuleAt(&runtime_, "Foo"));
  EXPECT_FALSE(foo.isType());
}

TEST_F(RuntimeTest, ClassWithTypeMetaclassIsInstanceOfType) {
  const char* src = R"(
class Foo(type):
    pass

class Bar(Foo):
    pass
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());

  Object foo(&scope, mainModuleAt(&runtime_, "Foo"));
  EXPECT_TRUE(runtime_.isInstanceOfType(*foo));

  Object bar(&scope, mainModuleAt(&runtime_, "Bar"));
  EXPECT_TRUE(runtime_.isInstanceOfType(*bar));
}

TEST_F(RuntimeTest, ClassWithCustomMetaclassIsInstanceOfType) {
  const char* src = R"(
class MyMeta(type):
    pass

class Foo(type, metaclass=MyMeta):
    pass
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Object foo(&scope, mainModuleAt(&runtime_, "Foo"));
  EXPECT_TRUE(runtime_.isInstanceOfType(*foo));
}

TEST_F(RuntimeTest, VerifyMetaclassHierarchy) {
  const char* src = R"(
class GrandMeta(type):
    pass

class ParentMeta(type, metaclass=GrandMeta):
    pass

class ChildMeta(type, metaclass=ParentMeta):
    pass
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Object type(&scope, runtime_.typeAt(LayoutId::kType));

  Object grand_meta(&scope, mainModuleAt(&runtime_, "GrandMeta"));
  EXPECT_EQ(runtime_.typeOf(*grand_meta), *type);

  Object parent_meta(&scope, mainModuleAt(&runtime_, "ParentMeta"));
  EXPECT_EQ(runtime_.typeOf(*parent_meta), *grand_meta);

  Object child_meta(&scope, mainModuleAt(&runtime_, "ChildMeta"));
  EXPECT_EQ(runtime_.typeOf(*child_meta), *parent_meta);
}

TEST_F(RuntimeMetaclassTest, CallMetaclass) {
  const char* src = R"(
class MyMeta(type):
    pass

Foo = MyMeta('Foo', (), {})
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Object mymeta(&scope, mainModuleAt(&runtime_, "MyMeta"));
  Object foo(&scope, mainModuleAt(&runtime_, "Foo"));
  EXPECT_EQ(runtime_.typeOf(*foo), *mymeta);
  EXPECT_FALSE(foo.isType());
  EXPECT_TRUE(runtime_.isInstanceOfType(*foo));
}

TEST_F(RuntimeTest, SubclassBuiltinSubclass) {
  const char* src = R"(
class Test(Exception):
  pass
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Object value(&scope, mainModuleAt(&runtime_, "Test"));
  ASSERT_TRUE(value.isType());

  Type type(&scope, *value);
  ASSERT_TRUE(type.mro().isTuple());

  Tuple mro(&scope, type.mro());
  ASSERT_EQ(mro.length(), 4);
  EXPECT_EQ(mro.at(0), *type);
  EXPECT_EQ(mro.at(1), runtime_.typeAt(LayoutId::kException));
  EXPECT_EQ(mro.at(2), runtime_.typeAt(LayoutId::kBaseException));
  EXPECT_EQ(mro.at(3), runtime_.typeAt(LayoutId::kObject));
}

TEST_F(RuntimeModuleTest, ModuleImportsAllPublicSymbols) {
  HandleScope scope(thread_);

  // Create Module
  Object name(&scope, runtime_.newStrFromCStr("foo"));
  Module module(&scope, runtime_.newModule(name));

  // Add symbols
  Dict module_dict(&scope, module.dict());
  Str symbol_str1(&scope, runtime_.newStrFromCStr("public_symbol"));
  Str symbol_str2(&scope, runtime_.newStrFromCStr("_private_symbol"));
  runtime_.dictAtPutInValueCellByStr(thread_, module_dict, symbol_str1,
                                     symbol_str1);
  runtime_.dictAtPutInValueCellByStr(thread_, module_dict, symbol_str2,
                                     symbol_str2);

  // Import public symbols to dictionary
  Dict symbols_dict(&scope, runtime_.newDict());
  runtime_.moduleImportAllFrom(symbols_dict, module);
  EXPECT_EQ(symbols_dict.numItems(), 1);

  ValueCell result(&scope,
                   runtime_.dictAtByStr(thread_, symbols_dict, symbol_str1));
  EXPECT_TRUE(isStrEqualsCStr(result.value(), "public_symbol"));
}

TEST_F(RuntimeTest, HeapFrameCreate) {
  const char* src = R"(
def gen():
  yield 12
)";

  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Object gen_obj(&scope, mainModuleAt(&runtime_, "gen"));
  ASSERT_TRUE(gen_obj.isFunction());
  Function gen(&scope, *gen_obj);
  Object frame_obj(&scope, runtime_.newHeapFrame(gen));
  ASSERT_TRUE(frame_obj.isHeapFrame());
  HeapFrame heap_frame(&scope, *frame_obj);
  EXPECT_EQ(heap_frame.maxStackSize(), gen.stacksize());
}

extern "C" struct _inittab _PyImport_Inittab[];

TEST_F(RuntimeModuleTest, ImportModuleFromInitTab) {
  ASSERT_FALSE(runFromCStr(&runtime_, "import _empty").isError());
  HandleScope scope(thread_);
  Object mod(&scope, mainModuleAt(&runtime_, "_empty"));
  EXPECT_TRUE(mod.isModule());
}

TEST_F(RuntimeModuleTest, NewModuleSetsDictValuesAndModuleProxy) {
  HandleScope scope(thread_);

  // Create Module
  Object name(&scope, runtime_.newStrFromCStr("mymodule"));
  Module module(&scope, runtime_.newModule(name));
  runtime_.addModule(module);

  Str mod_name(&scope, moduleAtByCStr(&runtime_, "mymodule", "__name__"));
  EXPECT_TRUE(mod_name.equalsCStr("mymodule"));
  EXPECT_EQ(moduleAtByCStr(&runtime_, "mymodule", "__doc__"),
            NoneType::object());
  EXPECT_EQ(moduleAtByCStr(&runtime_, "mymodule", "__package__"),
            NoneType::object());
  EXPECT_EQ(moduleAtByCStr(&runtime_, "mymodule", "__loader__"),
            NoneType::object());
  EXPECT_EQ(moduleAtByCStr(&runtime_, "mymodule", "__spec__"),
            NoneType::object());

  ModuleProxy module_proxy(&scope, module.moduleProxy());
  EXPECT_EQ(module_proxy.module(), *module);
}

TEST_F(RuntimeFunctionAttrTest, SetAttribute) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(): pass
foo.x = 3
)")
                   .isError());
  HandleScope scope(thread_);
  Function function(&scope, mainModuleAt(&runtime_, "foo"));
  Dict function_dict(&scope, function.dict());
  Str name(&scope, runtime_.newStrFromCStr("x"));
  Object value(&scope, runtime_.dictAtByStr(thread_, function_dict, name));
  EXPECT_TRUE(isIntEqualsWord(*value, 3));
}

TEST_F(RuntimeTest, LazyInitializationOfFunctionDictWithAttribute) {
  HandleScope scope(thread_);
  Function function(&scope, makeTestFunction());
  ASSERT_TRUE(function.dict().isNoneType());

  Object key(&scope, runtime_.newStrFromCStr("bar"));
  runtime_.attributeAt(thread_, function, key);
  EXPECT_TRUE(function.dict().isDict());
}

TEST_F(RuntimeTest, LazyInitializationOfFunctionDict) {
  HandleScope scope(thread_);
  Function function(&scope, makeTestFunction());
  ASSERT_TRUE(function.dict().isNoneType());

  Object key(&scope, runtime_.newStrFromCStr("__dict__"));
  runtime_.attributeAt(thread_, function, key);
  EXPECT_TRUE(function.dict().isDict());
}

TEST_F(RuntimeTest, NotMatchingCellAndVarNamesSetsCell2ArgToNone) {
  HandleScope scope(thread_);
  word argcount = 3;
  word kwargcount = 0;
  word nlocals = 3;
  Tuple varnames(&scope, runtime_.newTuple(argcount + kwargcount));
  Tuple cellvars(&scope, runtime_.newTuple(2));
  Str foo(&scope, runtime_.internStrFromCStr(thread_, "foo"));
  Str bar(&scope, runtime_.internStrFromCStr(thread_, "bar"));
  Str baz(&scope, runtime_.internStrFromCStr(thread_, "baz"));
  Str foobar(&scope, runtime_.internStrFromCStr(thread_, "foobar"));
  Str foobaz(&scope, runtime_.internStrFromCStr(thread_, "foobaz"));
  varnames.atPut(0, *foo);
  varnames.atPut(1, *bar);
  varnames.atPut(2, *baz);
  cellvars.atPut(0, *foobar);
  cellvars.atPut(1, *foobaz);
  Object code_code(&scope, Bytes::empty());
  Tuple empty_tuple(&scope, runtime_.emptyTuple());
  Object empty_bytes(&scope, Bytes::empty());
  Object empty_str(&scope, Str::empty());
  Code code(&scope,
            runtime_.newCode(argcount, /*posonlyargcount=*/0, kwargcount,
                             nlocals, /*stacksize=*/0, /*flags=*/0, code_code,
                             empty_tuple, empty_tuple, varnames, empty_tuple,
                             cellvars, empty_str, empty_str, 0, empty_bytes));
  EXPECT_TRUE(code.cell2arg().isNoneType());
}

TEST_F(RuntimeTest, MatchingCellAndVarNamesCreatesCell2Arg) {
  HandleScope scope(thread_);
  word argcount = 3;
  word kwargcount = 0;
  word nlocals = 3;
  Tuple varnames(&scope, runtime_.newTuple(argcount + kwargcount));
  Tuple cellvars(&scope, runtime_.newTuple(2));
  Str foo(&scope, runtime_.internStrFromCStr(thread_, "foo"));
  Str bar(&scope, runtime_.internStrFromCStr(thread_, "bar"));
  Str baz(&scope, runtime_.internStrFromCStr(thread_, "baz"));
  Str foobar(&scope, runtime_.internStrFromCStr(thread_, "foobar"));
  varnames.atPut(0, *foo);
  varnames.atPut(1, *bar);
  varnames.atPut(2, *baz);
  cellvars.atPut(0, *baz);
  cellvars.atPut(1, *foobar);
  Object code_code(&scope, Bytes::empty());
  Tuple empty_tuple(&scope, runtime_.emptyTuple());
  Object empty_bytes(&scope, Bytes::empty());
  Object empty_str(&scope, Str::empty());
  Code code(&scope,
            runtime_.newCode(argcount, /*posonlyargcount=*/0, kwargcount,
                             nlocals, /*stacksize=*/0, /*flags=*/0, code_code,
                             empty_tuple, empty_tuple, varnames, empty_tuple,
                             cellvars, empty_str, empty_str, 0, empty_bytes));
  ASSERT_FALSE(code.cell2arg().isNoneType());
  Tuple cell2arg(&scope, code.cell2arg());
  ASSERT_EQ(cell2arg.length(), 2);

  Object cell2arg_value(&scope, cell2arg.at(0));
  EXPECT_TRUE(isIntEqualsWord(*cell2arg_value, 2));
  EXPECT_EQ(cell2arg.at(1), NoneType::object());
}

TEST_F(RuntimeTest, NewCodeWithCellvarsTurnsOffNofreeFlag) {
  HandleScope scope(thread_);
  word argcount = 3;
  word nlocals = 3;
  Tuple varnames(&scope, runtime_.newTuple(argcount));
  Tuple cellvars(&scope, runtime_.newTuple(2));
  Str foo(&scope, runtime_.internStrFromCStr(thread_, "foo"));
  Str bar(&scope, runtime_.internStrFromCStr(thread_, "bar"));
  Str baz(&scope, runtime_.internStrFromCStr(thread_, "baz"));
  Str foobar(&scope, runtime_.internStrFromCStr(thread_, "foobar"));
  varnames.atPut(0, *foo);
  varnames.atPut(1, *bar);
  varnames.atPut(2, *baz);
  cellvars.atPut(0, *baz);
  cellvars.atPut(1, *foobar);
  Object code_code(&scope, Bytes::empty());
  Tuple empty_tuple(&scope, runtime_.emptyTuple());
  Object empty_bytes(&scope, Bytes::empty());
  Object empty_str(&scope, Str::empty());
  Code code(&scope, runtime_.newCode(
                        argcount, /*posonlyargcount=*/0, /*kwonlyargcount=*/0,
                        nlocals, /*stacksize=*/0, /*flags=*/0, code_code,
                        empty_tuple, empty_tuple, varnames, empty_tuple,
                        cellvars, empty_str, empty_str, 0, empty_bytes));
  EXPECT_FALSE(code.flags() & Code::Flags::NOFREE);
}

TEST_F(RuntimeTest, NewCodeWithNoFreevarsOrCellvarsSetsNofreeFlag) {
  HandleScope scope(thread_);
  Tuple varnames(&scope, runtime_.newTuple(1));
  varnames.atPut(0, runtime_.newStrFromCStr("foobar"));
  Object code_code(&scope, Bytes::empty());
  Tuple empty_tuple(&scope, runtime_.emptyTuple());
  Object empty_bytes(&scope, Bytes::empty());
  Object empty_str(&scope, Str::empty());
  Object code_obj(
      &scope, runtime_.newCode(
                  /*argcount=*/0, /*posonlyargcount=*/0, /*kwonlyargcount=*/0,
                  /*nlocals=*/0, /*stacksize=*/0, /*flags=*/0, code_code,
                  empty_tuple, empty_tuple, varnames, empty_tuple, empty_tuple,
                  empty_str, empty_str, 0, empty_bytes));
  ASSERT_TRUE(code_obj.isCode());
  Code code(&scope, *code_obj);
  EXPECT_TRUE(code.flags() & Code::Flags::NOFREE);
}

TEST_F(RuntimeTest,
       NewCodeWithArgcountGreaterThanVarnamesLengthRaisesValueError) {
  HandleScope scope(thread_);
  Tuple varnames(&scope, runtime_.newTuple(1));
  Tuple cellvars(&scope, runtime_.newTuple(2));
  Object code_code(&scope, Bytes::empty());
  Tuple empty_tuple(&scope, runtime_.emptyTuple());
  Object empty_bytes(&scope, Bytes::empty());
  Object empty_str(&scope, Str::empty());
  EXPECT_TRUE(raisedWithStr(
      runtime_.newCode(/*argcount=*/10, /*posonlyargcount=*/0,
                       /*kwonlyargcount=*/0, /*nlocals=*/0, /*stacksize=*/0,
                       /*flags=*/0, code_code, empty_tuple, empty_tuple,
                       varnames, empty_tuple, cellvars, empty_str, empty_str, 0,
                       empty_bytes),
      LayoutId::kValueError, "code: varnames is too small"));
}

TEST_F(RuntimeTest,
       NewCodeWithKwonlyargcountGreaterThanVarnamesLengthRaisesValueError) {
  HandleScope scope(thread_);
  Tuple varnames(&scope, runtime_.newTuple(1));
  Tuple cellvars(&scope, runtime_.newTuple(2));
  Object code_code(&scope, Bytes::empty());
  Tuple empty_tuple(&scope, runtime_.emptyTuple());
  Object empty_bytes(&scope, Bytes::empty());
  Object empty_str(&scope, Str::empty());
  EXPECT_TRUE(raisedWithStr(
      runtime_.newCode(/*argcount=*/0, /*posonlyargcount=*/0,
                       /*kwonlyargcount=*/10, /*nlocals=*/0, /*stacksize=*/0,
                       /*flags=*/0, code_code, empty_tuple, empty_tuple,
                       varnames, empty_tuple, cellvars, empty_str, empty_str, 0,
                       empty_bytes),
      LayoutId::kValueError, "code: varnames is too small"));
}

TEST_F(RuntimeTest,
       NewCodeWithTotalArgsGreaterThanVarnamesLengthRaisesValueError) {
  HandleScope scope(thread_);
  Tuple varnames(&scope, runtime_.newTuple(1));
  Tuple cellvars(&scope, runtime_.newTuple(2));
  Object code_code(&scope, Bytes::empty());
  Tuple empty_tuple(&scope, runtime_.emptyTuple());
  Object empty_bytes(&scope, Bytes::empty());
  Object empty_str(&scope, Str::empty());
  EXPECT_TRUE(raisedWithStr(
      runtime_.newCode(/*argcount=*/1, /*posonlyargcount=*/0,
                       /*kwonlyargcount=*/1, /*nlocals=*/0, /*stacksize=*/0,
                       /*flags=*/0, code_code, empty_tuple, empty_tuple,
                       varnames, empty_tuple, cellvars, empty_str, empty_str, 0,
                       empty_bytes),
      LayoutId::kValueError, "code: varnames is too small"));
}

TEST_F(RuntimeTest, NewWeakLink) {
  HandleScope scope(thread_);

  Tuple referent(&scope, runtime_.newTuple(2));
  Object prev(&scope, runtime_.newInt(2));
  Object next(&scope, runtime_.newInt(3));
  WeakLink link(&scope, runtime_.newWeakLink(thread_, referent, prev, next));
  EXPECT_EQ(link.referent(), *referent);
  EXPECT_EQ(link.prev(), *prev);
  EXPECT_EQ(link.next(), *next);
}

// Set is not special except that it is a builtin type with sealed attributes.
TEST_F(RuntimeTest, SetHasSameSizeCreatedTwoDifferentWays) {
  HandleScope scope(thread_);
  Layout layout(&scope, runtime_.layoutAt(LayoutId::kSet));
  Set set1(&scope, runtime_.newInstance(layout));
  Set set2(&scope, runtime_.newSet());
  EXPECT_EQ(set1.size(), set2.size());
}

// Set is not special except that it is a builtin type with sealed attributes.
TEST_F(RuntimeTest, SealedClassLayoutDoesNotHaveSpaceForOverflowAttributes) {
  HandleScope scope(thread_);
  Layout layout(&scope, runtime_.layoutAt(LayoutId::kSet));
  EXPECT_TRUE(layout.isSealed());
  word expected_set_size = kPointerSize * layout.numInObjectAttributes();
  EXPECT_EQ(layout.instanceSize(), expected_set_size);
}

TEST_F(RuntimeTest, SettingNewAttributeOnSealedClassRaisesAttributeError) {
  HandleScope scope(thread_);
  Set set(&scope, runtime_.newSet());
  Str attr(&scope, runtime_.newStrFromCStr("attr"));
  Str value(&scope, runtime_.newStrFromCStr("value"));
  Object result(&scope, instanceSetAttr(thread_, set, attr, value));
  EXPECT_TRUE(raised(*result, LayoutId::kAttributeError));
}

TEST_F(RuntimeTest, InstanceAtPutWithReadOnlyAttributeRaisesAttributeError) {
  HandleScope scope(thread_);

  BuiltinAttribute attrs[] = {
      {SymbolId::kDunderGlobals, 0, AttributeFlags::kReadOnly},
      {SymbolId::kSentinelId, -1},
  };
  BuiltinMethod builtins[] = {
      {SymbolId::kSentinelId, nullptr},
  };
  LayoutId layout_id = runtime_.reserveLayoutId(thread_);
  Type type(&scope,
            runtime_.addBuiltinType(SymbolId::kVersion, layout_id,
                                    LayoutId::kObject, attrs, builtins));
  Layout layout(&scope, type.instanceLayout());
  runtime_.layoutAtPut(layout_id, *layout);
  HeapObject instance(&scope, runtime_.newInstance(layout));
  Str attribute_name(&scope,
                     runtime_.internStrFromCStr(thread_, "__globals__"));
  Object value(&scope, NoneType::object());
  EXPECT_TRUE(
      raisedWithStr(instanceSetAttr(thread_, instance, attribute_name, value),
                    LayoutId::kAttributeError,
                    "'version.__globals__' attribute is read-only"));
}

// Exception attributes can be set on the fly.
TEST_F(RuntimeTest, NonSealedClassHasSpaceForOverflowAttrbutes) {
  HandleScope scope(thread_);
  Layout layout(&scope, runtime_.layoutAt(LayoutId::kMemoryError));
  EXPECT_TRUE(layout.hasTupleOverflow());
  EXPECT_EQ(layout.instanceSize(),
            (layout.numInObjectAttributes() + 1) * kPointerSize);  // 1=overflow
}

// User-defined class attributes can be set on the fly.
TEST_F(RuntimeTest, UserCanSetOverflowAttributeOnUserDefinedClass) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C(): pass
a = C()
)")
                   .isError());
  HeapObject a(&scope, mainModuleAt(&runtime_, "a"));
  Str attr(&scope, runtime_.newStrFromCStr("attr"));
  Str value(&scope, runtime_.newStrFromCStr("value"));
  Object result(&scope, instanceSetAttr(thread_, a, attr, value));
  ASSERT_FALSE(result.isError());
  EXPECT_EQ(instanceGetAttribute(thread_, a, attr), *value);
}

TEST_F(RuntimeTest, IsMappingReturnsFalseOnSet) {
  HandleScope scope(thread_);
  Set set(&scope, runtime_.newSet());
  EXPECT_FALSE(runtime_.isMapping(thread_, set));
}

TEST_F(RuntimeTest, IsMappingReturnsTrueOnDict) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  EXPECT_TRUE(runtime_.isMapping(thread_, dict));
}

TEST_F(RuntimeTest, IsMappingReturnsTrueOnList) {
  HandleScope scope(thread_);
  List list(&scope, runtime_.newList());
  EXPECT_TRUE(runtime_.isMapping(thread_, list));
}

TEST_F(RuntimeTest, IsMappingReturnsTrueOnCustomClassWithMethod) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C():
  def __getitem__(self, key):
    pass
o = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Object obj(&scope, mainModuleAt(&runtime_, "o"));
  EXPECT_TRUE(runtime_.isMapping(thread_, obj));
}

TEST_F(RuntimeTest, IsMappingWithClassAttrNotCallableReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C():
  __getitem__ = 4
o = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Object obj(&scope, mainModuleAt(&runtime_, "o"));
  EXPECT_TRUE(runtime_.isMapping(thread_, obj));
}

TEST_F(RuntimeTest, IsMappingReturnsFalseOnCustomClassWithoutMethod) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C():
  pass
o = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Object obj(&scope, mainModuleAt(&runtime_, "o"));
  EXPECT_FALSE(runtime_.isMapping(thread_, obj));
}

TEST_F(RuntimeTest, IsMappingWithInstanceAttrReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C():
  pass
o = C()
o.__getitem__ = 4
)")
                   .isError());
  HandleScope scope(thread_);
  Object obj(&scope, mainModuleAt(&runtime_, "o"));
  EXPECT_FALSE(runtime_.isMapping(thread_, obj));
}

TEST_F(RuntimeTest, ModuleBuiltinsExists) {
  ASSERT_FALSE(moduleAtByCStr(&runtime_, "builtins", "__name__").isError());
}

TEST_F(RuntimeTest, ObjectEqualsWithSameObjectReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C():
  def __eq__(self, other):
    return False
i = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Object i(&scope, mainModuleAt(&runtime_, "i"));
  EXPECT_EQ(Runtime::objectEquals(thread_, *i, *i), Bool::trueObj());
}

TEST_F(RuntimeTest, ObjectEqualsCallsDunderEq) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __eq__(self, other):
    return True
i = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Object i(&scope, mainModuleAt(&runtime_, "i"));
  EXPECT_EQ(Runtime::objectEquals(thread_, *i, NoneType::object()),
            Bool::trueObj());
  EXPECT_EQ(Runtime::objectEquals(thread_, *i, SmallStr::fromCStr("foo")),
            Bool::trueObj());
  EXPECT_EQ(Runtime::objectEquals(thread_, *i, Bool::falseObj()),
            Bool::trueObj());
}

TEST_F(RuntimeTest, ObjectEqualsCallsStrSubclassDunderEq) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class StrSub(str):
  def __eq__(self, other):
    return True
i = StrSub("foo")
)")
                   .isError());

  HandleScope scope(thread_);
  Object i(&scope, mainModuleAt(&runtime_, "i"));
  EXPECT_EQ(Runtime::objectEquals(thread_, Str::empty(), *i), Bool::trueObj());
  EXPECT_EQ(Runtime::objectEquals(thread_, *i, Str::empty()), Bool::trueObj());
  LargeStr large_str(&scope, runtime_.newStrFromCStr("foobarbazbumbam"));
  EXPECT_EQ(Runtime::objectEquals(thread_, *large_str, *i), Bool::trueObj());
  EXPECT_EQ(Runtime::objectEquals(thread_, *i, *large_str), Bool::trueObj());
  EXPECT_EQ(Runtime::objectEquals(thread_, SmallInt::fromWord(0), *i),
            Bool::trueObj());
  EXPECT_EQ(Runtime::objectEquals(thread_, *i, SmallInt::fromWord(0)),
            Bool::trueObj());
}

TEST_F(RuntimeTest, ObjectEqualsCallsIntSubclassDunderEq) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class IntSub(int):
  def __eq__(self, other):
    return True
i = IntSub(7)
)")
                   .isError());
  HandleScope scope(thread_);
  Object i(&scope, mainModuleAt(&runtime_, "i"));
  EXPECT_EQ(Runtime::objectEquals(thread_, SmallInt::fromWord(1), *i),
            Bool::trueObj());
  EXPECT_EQ(Runtime::objectEquals(thread_, *i, SmallInt::fromWord(1)),
            Bool::trueObj());
  const uword digits[] = {1, 2};
  LargeInt large_int(&scope, runtime_.newIntWithDigits(digits));
  EXPECT_EQ(Runtime::objectEquals(thread_, *i, *large_int), Bool::trueObj());
  EXPECT_EQ(Runtime::objectEquals(thread_, *large_int, *i), Bool::trueObj());
  EXPECT_EQ(Runtime::objectEquals(thread_, *i, Bool::trueObj()),
            Bool::trueObj());
  EXPECT_EQ(Runtime::objectEquals(thread_, NoneType::object(), *i),
            Bool::trueObj());
  EXPECT_EQ(Runtime::objectEquals(thread_, *i, NoneType::object()),
            Bool::trueObj());
  EXPECT_EQ(Runtime::objectEquals(thread_, Bool::trueObj(), *i),
            Bool::falseObj());
}

TEST_F(RuntimeTest, ObjectEqualsWithSmallStrReturnsBool) {
  RawSmallStr s0 = SmallStr::empty();
  RawSmallStr s1 = SmallStr::fromCStr("foo");
  EXPECT_EQ(Runtime::objectEquals(thread_, s0, s0), Bool::trueObj());
  EXPECT_EQ(Runtime::objectEquals(thread_, s0, s1), Bool::falseObj());
  EXPECT_EQ(Runtime::objectEquals(thread_, s1, s0), Bool::falseObj());
  EXPECT_EQ(Runtime::objectEquals(thread_, s1, s1), Bool::trueObj());
  EXPECT_EQ(Runtime::objectEquals(thread_, NoneType::object(), s0),
            Bool::falseObj());
  EXPECT_EQ(Runtime::objectEquals(thread_, s0, NoneType::object()),
            Bool::falseObj());
}

TEST_F(RuntimeTest, ObjectEqualsWithLargeStrReturnsBool) {
  HandleScope scope(thread_);
  LargeStr large_str0(&scope, runtime_.newStrFromCStr("foobarbazbumbam"));
  LargeStr large_str1(&scope, runtime_.newStrFromCStr("foobarbazbumbam"));
  ASSERT_NE(large_str0, large_str1);
  EXPECT_EQ(Runtime::objectEquals(thread_, *large_str0, *large_str1),
            Bool::trueObj());
  EXPECT_EQ(Runtime::objectEquals(thread_, *large_str0,
                                  runtime_.newStrFromCStr("hello world!")),
            Bool::falseObj());
}

TEST_F(RuntimeTest, ObjectEqualsWithImmediatesReturnsBool) {
  EXPECT_EQ(
      Runtime::objectEquals(thread_, NoneType::object(), NoneType::object()),
      Bool::trueObj());
  EXPECT_EQ(Runtime::objectEquals(thread_, SmallInt::fromWord(-88),
                                  SmallInt::fromWord(-88)),
            Bool::trueObj());
  EXPECT_EQ(Runtime::objectEquals(thread_, NoneType::object(),
                                  NotImplementedType::object()),
            Bool::falseObj());
  EXPECT_EQ(Runtime::objectEquals(thread_, SmallInt::fromWord(11),
                                  SmallInt::fromWord(-11)),
            Bool::falseObj());
}

TEST_F(RuntimeTest, ObjectEqualsWithIntAndBoolReturnsBool) {
  EXPECT_EQ(
      Runtime::objectEquals(thread_, SmallInt::fromWord(0), Bool::falseObj()),
      Bool::trueObj());
  EXPECT_EQ(
      Runtime::objectEquals(thread_, SmallInt::fromWord(1), Bool::trueObj()),
      Bool::trueObj());
  EXPECT_EQ(
      Runtime::objectEquals(thread_, Bool::falseObj(), SmallInt::fromWord(0)),
      Bool::trueObj());
  EXPECT_EQ(
      Runtime::objectEquals(thread_, Bool::trueObj(), SmallInt::fromWord(1)),
      Bool::trueObj());
  EXPECT_EQ(
      Runtime::objectEquals(thread_, Bool::falseObj(), SmallInt::fromWord(1)),
      Bool::falseObj());
  EXPECT_EQ(
      Runtime::objectEquals(thread_, SmallInt::fromWord(0), Bool::trueObj()),
      Bool::falseObj());
}

TEST_F(RuntimeStrTest, StrJoinWithNonStrRaisesTypeError) {
  HandleScope scope(thread_);
  Str sep(&scope, runtime_.newStrFromCStr(","));
  Tuple elts(&scope, runtime_.newTuple(3));
  elts.atPut(0, runtime_.newStrFromCStr("foo"));
  elts.atPut(1, runtime_.newInt(4));
  elts.atPut(2, runtime_.newStrFromCStr("bar"));
  EXPECT_TRUE(raisedWithStr(
      runtime_.strJoin(thread_, sep, elts, elts.length()), LayoutId::kTypeError,
      "sequence item 1: expected str instance, int found"));
}

TEST_F(RuntimeStrTest, StrJoinWithStrSubclassReturnsJoinedString) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C(str):
  pass
elts = (C("a"), C("b"), C("c"))
)")
                   .isError());
  HandleScope scope(thread_);
  Str sep(&scope, runtime_.newStrFromCStr(","));
  Tuple elts(&scope, mainModuleAt(&runtime_, "elts"));
  Object result(&scope, runtime_.strJoin(thread_, sep, elts, elts.length()));
  EXPECT_TRUE(isStrEqualsCStr(*result, "a,b,c"));
}

TEST_F(RuntimeStrTest, StrReplaceWithSmallStrResult) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("1212"));
  Str old(&scope, runtime_.newStrFromCStr("2"));
  Str newstr(&scope, runtime_.newStrFromCStr("*"));
  Object result(&scope, runtime_.strReplace(thread_, str, old, newstr, -1));
  EXPECT_TRUE(isStrEqualsCStr(*result, "1*1*"));
}

TEST_F(RuntimeStrTest, StrReplaceWithSmallStrAndNegativeReplacesAll) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("122"));
  Str old(&scope, runtime_.newStrFromCStr("2"));
  Str newstr(&scope, runtime_.newStrFromCStr("*"));
  Object result(&scope, runtime_.strReplace(thread_, str, old, newstr, -1));
  EXPECT_TRUE(isStrEqualsCStr(*result, "1**"));
}

TEST_F(RuntimeStrTest, StrReplaceWithLargeStrAndNegativeReplacesAll) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("111111121111111111211"));
  Str old(&scope, runtime_.newStrFromCStr("2"));
  Str newstr(&scope, runtime_.newStrFromCStr("*"));
  Object result(&scope, runtime_.strReplace(thread_, str, old, newstr, -1));
  EXPECT_TRUE(isStrEqualsCStr(*result, "1111111*1111111111*11"));
}

TEST_F(RuntimeStrTest, StrReplaceWithLargeStrAndCountReplacesSome) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("11112111111111111211"));
  Str old(&scope, runtime_.newStrFromCStr("2"));
  Str newstr(&scope, runtime_.newStrFromCStr("*"));
  Object result(&scope, runtime_.strReplace(thread_, str, old, newstr, 1));
  EXPECT_TRUE(isStrEqualsCStr(*result, "1111*111111111111211"));
}

TEST_F(RuntimeStrTest, StrReplaceWithSameLengthReplacesSubstr) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("12"));
  Str old(&scope, runtime_.newStrFromCStr("2"));
  Str newstr(&scope, runtime_.newStrFromCStr("*"));
  Object result(&scope, runtime_.strReplace(thread_, str, old, newstr, -1));
  EXPECT_TRUE(isStrEqualsCStr(*result, "1*"));
}

TEST_F(RuntimeStrTest, StrReplaceWithLongerNewReturnsLonger) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("12"));
  Str old(&scope, runtime_.newStrFromCStr("2"));
  Str newstr(&scope, runtime_.newStrFromCStr("**"));
  Object result(&scope, runtime_.strReplace(thread_, str, old, newstr, -1));
  EXPECT_TRUE(isStrEqualsCStr(*result, "1**"));
}

TEST_F(RuntimeStrTest, StrReplaceWithShorterNewReturnsShorter) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("12"));
  Str old(&scope, runtime_.newStrFromCStr("12"));
  Str newstr(&scope, runtime_.newStrFromCStr("*"));
  Object result(&scope, runtime_.strReplace(thread_, str, old, newstr, -1));
  EXPECT_TRUE(isStrEqualsCStr(*result, "*"));
}

TEST_F(RuntimeStrTest, StrReplaceWithPrefixReplacesBeginning) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("12"));
  Str old(&scope, runtime_.newStrFromCStr("1"));
  Str newstr(&scope, runtime_.newStrFromCStr("*"));
  Object result(&scope, runtime_.strReplace(thread_, str, old, newstr, -1));
  EXPECT_TRUE(isStrEqualsCStr(*result, "*2"));
}

TEST_F(RuntimeStrTest, StrReplaceWithInfixReplacesMiddle) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("121"));
  Str old(&scope, runtime_.newStrFromCStr("2"));
  Str newstr(&scope, runtime_.newStrFromCStr("*"));
  Object result(&scope, runtime_.strReplace(thread_, str, old, newstr, -1));
  EXPECT_TRUE(isStrEqualsCStr(*result, "1*1"));
}

TEST_F(RuntimeStrTest, StrReplaceWithPostfixReplacesEnd) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("112"));
  Str old(&scope, runtime_.newStrFromCStr("2"));
  Str newstr(&scope, runtime_.newStrFromCStr("*"));
  Object result(&scope, runtime_.strReplace(thread_, str, old, newstr, -1));
  EXPECT_TRUE(isStrEqualsCStr(*result, "11*"));
}

TEST_F(RuntimeStrTest, StrSliceASCII) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("hello world goodbye world"));
  Object slice(&scope, runtime_.strSlice(thread_, str, 2, 10, 2));
  EXPECT_TRUE(isStrEqualsCStr(*slice, "lowr"));
}

TEST_F(RuntimeStrTest, StrSliceUnicode) {
  HandleScope scope(thread_);
  Str str(&scope,
          runtime_.newStrFromCStr(
              u8"\u05d0\u05e0\u05d9 \u05dc\u05d0 \u05d0\u05d5\u05d4\u05d1 "
              u8"\u05e0\u05d7\u05e9\u05d9\u05dd"));
  Str slice(&scope, runtime_.strSlice(thread_, str, 2, 10, 2));
  EXPECT_TRUE(isStrEqualsCStr(*slice, u8"\u05d9\u05dc \u05d5"));
}

TEST_F(RuntimeTest, BuiltinBaseOfNonEmptyTypeIsTypeItself) {
  HandleScope scope(thread_);

  BuiltinAttribute attrs[] = {
      {SymbolId::kDunderGlobals, 0, AttributeFlags::kReadOnly},
      {SymbolId::kSentinelId, -1},
  };
  BuiltinMethod builtins[] = {
      {SymbolId::kSentinelId, nullptr},
  };
  LayoutId layout_id = runtime_.reserveLayoutId(thread_);
  Type type(&scope,
            runtime_.addBuiltinType(SymbolId::kVersion, layout_id,
                                    LayoutId::kObject, attrs, builtins));
  EXPECT_EQ(type.builtinBase(), layout_id);
}

TEST_F(RuntimeTest, BuiltinBaseOfEmptyTypeIsSuperclass) {
  HandleScope scope(thread_);

  BuiltinAttribute attrs[] = {
      {SymbolId::kSentinelId, -1},
  };
  BuiltinMethod builtins[] = {
      {SymbolId::kSentinelId, nullptr},
  };
  LayoutId layout_id = runtime_.reserveLayoutId(thread_);
  Type type(&scope,
            runtime_.addBuiltinType(SymbolId::kVersion, layout_id,
                                    LayoutId::kObject, attrs, builtins));
  EXPECT_EQ(type.builtinBase(), LayoutId::kObject);
}

TEST_F(RuntimeTest, NonModuleInModulesDoesNotCrash) {
  HandleScope scope(thread_);
  Object not_a_module(&scope, runtime_.newInt(42));
  Str name(&scope, runtime_.newStrFromCStr("a_valid_module_name"));
  Dict modules(&scope, runtime_.modules());
  runtime_.dictAtPutByStr(thread_, modules, name, not_a_module);

  Object result(&scope, runtime_.findModule(name));
  EXPECT_EQ(result, not_a_module);
}

TEST_F(RuntimeStrArrayTest, NewStrArrayReturnsEmptyStrArray) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_.newStrArray());
  ASSERT_TRUE(obj.isStrArray());
  StrArray str_arr(&scope, *obj);
  EXPECT_EQ(str_arr.numItems(), 0);
  EXPECT_EQ(str_arr.capacity(), 0);
}

TEST_F(RuntimeStrArrayTest, EnsureCapacitySetsProperCapacity) {
  HandleScope scope(thread_);

  StrArray array(&scope, runtime_.newStrArray());
  word length = 1;
  word expected_capacity = 16;
  runtime_.strArrayEnsureCapacity(thread_, array, length);
  EXPECT_EQ(array.capacity(), expected_capacity);

  length = 17;
  expected_capacity = 24;
  runtime_.strArrayEnsureCapacity(thread_, array, length);
  EXPECT_EQ(array.capacity(), expected_capacity);

  length = 40;
  expected_capacity = 40;
  runtime_.strArrayEnsureCapacity(thread_, array, length);
  EXPECT_EQ(array.capacity(), expected_capacity);
}

TEST_F(RuntimeStrArrayTest, NewStrFromEmptyStrArrayReturnsEmptyStr) {
  HandleScope scope(thread_);

  StrArray array(&scope, runtime_.newStrArray());
  EXPECT_EQ(runtime_.strFromStrArray(array), Str::empty());
}

TEST_F(RuntimeStrArrayTest, AppendStrAppendsValidUTF8) {
  HandleScope scope(thread_);

  StrArray array(&scope, runtime_.newStrArray());
  Str one(&scope, runtime_.newStrFromCStr("a\xC3\xA9"));
  Str two(&scope, runtime_.newStrFromCStr("\xE2\xB3\x80\xF0\x9F\x86\x92"));
  runtime_.strArrayAddStr(thread_, array, one);
  runtime_.strArrayAddStr(thread_, array, two);
  EXPECT_EQ(array.numItems(), 10);

  EXPECT_TRUE(isStrEqualsCStr(runtime_.strFromStrArray(array),
                              "a\xC3\xA9\xE2\xB3\x80\xF0\x9F\x86\x92"));
}

TEST_F(RuntimeStrArrayTest, AddASCIIAppendsASCII) {
  HandleScope scope(thread_);

  StrArray array(&scope, runtime_.newStrArray());
  runtime_.strArrayAddASCII(thread_, array, 'h');
  runtime_.strArrayAddASCII(thread_, array, 'i');
  EXPECT_EQ(array.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(runtime_.strFromStrArray(array), "hi"));
}

}  // namespace python
