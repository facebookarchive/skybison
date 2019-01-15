#include "gtest/gtest.h"

#include <memory>

#include "bytecode.h"
#include "frame.h"
#include "layout.h"
#include "runtime.h"
#include "symbols.h"
#include "test-utils.h"

namespace python {
using namespace testing;

TEST(RuntimeTest, CollectGarbage) {
  Runtime runtime;
  ASSERT_TRUE(runtime.heap()->verify());
  runtime.collectGarbage();
  ASSERT_TRUE(runtime.heap()->verify());
}

TEST(RuntimeTest, AllocateAndCollectGarbage) {
  const word heap_size = 32 * kMiB;
  const word array_length = 1024;
  const word allocation_size = Bytes::allocationSize(array_length);
  const word total_allocation_size = heap_size * 10;
  Runtime runtime(heap_size);
  ASSERT_TRUE(runtime.heap()->verify());
  for (word i = 0; i < total_allocation_size; i += allocation_size) {
    runtime.newBytes(array_length, 0);
  }
  ASSERT_TRUE(runtime.heap()->verify());
}

TEST(RuntimeTest, BuiltinsModuleExists) {
  Runtime runtime;
  HandleScope scope;

  Dict modules(&scope, runtime.modules());
  Object name(&scope, runtime.newStrFromCStr("builtins"));
  ASSERT_TRUE(runtime.dictAt(modules, name)->isModule());
}

// Return the raw name of a builtin LayoutId, or "<invalid>" for user-defined or
// invalid LayoutIds.
static const char* layoutIdName(LayoutId id) {
  switch (id) {
    case LayoutId::kError:
      // Special-case the one type that isn't really a class so we don't have to
      // have it in INTRINSIC_CLASS_NAMES.
      return "RawError";

#define CASE(name)                                                             \
  case LayoutId::k##name:                                                      \
    return #name;
      INTRINSIC_CLASS_NAMES(CASE)
#undef CASE
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
  ASSERT_EQ(runtime.layoutAt(id)->layoutId(), LayoutId::kLayout)
      << "Bad RawLayout for " << layoutIdName(id);
  Object elt(&scope, runtime.typeAt(id));
  ASSERT_TRUE(elt->isType());
  Type cls(&scope, *elt);
  Layout layout(&scope, cls->instanceLayout());
  EXPECT_EQ(layout->id(), GetParam());
}

static const LayoutId kBuiltinHeapTypeIds[] = {
#define ENUM(x) LayoutId::k##x,
    INTRINSIC_HEAP_CLASS_NAMES(ENUM)
#undef ENUM
};

INSTANTIATE_TEST_CASE_P(BuiltinTypeIdsParameters, BuiltinTypeIdsTest,
                        ::testing::ValuesIn(kBuiltinHeapTypeIds), );

TEST(RuntimeDictTest, EmptyDictInvariants) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());

  EXPECT_EQ(dict->numItems(), 0);
  ASSERT_TRUE(dict->data()->isTuple());
  EXPECT_EQ(RawTuple::cast(dict->data())->length(), 0);
}

TEST(RuntimeDictTest, GetSet) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object key(&scope, SmallInt::fromWord(12345));

  // Looking up a key that doesn't exist should fail
  EXPECT_TRUE(runtime.dictAt(dict, key)->isError());

  // Store a value
  Object stored(&scope, SmallInt::fromWord(67890));
  runtime.dictAtPut(dict, key, stored);
  EXPECT_EQ(dict->numItems(), 1);

  // Retrieve the stored value
  RawObject retrieved = runtime.dictAt(dict, key);
  ASSERT_TRUE(retrieved->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(retrieved)->value(),
            RawSmallInt::cast(*stored)->value());

  // Overwrite the stored value
  Object new_value(&scope, SmallInt::fromWord(5555));
  runtime.dictAtPut(dict, key, new_value);
  EXPECT_EQ(dict->numItems(), 1);

  // Get the new value
  retrieved = runtime.dictAt(dict, key);
  ASSERT_TRUE(retrieved->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(retrieved)->value(),
            RawSmallInt::cast(*new_value)->value());
}

TEST(RuntimeDictTest, Remove) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object key(&scope, SmallInt::fromWord(12345));

  // Removing a key that doesn't exist should fail
  bool is_missing = runtime.dictRemove(dict, key)->isError();
  EXPECT_TRUE(is_missing);

  // Removing a key that exists should succeed and return the value that was
  // stored.
  Object stored(&scope, SmallInt::fromWord(54321));

  runtime.dictAtPut(dict, key, stored);
  EXPECT_EQ(dict->numItems(), 1);

  RawObject retrieved = runtime.dictRemove(dict, key);
  ASSERT_FALSE(retrieved->isError());
  ASSERT_EQ(RawSmallInt::cast(retrieved)->value(),
            RawSmallInt::cast(*stored)->value());

  // Looking up a key that was deleted should fail
  EXPECT_TRUE(runtime.dictAt(dict, key)->isError());
  EXPECT_EQ(dict->numItems(), 0);
}

TEST(RuntimeDictTest, Length) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());

  // Add 10 items and make sure length reflects it
  for (int i = 0; i < 10; i++) {
    Object key(&scope, SmallInt::fromWord(i));
    runtime.dictAtPut(dict, key, key);
  }
  EXPECT_EQ(dict->numItems(), 10);

  // Remove half the items
  for (int i = 0; i < 5; i++) {
    Object key(&scope, SmallInt::fromWord(i));
    ASSERT_FALSE(runtime.dictRemove(dict, key)->isError());
  }
  EXPECT_EQ(dict->numItems(), 5);
}

TEST(RuntimeDictTest, AtIfAbsentPutLength) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());

  Object k1(&scope, SmallInt::fromWord(1));
  Object v1(&scope, SmallInt::fromWord(111));
  runtime.dictAtPut(dict, k1, v1);
  EXPECT_EQ(dict->numItems(), 1);

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
  Object entry2(&scope, runtime.dictAtIfAbsentPut(dict, k2, &cb));
  EXPECT_EQ(dict->numItems(), 2);
  RawObject retrieved = runtime.dictAt(dict, k2);
  EXPECT_TRUE(retrieved->isSmallInt());
  EXPECT_EQ(retrieved, SmallInt::fromWord(222));

  // Don't overrwite existing item 1 -> v1
  Object k3(&scope, SmallInt::fromWord(1));
  SmallIntCallback cb3(333);
  Object entry3(&scope, runtime.dictAtIfAbsentPut(dict, k3, &cb3));
  EXPECT_EQ(dict->numItems(), 2);
  retrieved = runtime.dictAt(dict, k3);
  EXPECT_TRUE(retrieved->isSmallInt());
  EXPECT_EQ(retrieved, *v1);
}

TEST(RuntimeDictTest, GrowWhenFull) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());

  // Fill up the dict - we insert an initial key to force the allocation of the
  // backing Tuple.
  Object init_key(&scope, SmallInt::fromWord(0));
  runtime.dictAtPut(dict, init_key, init_key);
  ASSERT_TRUE(dict->data()->isTuple());
  word init_data_size = RawTuple::cast(dict->data())->length();

  auto make_key = [&runtime](int i) {
    byte text[]{"0123456789abcdeghiklmn"};
    return runtime.newStrWithAll(View<byte>(text + i % 10, 10));
  };
  auto make_value = [](int i) { return SmallInt::fromWord(i); };

  // Fill in one fewer keys than would require growing the underlying object
  // array again
  word num_keys = Runtime::kInitialDictCapacity;
  for (int i = 1; i < num_keys; i++) {
    Object key(&scope, make_key(i));
    Object value(&scope, make_value(i));
    runtime.dictAtPut(dict, key, value);
  }

  // Add another key which should force us to double the capacity
  Object straw(&scope, make_key(num_keys));
  Object straw_value(&scope, make_value(num_keys));
  runtime.dictAtPut(dict, straw, straw_value);
  ASSERT_TRUE(dict->data()->isTuple());
  word new_data_size = RawTuple::cast(dict->data())->length();
  EXPECT_EQ(new_data_size, Runtime::kDictGrowthFactor * init_data_size);

  // Make sure we can still read all the stored keys/values
  for (int i = 1; i <= num_keys; i++) {
    Object key(&scope, make_key(i));
    RawObject value = runtime.dictAt(dict, key);
    ASSERT_FALSE(value->isError());
    EXPECT_TRUE(Object::equals(value, make_value(i)));
  }
}

TEST(RuntimeDictTest, CollidingKeys) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());

  // Add two different keys with different values using the same hash
  Object key1(&scope, SmallInt::fromWord(1));
  runtime.dictAtPut(dict, key1, key1);

  Object key2(&scope, Bool::trueObj());
  runtime.dictAtPut(dict, key2, key2);

  // Make sure we get both back
  RawObject retrieved = runtime.dictAt(dict, key1);
  ASSERT_TRUE(retrieved->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(retrieved)->value(),
            RawSmallInt::cast(*key1)->value());

  retrieved = runtime.dictAt(dict, key2);
  ASSERT_TRUE(retrieved->isBool());
  EXPECT_EQ(RawBool::cast(retrieved)->value(), RawBool::cast(*key2)->value());
}

TEST(RuntimeDictTest, MixedKeys) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());

  // Add keys of different type
  Object int_key(&scope, SmallInt::fromWord(100));
  runtime.dictAtPut(dict, int_key, int_key);

  Object str_key(&scope, runtime.newStrFromCStr("testing 123"));
  runtime.dictAtPut(dict, str_key, str_key);

  // Make sure we get the appropriate values back out
  RawObject retrieved = runtime.dictAt(dict, int_key);
  ASSERT_TRUE(retrieved->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(retrieved)->value(),
            RawSmallInt::cast(*int_key)->value());

  retrieved = runtime.dictAt(dict, str_key);
  ASSERT_TRUE(retrieved->isStr());
  EXPECT_TRUE(Object::equals(*str_key, retrieved));
}

TEST(RuntimeDictTest, GetKeys) {
  Runtime runtime;
  HandleScope scope;

  // Create keys
  Tuple keys(&scope, runtime.newTuple(4));
  keys->atPut(0, SmallInt::fromWord(100));
  keys->atPut(1, runtime.newStrFromCStr("testing 123"));
  keys->atPut(2, Bool::trueObj());
  keys->atPut(3, NoneType::object());

  // Add keys to dict
  Dict dict(&scope, runtime.newDict());
  for (word i = 0; i < keys->length(); i++) {
    Object key(&scope, keys->at(i));
    runtime.dictAtPut(dict, key, key);
  }

  // Grab the keys and verify everything is there
  Tuple retrieved(&scope, runtime.dictKeys(dict));
  ASSERT_EQ(retrieved->length(), keys->length());
  for (word i = 0; i < keys->length(); i++) {
    Object key(&scope, keys->at(i));
    EXPECT_TRUE(tupleContains(retrieved, key)) << " missing key " << i;
  }
}

TEST(RuntimeDictTest, CanCreateDictItems) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  RawObject iter = runtime.newDictItemIterator(dict);
  ASSERT_TRUE(iter->isDictItemIterator());
}

TEST(RuntimeListTest, ListGrowth) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, runtime.newList());
  Tuple array1(&scope, runtime.newTuple(1));
  list->setItems(*array1);
  EXPECT_EQ(array1->length(), 1);
  runtime.listEnsureCapacity(list, 2);
  Tuple array2(&scope, list->items());
  EXPECT_NE(*array1, *array2);
  EXPECT_GT(array2->length(), 2);

  Tuple array4(&scope, runtime.newTuple(4));
  EXPECT_EQ(array4->length(), 4);
  list->setItems(*array4);
  runtime.listEnsureCapacity(list, 5);
  Tuple array8(&scope, list->items());
  EXPECT_NE(*array4, *array8);
  EXPECT_EQ(array8->length(), 8);
  list->setItems(*array8);
  runtime.listEnsureCapacity(list, 9);
  Tuple array16(&scope, list->items());
  EXPECT_NE(*array8, *array16);
  EXPECT_EQ(array16->length(), 16);
}

TEST(RuntimeListTest, EmptyListInvariants) {
  Runtime runtime;
  RawList list = RawList::cast(runtime.newList());
  ASSERT_EQ(list->capacity(), 0);
  ASSERT_EQ(list->numItems(), 0);
}

TEST(RuntimeListTest, AppendToList) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, runtime.newList());

  // Check that list capacity grows according to a doubling schedule
  word expected_capacity[] = {4,  4,  4,  4,  8,  8,  8,  8,
                              16, 16, 16, 16, 16, 16, 16, 16};
  for (int i = 0; i < 16; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.listAdd(list, value);
    ASSERT_EQ(list->capacity(), expected_capacity[i]);
    ASSERT_EQ(list->numItems(), i + 1);
  }

  // Sanity check list contents
  for (int i = 0; i < 16; i++) {
    RawSmallInt elem = RawSmallInt::cast(list->at(i));
    ASSERT_EQ(elem->value(), i);
  }
}

TEST(RuntimeTest, NewBytes) {
  Runtime runtime;
  HandleScope scope;

  Bytes len0(&scope, runtime.newBytes(0, 0));
  EXPECT_EQ(len0->length(), 0);

  Bytes len3(&scope, runtime.newBytes(3, 9));
  EXPECT_EQ(len3->length(), 3);
  EXPECT_EQ(len3->byteAt(0), 9);
  EXPECT_EQ(len3->byteAt(1), 9);
  EXPECT_EQ(len3->byteAt(2), 9);

  Bytes len254(&scope, runtime.newBytes(254, 0));
  EXPECT_EQ(len254->length(), 254);
  EXPECT_EQ(len254->size(), Utils::roundUp(kPointerSize + 254, kPointerSize));

  Bytes len255(&scope, runtime.newBytes(255, 0));
  EXPECT_EQ(len255->length(), 255);
  EXPECT_EQ(len255->size(),
            Utils::roundUp(kPointerSize * 2 + 255, kPointerSize));
}

TEST(RuntimeTest, NewBytesWithAll) {
  Runtime runtime;
  HandleScope scope;

  Bytes len0(&scope, runtime.newBytesWithAll(View<byte>(nullptr, 0)));
  EXPECT_EQ(len0->length(), 0);

  const byte src1[] = {0x42};
  Bytes len1(&scope, runtime.newBytesWithAll(src1));
  EXPECT_EQ(len1->length(), 1);
  EXPECT_EQ(len1->size(), Utils::roundUp(kPointerSize + 1, kPointerSize));
  EXPECT_EQ(len1->byteAt(0), 0x42);

  const byte src3[] = {0xAA, 0xBB, 0xCC};
  Bytes len3(&scope, runtime.newBytesWithAll(src3));
  EXPECT_EQ(len3->length(), 3);
  EXPECT_EQ(len3->size(), Utils::roundUp(kPointerSize + 3, kPointerSize));
  EXPECT_EQ(len3->byteAt(0), 0xAA);
  EXPECT_EQ(len3->byteAt(1), 0xBB);
  EXPECT_EQ(len3->byteAt(2), 0xCC);
}

TEST(RuntimeTest, NewTuple) {
  Runtime runtime;
  HandleScope scope;

  Tuple a0(&scope, runtime.newTuple(0));
  EXPECT_EQ(a0->length(), 0);

  Tuple a1(&scope, runtime.newTuple(1));
  ASSERT_EQ(a1->length(), 1);
  EXPECT_EQ(a1->at(0), NoneType::object());
  a1->atPut(0, SmallInt::fromWord(42));
  EXPECT_EQ(a1->at(0), SmallInt::fromWord(42));

  Tuple a300(&scope, runtime.newTuple(300));
  ASSERT_EQ(a300->length(), 300);
}

TEST(RuntimeTest, NewStr) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[400]{0};
  Str empty0(&scope, runtime.newStrWithAll(View<byte>(bytes, 0)));
  ASSERT_TRUE(empty0->isSmallStr());
  EXPECT_EQ(empty0->length(), 0);

  Str empty1(&scope, runtime.newStrWithAll(View<byte>(bytes, 0)));
  ASSERT_TRUE(empty1->isSmallStr());
  EXPECT_EQ(*empty0, *empty1);

  Str empty2(&scope, runtime.newStrFromCStr("\0"));
  ASSERT_TRUE(empty2->isSmallStr());
  EXPECT_EQ(*empty0, *empty2);

  Str s1(&scope, runtime.newStrWithAll(View<byte>(bytes, 1)));
  ASSERT_TRUE(s1->isSmallStr());
  EXPECT_EQ(s1->length(), 1);

  Str s254(&scope, runtime.newStrWithAll(View<byte>(bytes, 254)));
  EXPECT_EQ(s254->length(), 254);
  ASSERT_TRUE(s254->isLargeStr());
  EXPECT_EQ(RawHeapObject::cast(*s254)->size(),
            Utils::roundUp(kPointerSize + 254, kPointerSize));

  Str s255(&scope, runtime.newStrWithAll(View<byte>(bytes, 255)));
  EXPECT_EQ(s255->length(), 255);
  ASSERT_TRUE(s255->isLargeStr());
  EXPECT_EQ(RawHeapObject::cast(*s255)->size(),
            Utils::roundUp(kPointerSize * 2 + 255, kPointerSize));

  Str s300(&scope, runtime.newStrWithAll(View<byte>(bytes, 300)));
  ASSERT_EQ(s300->length(), 300);
}

TEST(RuntimeTest, NewStrFromFormatWithCStrArg) {
  Runtime runtime;
  HandleScope scope;

  const char input[] = "hello";
  Str str(&scope, runtime.newStrFromFormat("%s", input));
  EXPECT_PYSTRING_EQ(*str, input);
}

TEST(RuntimeTest, NewStrWithAll) {
  Runtime runtime;
  HandleScope scope;

  Str str0(&scope, runtime.newStrWithAll(View<byte>(nullptr, 0)));
  EXPECT_EQ(str0->length(), 0);
  EXPECT_TRUE(str0->equalsCStr(""));

  const byte bytes3[] = {'A', 'B', 'C'};
  Str str3(&scope, runtime.newStrWithAll(bytes3));
  EXPECT_EQ(str3->length(), 3);
  EXPECT_TRUE(str3->equalsCStr("ABC"));

  const byte bytes10[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'};
  Str str10(&scope, runtime.newStrWithAll(bytes10));
  EXPECT_EQ(str10->length(), 10);
  EXPECT_TRUE(str10->equalsCStr("ABCDEFGHIJ"));
}

TEST(RuntimeTest, NewStrFromUTF32WithZeroSizeReturnsEmpty) {
  Runtime runtime;
  HandleScope scope;
  int32 str[2] = {'a', 's'};
  Str empty(&scope, runtime.newStrFromUTF32(View<int32>(str, 0)));
  EXPECT_EQ(empty->length(), 0);
}

TEST(RuntimeTest, NewStrFromUTF32WithLargeASCIIStringReturnsString) {
  Runtime runtime;
  HandleScope scope;
  int32 str[7] = {'a', 'b', 'c', '1', '2', '3', '-'};
  Str unicode(&scope, runtime.newStrFromUTF32(View<int32>(str, 7)));
  EXPECT_EQ(unicode->length(), 7);
  EXPECT_TRUE(unicode->equalsCStr("abc123-"));
}

TEST(RuntimeTest, NewStrFromUTF32WithSmallASCIIStringReturnsString) {
  Runtime runtime;
  HandleScope scope;
  int32 str[7] = {'a', 'b'};
  Str unicode(&scope, runtime.newStrFromUTF32(View<int32>(str, 2)));
  EXPECT_EQ(unicode->length(), 2);
  EXPECT_TRUE(unicode->equalsCStr("ab"));
}

TEST(RuntimeTest, NewStrFromUTF32WithGreaterThanASCIIAborts) {
  Runtime runtime;
  int32 str[1] = {0xC4};
  EXPECT_DEATH(
      runtime.newStrFromUTF32(View<int32>(str, 1)),
      "unimplemented: PyUnicode currently only supports ASCII characters");
}

TEST(RuntimeTest, HashBools) {
  Runtime runtime;

  // In CPython, False hashes to 0 and True hashes to 1.
  RawSmallInt hash0 = RawSmallInt::cast(runtime.hash(Bool::falseObj()));
  EXPECT_EQ(hash0->value(), 0);
  RawSmallInt hash1 = RawSmallInt::cast(runtime.hash(Bool::trueObj()));
  EXPECT_EQ(hash1->value(), 1);
}

TEST(RuntimeTest, HashBytes) {
  Runtime runtime;
  HandleScope scope;

  // Strings have their hash codes computed lazily.
  const byte src1[] = {0x1, 0x2, 0x3};
  Bytes arr1(&scope, runtime.newBytesWithAll(src1));
  EXPECT_EQ(arr1->header()->hashCode(), 0);
  word hash1 = RawSmallInt::cast(runtime.hash(*arr1))->value();
  EXPECT_NE(arr1->header()->hashCode(), 0);
  EXPECT_EQ(arr1->header()->hashCode(), hash1);

  word code1 = runtime.siphash24(src1);
  EXPECT_EQ(code1 & RawHeader::kHashCodeMask, static_cast<uword>(hash1));

  // Str with different values should (ideally) hash differently.
  const byte src2[] = {0x3, 0x2, 0x1};
  Bytes arr2(&scope, runtime.newBytesWithAll(src2));
  word hash2 = RawSmallInt::cast(runtime.hash(*arr2))->value();
  EXPECT_NE(hash1, hash2);

  word code2 = runtime.siphash24(src2);
  EXPECT_EQ(code2 & RawHeader::kHashCodeMask, static_cast<uword>(hash2));

  // Strings with the same value should hash the same.
  const byte src3[] = {0x1, 0x2, 0x3};
  Bytes arr3(&scope, runtime.newBytesWithAll(src3));
  word hash3 = RawSmallInt::cast(runtime.hash(*arr3))->value();
  EXPECT_EQ(hash1, hash3);

  word code3 = runtime.siphash24(src3);
  EXPECT_EQ(code3 & RawHeader::kHashCodeMask, static_cast<uword>(hash3));
}

TEST(RuntimeTest, HashSmallInts) {
  Runtime runtime;

  // In CPython, Ints hash to themselves.
  RawSmallInt hash123 =
      RawSmallInt::cast(runtime.hash(SmallInt::fromWord(123)));
  EXPECT_EQ(hash123->value(), 123);
  RawSmallInt hash456 =
      RawSmallInt::cast(runtime.hash(SmallInt::fromWord(456)));
  EXPECT_EQ(hash456->value(), 456);
}

TEST(RuntimeTest, HashSingletonImmediates) {
  Runtime runtime;

  // In CPython, these objects hash to arbitrary values.
  word none_value = NoneType::object().raw();
  RawSmallInt hash_none = RawSmallInt::cast(runtime.hash(NoneType::object()));
  EXPECT_EQ(hash_none->value(), none_value);

  word error_value = Error::object().raw();
  RawSmallInt hash_error = RawSmallInt::cast(runtime.hash(Error::object()));
  EXPECT_EQ(hash_error->value(), error_value);
}

TEST(RuntimeTest, HashStr) {
  Runtime runtime;
  HandleScope scope;

  // LargeStr instances have their hash codes computed lazily.
  Object str1(&scope, runtime.newStrFromCStr("testing 123"));
  EXPECT_EQ(RawHeapObject::cast(*str1)->header()->hashCode(), 0);
  RawSmallInt hash1 = RawSmallInt::cast(runtime.hash(*str1));
  EXPECT_NE(RawHeapObject::cast(*str1)->header()->hashCode(), 0);
  EXPECT_EQ(RawHeapObject::cast(*str1)->header()->hashCode(), hash1->value());

  // Str with different values should (ideally) hash differently.
  Str str2(&scope, runtime.newStrFromCStr("321 testing"));
  RawSmallInt hash2 = RawSmallInt::cast(runtime.hash(*str2));
  EXPECT_NE(hash1, hash2);

  // Strings with the same value should hash the same.
  Str str3(&scope, runtime.newStrFromCStr("testing 123"));
  RawSmallInt hash3 = RawSmallInt::cast(runtime.hash(*str3));
  EXPECT_EQ(hash1, hash3);
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

TEST(RuntimeTest, HashCodeSizeCheck) {
  Runtime runtime;
  RawObject code = testing::newEmptyCode(&runtime);
  ASSERT_TRUE(code->isHeapObject());
  EXPECT_EQ(RawHeapObject::cast(code)->header()->hashCode(), 0);
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
  word hash1 = RawSmallInt::cast(runtime.hash(code))->value();
  EXPECT_EQ(hash1, 1);
}

TEST(RuntimeTest, EnsureCapacity) {
  Runtime runtime;
  HandleScope scope;

  // Check that empty arrays expand
  List list(&scope, runtime.newList());
  Tuple empty(&scope, list->items());
  runtime.listEnsureCapacity(list, 0);
  Tuple orig(&scope, list->items());
  ASSERT_NE(*empty, *orig);
  ASSERT_GT(orig->length(), 0);

  // We shouldn't grow the array if there is sufficient capacity
  runtime.listEnsureCapacity(list, orig->length() - 1);
  Tuple ensured0(&scope, list->items());
  ASSERT_EQ(*orig, *ensured0);

  // We should double the array if there is insufficient capacity
  runtime.listEnsureCapacity(list, orig->length());
  Tuple ensured1(&scope, list->items());
  ASSERT_EQ(ensured1->length(), orig->length() * 2);
}

TEST(RuntimeTest, InternLargeStr) {
  Runtime runtime;
  HandleScope scope;

  Set interned(&scope, runtime.interned());

  // Creating an ordinary large string should not affect on the intern table.
  word num_interned = interned->numItems();
  Object str1(&scope, runtime.newStrFromCStr("hello, world"));
  ASSERT_TRUE(str1->isLargeStr());
  EXPECT_EQ(num_interned, interned->numItems());
  EXPECT_FALSE(runtime.setIncludes(interned, str1));

  // Interning the string should add it to the intern table and increase the
  // size of the intern table by one.
  num_interned = interned->numItems();
  Object sym1(&scope, runtime.internStr(str1));
  EXPECT_TRUE(runtime.setIncludes(interned, str1));
  EXPECT_EQ(*sym1, *str1);
  EXPECT_EQ(num_interned + 1, interned->numItems());

  Object str2(&scope, runtime.newStrFromCStr("goodbye, world"));
  ASSERT_TRUE(str2->isLargeStr());
  EXPECT_NE(*str1, *str2);

  // Intern another string and make sure we get it back (as opposed to the
  // previously interned string).
  num_interned = interned->numItems();
  Object sym2(&scope, runtime.internStr(str2));
  EXPECT_EQ(num_interned + 1, interned->numItems());
  EXPECT_TRUE(runtime.setIncludes(interned, str2));
  EXPECT_EQ(*sym2, *str2);
  EXPECT_NE(*sym1, *sym2);

  // Create a unique copy of a previously created string.
  Object str3(&scope, runtime.newStrFromCStr("hello, world"));
  ASSERT_TRUE(str3->isLargeStr());
  EXPECT_NE(*str1, *str3);
  EXPECT_TRUE(runtime.setIncludes(interned, str3));

  // Interning a duplicate string should not affecct the intern table.
  num_interned = interned->numItems();
  Object sym3(&scope, runtime.internStr(str3));
  EXPECT_EQ(num_interned, interned->numItems());
  EXPECT_NE(*sym3, *str3);
  EXPECT_EQ(*sym3, *sym1);
}

TEST(RuntimeTest, InternSmallStr) {
  Runtime runtime;
  HandleScope scope;

  Set interned(&scope, runtime.interned());

  // Creating a small string should not affect the intern table.
  word num_interned = interned->numItems();
  Object str(&scope, runtime.newStrFromCStr("a"));
  ASSERT_TRUE(str->isSmallStr());
  EXPECT_FALSE(runtime.setIncludes(interned, str));
  EXPECT_EQ(num_interned, interned->numItems());

  // Interning a small string should have no affect on the intern table.
  Object sym(&scope, runtime.internStr(str));
  EXPECT_TRUE(sym->isSmallStr());
  EXPECT_FALSE(runtime.setIncludes(interned, str));
  EXPECT_EQ(num_interned, interned->numItems());
  EXPECT_EQ(*sym, *str);
}

TEST(RuntimeTest, InternCStr) {
  Runtime runtime;
  HandleScope scope;

  Set interned(&scope, runtime.interned());

  word num_interned = interned->numItems();
  Object sym(&scope, runtime.internStrFromCStr("hello, world"));
  EXPECT_TRUE(sym->isStr());
  EXPECT_TRUE(runtime.setIncludes(interned, sym));
  EXPECT_EQ(num_interned + 1, interned->numItems());
}

TEST(RuntimeTest, CollectAttributes) {
  Runtime runtime;
  HandleScope scope;

  Object foo(&scope, runtime.newStrFromCStr("foo"));
  Object bar(&scope, runtime.newStrFromCStr("bar"));
  Object baz(&scope, runtime.newStrFromCStr("baz"));

  Tuple names(&scope, runtime.newTuple(3));
  names->atPut(0, *foo);
  names->atPut(1, *bar);
  names->atPut(2, *baz);

  Tuple consts(&scope, runtime.newTuple(4));
  consts->atPut(0, SmallInt::fromWord(100));
  consts->atPut(1, SmallInt::fromWord(200));
  consts->atPut(2, SmallInt::fromWord(300));
  consts->atPut(3, NoneType::object());

  Code code(&scope, testing::newEmptyCode(&runtime));
  code->setNames(*names);
  // Bytecode for the snippet:
  //
  //   def __init__(self):
  //       self.foo = 100
  //       self.foo = 200
  //
  // The assignment to self.foo is intentionally duplicated to ensure that we
  // only record a single attribute name.
  const byte bc[] = {LOAD_CONST,   0, LOAD_FAST, 0, STORE_ATTR, 0,
                     LOAD_CONST,   1, LOAD_FAST, 0, STORE_ATTR, 0,
                     RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bc));

  Dict attributes(&scope, runtime.newDict());
  runtime.collectAttributes(code, attributes);

  // We should have collected a single attribute: 'foo'
  EXPECT_EQ(attributes->numItems(), 1);

  // Check that we collected 'foo'
  Object result(&scope, runtime.dictAt(attributes, foo));
  ASSERT_TRUE(result->isStr());
  EXPECT_TRUE(RawStr::cast(*result)->equals(*foo));

  // Bytecode for the snippet:
  //
  //   def __init__(self):
  //       self.bar = 200
  //       self.baz = 300
  const byte bc2[] = {LOAD_CONST,   1, LOAD_FAST, 0, STORE_ATTR, 1,
                      LOAD_CONST,   2, LOAD_FAST, 0, STORE_ATTR, 2,
                      RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bc2));
  runtime.collectAttributes(code, attributes);

  // We should have collected a two more attributes: 'bar' and 'baz'
  EXPECT_EQ(attributes->numItems(), 3);

  // Check that we collected 'bar'
  result = runtime.dictAt(attributes, bar);
  ASSERT_TRUE(result->isStr());
  EXPECT_TRUE(RawStr::cast(*result)->equals(*bar));

  // Check that we collected 'baz'
  result = runtime.dictAt(attributes, baz);
  ASSERT_TRUE(result->isStr());
  EXPECT_TRUE(RawStr::cast(*result)->equals(*baz));
}

TEST(RuntimeTest, CollectAttributesWithExtendedArg) {
  Runtime runtime;
  HandleScope scope;

  Object foo(&scope, runtime.newStrFromCStr("foo"));
  Object bar(&scope, runtime.newStrFromCStr("bar"));

  Tuple names(&scope, runtime.newTuple(2));
  names->atPut(0, *foo);
  names->atPut(1, *bar);

  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, NoneType::object());

  Code code(&scope, testing::newEmptyCode(&runtime));
  code->setNames(*names);
  // Bytecode for the snippet:
  //
  //   def __init__(self):
  //       self.foo = None
  //
  // There is an additional LOAD_FAST that is preceded by an EXTENDED_ARG
  // that must be skipped.
  const byte bc[] = {LOAD_CONST, 0, EXTENDED_ARG, 10, LOAD_FAST, 0,
                     STORE_ATTR, 1, LOAD_CONST,   0,  LOAD_FAST, 0,
                     STORE_ATTR, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bc));

  Dict attributes(&scope, runtime.newDict());
  runtime.collectAttributes(code, attributes);

  // We should have collected a single attribute: 'foo'
  EXPECT_EQ(attributes->numItems(), 1);

  // Check that we collected 'foo'
  Object result(&scope, runtime.dictAt(attributes, foo));
  ASSERT_TRUE(result->isStr());
  EXPECT_TRUE(RawStr::cast(*result)->equals(*foo));
}

TEST(RuntimeTest, GetTypeConstructor) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.newType());
  Dict type_dict(&scope, runtime.newDict());
  type->setDict(*type_dict);

  EXPECT_EQ(runtime.classConstructor(type), NoneType::object());

  Object init(&scope, runtime.symbols()->DunderInit());
  Object func(&scope, runtime.newFunction());
  runtime.dictAtPutInValueCell(type_dict, init, func);

  EXPECT_EQ(runtime.classConstructor(type), *func);
}

TEST(RuntimeTest, NewInstanceEmptyClass) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, "class MyEmptyClass: pass");

  Module main(&scope, findModule(&runtime, "__main__"));
  Type type(&scope, moduleAt(&runtime, main, "MyEmptyClass"));
  Layout layout(&scope, type->instanceLayout());
  EXPECT_EQ(layout->instanceSize(), 1 * kPointerSize);

  Type cls(&scope, layout->describedType());
  EXPECT_PYSTRING_EQ(RawStr::cast(cls->name()), "MyEmptyClass");

  Instance instance(&scope, runtime.newInstance(layout));
  EXPECT_TRUE(instance->isInstance());
  EXPECT_EQ(instance->header()->layoutId(), layout->id());
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
  runFromCStr(&runtime, src);

  Module main(&scope, findModule(&runtime, "__main__"));
  Type type(&scope, moduleAt(&runtime, main, "MyTypeWithAttributes"));
  Layout layout(&scope, type->instanceLayout());
  ASSERT_EQ(layout->instanceSize(), 4 * kPointerSize);

  Type cls(&scope, layout->describedType());
  EXPECT_PYSTRING_EQ(RawStr::cast(cls->name()), "MyTypeWithAttributes");

  Instance instance(&scope, runtime.newInstance(layout));
  EXPECT_TRUE(instance->isInstance());
  EXPECT_EQ(instance->header()->layoutId(), layout->id());
}

TEST(RuntimeTest, VerifySymbols) {
  Runtime runtime;
  Symbols* symbols = runtime.symbols();
  for (int i = 0; i < static_cast<int>(SymbolId::kMaxId); i++) {
    SymbolId id = static_cast<SymbolId>(i);
    RawObject value = symbols->at(id);
    ASSERT_TRUE(value->isStr());
    const char* expected = symbols->literalAt(id);
    EXPECT_TRUE(RawStr::cast(value)->equalsCStr(expected))
        << "Incorrect symbol value for " << expected;
  }
}

static RawStr className(Runtime* runtime, RawObject o) {
  auto cls = RawType::cast(runtime->typeOf(o));
  auto name = RawStr::cast(cls->name());
  return name;
}

TEST(RuntimeTest, TypeIds) {
  Runtime runtime;
  HandleScope scope;

  EXPECT_PYSTRING_EQ(className(&runtime, Bool::trueObj()), "bool");
  EXPECT_PYSTRING_EQ(className(&runtime, NoneType::object()), "NoneType");
  EXPECT_PYSTRING_EQ(className(&runtime, runtime.newStrFromCStr("abc")),
                     "smallstr");

  for (word i = 0; i < 16; i++) {
    auto small_int = SmallInt::fromWord(i);
    EXPECT_PYSTRING_EQ(className(&runtime, small_int), "smallint");
  }
}

TEST(RuntimeTest, CallRunTwice) {
  Runtime runtime;
  runFromCStr(&runtime, "x = 42");
  runFromCStr(&runtime, "y = 1764");

  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Object x(&scope, moduleAt(&runtime, main, "x"));
  EXPECT_TRUE(x->isSmallInt());
  EXPECT_EQ(SmallInt::cast(x)->value(), 42);
  Object y(&scope, moduleAt(&runtime, main, "y"));
  EXPECT_TRUE(y->isSmallInt());
  EXPECT_EQ(SmallInt::cast(y)->value(), 1764);
}

struct LookupNameInMroData {
  const char* test_name;
  const char* name;
  RawObject expected;
};

static std::string lookupNameInMroTestName(
    ::testing::TestParamInfo<LookupNameInMroData> info) {
  return info.param.test_name;
}

class LookupNameInMroTest
    : public ::testing::TestWithParam<LookupNameInMroData> {};

TEST_P(LookupNameInMroTest, Lookup) {
  Runtime runtime;
  HandleScope scope;

  auto create_class_with_attr = [&](const char* attr, word value) {
    Type type(&scope, runtime.newType());
    Dict dict(&scope, type->dict());
    Object key(&scope, runtime.newStrFromCStr(attr));
    Object val(&scope, SmallInt::fromWord(value));
    runtime.dictAtPutInValueCell(dict, key, val);
    return *type;
  };

  Tuple mro(&scope, runtime.newTuple(3));
  mro->atPut(0, create_class_with_attr("foo", 2));
  mro->atPut(1, create_class_with_attr("bar", 4));
  mro->atPut(2, create_class_with_attr("baz", 8));

  Type type(&scope, mro->at(0));
  type->setMro(*mro);

  auto param = GetParam();
  Object key(&scope, runtime.newStrFromCStr(param.name));
  RawObject result =
      runtime.lookupNameInMro(Thread::currentThread(), type, key);
  EXPECT_EQ(result, param.expected);
}

INSTANTIATE_TEST_CASE_P(
    LookupNameInMro, LookupNameInMroTest,
    ::testing::Values(
        LookupNameInMroData{"OnInstance", "foo", SmallInt::fromWord(2)},
        LookupNameInMroData{"OnParent", "bar", SmallInt::fromWord(4)},
        LookupNameInMroData{"OnGrandParent", "baz", SmallInt::fromWord(8)},
        LookupNameInMroData{"NonExistent", "xxx", Error::object()}),
    lookupNameInMroTestName);

TEST(RuntimeTypeCallTest, TypeCallNoInitMethod) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class MyTypeWithNoInitMethod():
  def m(self):
    pass

c = MyTypeWithNoInitMethod()
)";
  runFromCStr(&runtime, src);

  Module main(&scope, findModule(&runtime, "__main__"));
  Object instance(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(instance->isInstance());
  LayoutId layout_id = instance->layoutId();
  Layout layout(&scope, runtime.layoutAt(layout_id));
  EXPECT_EQ(layout->instanceSize(), 1 * kPointerSize);

  Type cls(&scope, layout->describedType());
  EXPECT_PYSTRING_EQ(RawStr::cast(cls->name()), "MyTypeWithNoInitMethod");
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
  runFromCStr(&runtime, src);

  Module main(&scope, findModule(&runtime, "__main__"));
  Object instance(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(instance->isInstance());
  LayoutId layout_id = instance->layoutId();
  Layout layout(&scope, runtime.layoutAt(layout_id));
  EXPECT_EQ(layout->instanceSize(), 1 * kPointerSize);

  Type cls(&scope, layout->describedType());
  EXPECT_PYSTRING_EQ(RawStr::cast(cls->name()), "MyTypeWithEmptyInitMethod");
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
  runFromCStr(&runtime, src);

  Module main(&scope, findModule(&runtime, "__main__"));
  Type type(&scope, moduleAt(&runtime, main, "MyTypeWithAttributes"));
  Object instance(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(instance->isInstance());
  LayoutId layout_id = instance->layoutId();
  // Since this class has extra attributes, its layout id should be greater than
  // the layout id from the type.
  ASSERT_GT(layout_id, RawLayout::cast(type->instanceLayout())->id());
  Layout layout(&scope, runtime.layoutAt(layout_id));
  ASSERT_EQ(layout->instanceSize(), 2 * kPointerSize);

  Type cls(&scope, layout->describedType());
  EXPECT_PYSTRING_EQ(RawStr::cast(cls->name()), "MyTypeWithAttributes");

  Object name(&scope, runtime.newStrFromCStr("x"));
  Object value(&scope,
               runtime.attributeAt(Thread::currentThread(), instance, name));
  EXPECT_FALSE(value->isError());
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
  runFromCStr(&runtime, src);
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
  Code code(&scope, func->code());
  ASSERT_EQ(code->firstlineno(), 2);

  // a = 1
  Thread* thread = Thread::currentThread();
  EXPECT_EQ(runtime.codeOffsetToLineNum(thread, code, 0), 3);
  EXPECT_EQ(runtime.codeOffsetToLineNum(thread, code, 2), 3);

  // b = 2
  EXPECT_EQ(runtime.codeOffsetToLineNum(thread, code, 4), 4);
  EXPECT_EQ(runtime.codeOffsetToLineNum(thread, code, 6), 4);

  // print(a, b)
  for (word i = 8; i < RawBytes::cast(code->code())->length(); i++) {
    EXPECT_EQ(runtime.codeOffsetToLineNum(thread, code, i), 5);
  }
}

TEST(RuntimeTest, IsInstanceOf) {
  Runtime runtime;
  HandleScope scope;

  Object i(&scope, runtime.newInt(123));
  EXPECT_TRUE(runtime.isInstanceOfInt(*i));
  EXPECT_FALSE(runtime.isInstanceOfStr(*i));

  Object str(&scope, runtime.newStrFromCStr("this is a long string"));
  EXPECT_TRUE(runtime.isInstanceOfStr(*str));
  EXPECT_FALSE(runtime.isInstanceOfInt(*str));

  runFromCStr(&runtime, R"(
class StopIterationSub(StopIteration):
  pass
stop_iteration = StopIterationSub()
  )");
  Object stop_iteration(&scope,
                        moduleAt(&runtime, "__main__", "stop_iteration"));
  EXPECT_TRUE(runtime.isInstanceOfStopIteration(*stop_iteration));
  EXPECT_TRUE(runtime.isInstanceOfBaseException(*stop_iteration));
  EXPECT_FALSE(runtime.isInstanceOfSystemExit(*stop_iteration));
}

TEST(RuntimeTupleTest, Create) {
  Runtime runtime;

  RawObject obj0 = runtime.newTuple(0);
  ASSERT_TRUE(obj0->isTuple());
  RawTuple array0 = RawTuple::cast(obj0);
  EXPECT_EQ(array0->length(), 0);

  RawObject obj1 = runtime.newTuple(1);
  ASSERT_TRUE(obj1->isTuple());
  RawTuple array1 = RawTuple::cast(obj1);
  EXPECT_EQ(array1->length(), 1);

  RawObject obj7 = runtime.newTuple(7);
  ASSERT_TRUE(obj7->isTuple());
  RawTuple array7 = RawTuple::cast(obj7);
  EXPECT_EQ(array7->length(), 7);

  RawObject obj8 = runtime.newTuple(8);
  ASSERT_TRUE(obj8->isTuple());
  RawTuple array8 = RawTuple::cast(obj8);
  EXPECT_EQ(array8->length(), 8);
}

TEST(RuntimeSetTest, EmptySetInvariants) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());

  EXPECT_EQ(set->numItems(), 0);
  ASSERT_TRUE(set->isSet());
  ASSERT_TRUE(set->data()->isTuple());
  EXPECT_EQ(RawTuple::cast(set->data())->length(), 0);
}

TEST(RuntimeSetTest, Add) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Object value(&scope, SmallInt::fromWord(12345));

  // Store a value
  runtime.setAdd(set, value);
  EXPECT_EQ(set->numItems(), 1);

  // Retrieve the stored value
  ASSERT_TRUE(runtime.setIncludes(set, value));

  // Add a new value
  Object new_value(&scope, SmallInt::fromWord(5555));
  runtime.setAdd(set, new_value);
  EXPECT_EQ(set->numItems(), 2);

  // Get the new value
  ASSERT_TRUE(runtime.setIncludes(set, new_value));

  // Add a existing value
  Object same_value(&scope, SmallInt::fromWord(12345));
  RawObject old_value = runtime.setAdd(set, same_value);
  EXPECT_EQ(set->numItems(), 2);
  EXPECT_EQ(old_value, *value);
}

TEST(RuntimeSetTest, Remove) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Object value(&scope, SmallInt::fromWord(12345));

  // Removing a key that doesn't exist should fail
  EXPECT_FALSE(runtime.setRemove(set, value));

  runtime.setAdd(set, value);
  EXPECT_EQ(set->numItems(), 1);

  ASSERT_TRUE(runtime.setRemove(set, value));
  EXPECT_EQ(set->numItems(), 0);

  // Looking up a key that was deleted should fail
  ASSERT_FALSE(runtime.setIncludes(set, value));
}

TEST(RuntimeSetTest, Grow) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());

  // Fill up the dict - we insert an initial key to force the allocation of the
  // backing Tuple.
  Object init_key(&scope, SmallInt::fromWord(0));
  runtime.setAdd(set, init_key);
  ASSERT_TRUE(set->data()->isTuple());
  word init_data_size = RawTuple::cast(set->data())->length();

  auto make_key = [&runtime](int i) {
    byte text[]{"0123456789abcdeghiklmn"};
    return runtime.newStrWithAll(View<byte>(text + i % 10, 10));
  };

  // Fill in one fewer keys than would require growing the underlying object
  // array again
  word num_keys = Runtime::kInitialSetCapacity;
  for (int i = 1; i < num_keys; i++) {
    Object key(&scope, make_key(i));
    runtime.setAdd(set, key);
  }

  // Add another key which should force us to double the capacity
  Object straw(&scope, make_key(num_keys));
  runtime.setAdd(set, straw);
  ASSERT_TRUE(set->data()->isTuple());
  word new_data_size = RawTuple::cast(set->data())->length();
  EXPECT_EQ(new_data_size, Runtime::kSetGrowthFactor * init_data_size);

  // Make sure we can still read all the stored keys
  for (int i = 1; i <= num_keys; i++) {
    Object key(&scope, make_key(i));
    bool found = runtime.setIncludes(set, key);
    ASSERT_TRUE(found);
  }
}

TEST(RuntimeSetTest, UpdateSet) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Set set1(&scope, runtime.newSet());
  Object set1_handle(&scope, *set1);
  for (word i = 0; i < 8; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(set, value);
  }
  runtime.setUpdate(Thread::currentThread(), set, set1_handle);
  ASSERT_EQ(set->numItems(), 8);
  for (word i = 4; i < 12; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(set1, value);
  }
  runtime.setUpdate(Thread::currentThread(), set, set1_handle);
  ASSERT_EQ(set->numItems(), 12);
  runtime.setUpdate(Thread::currentThread(), set, set1_handle);
  ASSERT_EQ(set->numItems(), 12);
}

TEST(RuntimeSetTest, UpdateList) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, runtime.newList());
  Set set(&scope, runtime.newSet());
  for (word i = 0; i < 8; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.listAdd(list, value);
  }
  for (word i = 4; i < 12; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(set, value);
  }
  ASSERT_EQ(set->numItems(), 8);
  Object list_handle(&scope, *list);
  runtime.setUpdate(Thread::currentThread(), set, list_handle);
  ASSERT_EQ(set->numItems(), 12);
  runtime.setUpdate(Thread::currentThread(), set, list_handle);
  ASSERT_EQ(set->numItems(), 12);
}

TEST(RuntimeSetTest, UpdateListIterator) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, runtime.newList());
  Set set(&scope, runtime.newSet());
  for (word i = 0; i < 8; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.listAdd(list, value);
  }
  for (word i = 4; i < 12; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(set, value);
  }
  ASSERT_EQ(set->numItems(), 8);
  Object list_handle(&scope, *list);
  Object list_iterator(&scope, runtime.newListIterator(list_handle));
  runtime.setUpdate(Thread::currentThread(), set, list_iterator);
  ASSERT_EQ(set->numItems(), 12);
}

TEST(RuntimeSetTest, UpdateTuple) {
  Runtime runtime;
  HandleScope scope;
  Tuple object_array(&scope, runtime.newTuple(8));
  Set set(&scope, runtime.newSet());
  for (word i = 0; i < 8; i++) {
    object_array->atPut(i, SmallInt::fromWord(i));
  }
  for (word i = 4; i < 12; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(set, value);
  }
  ASSERT_EQ(set->numItems(), 8);
  Object object_array_handle(&scope, *object_array);
  runtime.setUpdate(Thread::currentThread(), set, object_array_handle);
  ASSERT_EQ(set->numItems(), 12);
}

TEST(RuntimeSetTest, UpdateIterator) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Object iterable(&scope, runtime.newRange(1, 4, 1));
  runtime.setUpdate(Thread::currentThread(), set, iterable);

  ASSERT_EQ(set->numItems(), 3);
}

TEST(RuntimeSetTest, UpdateWithNonIterable) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Object non_iterable(&scope, NoneType::object());
  Object result(&scope,
                runtime.setUpdate(Thread::currentThread(), set, non_iterable));
  ASSERT_TRUE(result->isError());
}

TEST(RuntimeSetTest, EmptySetItersectionReturnsEmptySet) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Set set1(&scope, runtime.newSet());

  // set() & set()
  Object result(&scope, runtime.setIntersection(thread, set, set1));
  ASSERT_TRUE(result->isSet());
  EXPECT_EQ(RawSet::cast(*result)->numItems(), 0);
}

TEST(RuntimeSetTest, ItersectionWithEmptySetReturnsEmptySet) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Set set1(&scope, runtime.newSet());

  for (word i = 0; i < 8; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(set1, value);
  }

  // set() & {0, 1, 2, 3, 4, 5, 6, 7}
  Object result(&scope, runtime.setIntersection(thread, set, set1));
  ASSERT_TRUE(result->isSet());
  EXPECT_EQ(RawSet::cast(*result)->numItems(), 0);

  // {0, 1, 2, 3, 4, 5, 6, 7} & set()
  Object result1(&scope, runtime.setIntersection(thread, set1, set));
  ASSERT_TRUE(result1->isSet());
  EXPECT_EQ(RawSet::cast(*result1)->numItems(), 0);
}

TEST(RuntimeSetTest, IntersectionReturnsSetWithCommonElements) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Set set1(&scope, runtime.newSet());
  Object key(&scope, NoneType::object());

  for (word i = 0; i < 8; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(set1, value);
  }

  for (word i = 0; i < 4; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(set, value);
  }

  // {0, 1, 2, 3} & {0, 1, 2, 3, 4, 5, 6, 7}
  Set result(&scope, runtime.setIntersection(thread, set, set1));
  EXPECT_EQ(RawSet::cast(*result)->numItems(), 4);
  key = SmallInt::fromWord(0);
  EXPECT_TRUE(runtime.setIncludes(result, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(runtime.setIncludes(result, key));
  key = SmallInt::fromWord(2);
  EXPECT_TRUE(runtime.setIncludes(result, key));
  key = SmallInt::fromWord(3);
  EXPECT_TRUE(runtime.setIncludes(result, key));

  // {0, 1, 2, 3, 4, 5, 6, 7} & {0, 1, 2, 3}
  Set result1(&scope, runtime.setIntersection(thread, set, set1));
  EXPECT_EQ(RawSet::cast(*result1)->numItems(), 4);
  key = SmallInt::fromWord(0);
  EXPECT_TRUE(runtime.setIncludes(result1, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(runtime.setIncludes(result1, key));
  key = SmallInt::fromWord(2);
  EXPECT_TRUE(runtime.setIncludes(result1, key));
  key = SmallInt::fromWord(3);
  EXPECT_TRUE(runtime.setIncludes(result1, key));
}

TEST(RuntimeSetTest, IntersectIterator) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Object iterable(&scope, runtime.newRange(1, 4, 1));
  Set result(&scope, runtime.setIntersection(thread, set, iterable));
  EXPECT_EQ(result->numItems(), 0);

  Object key(&scope, SmallInt::fromWord(1));
  runtime.setAdd(set, key);
  key = SmallInt::fromWord(2);
  runtime.setAdd(set, key);
  Object iterable1(&scope, runtime.newRange(1, 4, 1));
  Set result1(&scope, runtime.setIntersection(thread, set, iterable1));
  EXPECT_EQ(result1->numItems(), 2);
  EXPECT_TRUE(runtime.setIncludes(result1, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(runtime.setIncludes(result1, key));
}

TEST(RuntimeSetTest, IntersectWithNonIterable) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Object non_iterable(&scope, NoneType::object());

  Object result(&scope, runtime.setIntersection(thread, set, non_iterable));
  ASSERT_TRUE(result->isError());
}

// Attribute tests

static RawObject createType(Runtime* runtime) {
  HandleScope scope;
  Type type(&scope, runtime->newType());
  Thread* thread = Thread::currentThread();
  Layout layout(&scope, runtime->layoutCreateEmpty(thread));
  layout->setDescribedType(*type);
  type->setInstanceLayout(*layout);
  Tuple mro(&scope, runtime->newTuple(1));
  mro->atPut(0, *type);
  type->setMro(*mro);
  layout->setId(runtime->reserveLayoutId());
  runtime->layoutAtPut(layout->id(), *layout);
  return *type;
}

static void setInTypeDict(Runtime* runtime, const Object& type,
                          const Object& attr, const Object& value) {
  HandleScope scope;
  Type k(&scope, *type);
  Dict type_dict(&scope, k->dict());
  runtime->dictAtPutInValueCell(type_dict, attr, value);
}

static void setInMetaclass(Runtime* runtime, const Object& type,
                           const Object& attr, const Object& value) {
  HandleScope scope;
  Object meta_type(&scope, runtime->typeOf(*type));
  setInTypeDict(runtime, meta_type, attr, value);
}

// Get an attribute that corresponds to a function on the metaclass
TEST(TypeGetAttrTest, MetaClassFunction) {
  Runtime runtime;
  HandleScope scope;
  Object type(&scope, createType(&runtime));

  // Store the function on the metaclass
  Object attr(&scope, runtime.newStrFromCStr("test"));
  Object value(&scope, runtime.newFunction());
  setInMetaclass(&runtime, type, attr, value);

  // Fetch it from the class and ensure the bound method was created
  RawObject result = runtime.attributeAt(Thread::currentThread(), type, attr);
  ASSERT_TRUE(result->isBoundMethod());
  BoundMethod bm(&scope, result);
  EXPECT_TRUE(Object::equals(bm->function(), *value));
  EXPECT_TRUE(Object::equals(bm->self(), *type));
}

// Get an attribute that resides on the metaclass
TEST(TypeGetAttrTest, MetaclassAttr) {
  Runtime runtime;
  HandleScope scope;
  Object type(&scope, createType(&runtime));

  // Store the attribute on the metaclass
  Object attr(&scope, runtime.newStrFromCStr("test"));
  Object value(&scope, SmallInt::fromWord(100));
  setInMetaclass(&runtime, type, attr, value);

  // Fetch it from the class
  RawObject result = runtime.attributeAt(Thread::currentThread(), type, attr);
  EXPECT_TRUE(Object::equals(result, *value));
}

// Get an attribute that resides on the class and shadows an attribute on
// the metaclass
TEST(TypeGetAttrTest, ShadowingAttr) {
  Runtime runtime;
  HandleScope scope;
  Object type(&scope, createType(&runtime));

  // Store the attribute on the metaclass
  Object attr(&scope, runtime.newStrFromCStr("test"));
  Object meta_type_value(&scope, SmallInt::fromWord(100));
  setInMetaclass(&runtime, type, attr, meta_type_value);

  // Store the attribute on the class so that it shadows the attr
  // on the metaclass
  Object type_value(&scope, SmallInt::fromWord(200));
  setInTypeDict(&runtime, type, attr, type_value);

  // Fetch it from the class
  RawObject result = runtime.attributeAt(Thread::currentThread(), type, attr);
  EXPECT_TRUE(Object::equals(result, *type_value));
}

struct IntrinsicTypeSetAttrTestData {
  LayoutId layout_id;
  const char* name;
};

IntrinsicTypeSetAttrTestData kIntrinsicTypeSetAttrTests[] = {
// clang-format off
#define DEFINE_TEST(class_name) {LayoutId::k##class_name, #class_name},
  INTRINSIC_CLASS_NAMES(DEFINE_TEST)
#undef DEFINE_TEST
    // clang-format on
};

static std::string intrinsicTypeName(
    ::testing::TestParamInfo<IntrinsicTypeSetAttrTestData> info) {
  return info.param.name;
}

class IntrinsicTypeSetAttrTest
    : public ::testing::TestWithParam<IntrinsicTypeSetAttrTestData> {};

TEST_P(IntrinsicTypeSetAttrTest, SetAttr) {
  Runtime runtime;
  HandleScope scope;
  Object type(&scope, runtime.typeAt(GetParam().layout_id));
  Object attr(&scope, runtime.newStrFromCStr("test"));
  Object value(&scope, SmallInt::fromWord(100));
  Thread* thread = Thread::currentThread();

  RawObject result = runtime.attributeAtPut(thread, type, attr, value);

  EXPECT_TRUE(result->isError());
  ASSERT_TRUE(thread->pendingExceptionValue()->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(thread->pendingExceptionValue()),
                     "can't set attributes of built-in/extension type");
}

INSTANTIATE_TEST_CASE_P(IntrinsicTypes, IntrinsicTypeSetAttrTest,
                        ::testing::ValuesIn(kIntrinsicTypeSetAttrTests),
                        intrinsicTypeName);

// Set an attribute directly on the class
TEST(TypeAttributeTest, SetAttrOnType) {
  Runtime runtime;
  HandleScope scope;

  Object type(&scope, createType(&runtime));
  Object attr(&scope, runtime.newStrFromCStr("test"));
  Object value(&scope, SmallInt::fromWord(100));

  RawObject result =
      runtime.attributeAtPut(Thread::currentThread(), type, attr, value);
  ASSERT_FALSE(result->isError());

  Dict type_dict(&scope, RawType::cast(*type)->dict());
  Object value_cell(&scope, runtime.dictAt(type_dict, attr));
  ASSERT_TRUE(value_cell->isValueCell());
  EXPECT_EQ(RawValueCell::cast(*value_cell)->value(), SmallInt::fromWord(100));
}

TEST(TypeAttributeTest, Simple) {
  Runtime runtime;
  const char* src = R"(
class A:
  foo = 'hello'
print(A.foo)
)";
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hello\n");
}

TEST(TypeAttributeTest, SingleInheritance) {
  Runtime runtime;
  const char* src = R"(
class A:
  foo = 'hello'
class B(A): pass
class C(B): pass
print(A.foo, B.foo, C.foo)
B.foo = 123
print(A.foo, B.foo, C.foo)
)";
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hello hello hello\nhello 123 123\n");
}

TEST(TypeAttributeTest, MultipleInheritance) {
  Runtime runtime;
  const char* src = R"(
class A:
  foo = 'hello'
class B:
  bar = 'there'
class C(B, A): pass
print(C.foo, C.bar)
)";
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hello there\n");
}

TEST(ClassAttributeDeathTest, GetMissingAttribute) {
  Runtime runtime;
  const char* src = R"(
class A: pass
print(A.foo)
)";
  ASSERT_DEATH(runFromCStr(&runtime, src),
               "aborting due to pending exception: missing attribute");
}

TEST(TypeAttributeTest, GetFunction) {
  Runtime runtime;
  const char* src = R"(
class Foo:
  def bar(self):
    print(self)
Foo.bar('testing 123')
)";
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "testing 123\n");
}

TEST(ClassAttributeDeathTest, GetDataDescriptorOnMetaclass) {
  Runtime runtime;

  // Create the data descriptor class
  const char* src = R"(
class DataDescriptor:
  def __set__(self, instance, value):
    pass

  def __get__(self, instance, owner):
    pass
)";
  runFromCStr(&runtime, src);
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Type descr_type(&scope, moduleAt(&runtime, main, "DataDescriptor"));

  // Create the class
  Object type(&scope, createType(&runtime));

  // Create an instance of the descriptor and store it on the metaclass
  Object attr(&scope, runtime.newStrFromCStr("test"));
  Layout layout(&scope, descr_type->instanceLayout());
  Object descr(&scope, runtime.newInstance(layout));
  setInMetaclass(&runtime, type, attr, descr);

  ASSERT_DEATH(runtime.attributeAt(Thread::currentThread(), type, attr),
               "custom descriptors are unsupported");
}

TEST(TypeAttributeTest, GetNonDataDescriptorOnMetaclass) {
  Runtime runtime;

  // Create the non-data descriptor class
  const char* src = R"(
class DataDescriptor:
  def __get__(self, instance, owner):
    return (self, instance, owner)
)";
  runFromCStr(&runtime, src);
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Type descr_type(&scope, moduleAt(&runtime, main, "DataDescriptor"));

  // Create the class
  Object type(&scope, createType(&runtime));

  // Create an instance of the descriptor and store it on the metaclass
  Object attr(&scope, runtime.newStrFromCStr("test"));
  Layout layout(&scope, descr_type->instanceLayout());
  Object descr(&scope, runtime.newInstance(layout));
  setInMetaclass(&runtime, type, attr, descr);

  RawObject result = runtime.attributeAt(Thread::currentThread(), type, attr);
  ASSERT_EQ(RawTuple::cast(result)->length(), 3);
  EXPECT_EQ(runtime.typeOf(RawTuple::cast(result)->at(0)), *descr_type);
  EXPECT_EQ(RawTuple::cast(result)->at(1), *type);
  EXPECT_EQ(RawTuple::cast(result)->at(2), runtime.typeOf(*type));
}

TEST(TypeAttributeTest, GetNonDataDescriptorOnType) {
  Runtime runtime;

  // Create the non-data descriptor class
  const char* src = R"(
class DataDescriptor:
  def __get__(self, instance, owner):
    return (self, instance, owner)
)";
  runFromCStr(&runtime, src);
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Type descr_type(&scope, moduleAt(&runtime, main, "DataDescriptor"));

  // Create the class
  Object type(&scope, createType(&runtime));

  // Create an instance of the descriptor and store it on the metaclass
  Object attr(&scope, runtime.newStrFromCStr("test"));
  Layout layout(&scope, descr_type->instanceLayout());
  Object descr(&scope, runtime.newInstance(layout));
  setInTypeDict(&runtime, type, attr, descr);

  RawObject result = runtime.attributeAt(Thread::currentThread(), type, attr);
  ASSERT_EQ(RawTuple::cast(result)->length(), 3);
  EXPECT_EQ(runtime.typeOf(RawTuple::cast(result)->at(0)), *descr_type);
  EXPECT_EQ(RawTuple::cast(result)->at(1), NoneType::object());
  EXPECT_EQ(RawTuple::cast(result)->at(2), *type);
}

TEST(GetTypeAttributeTest, GetMetaclassAttribute) {
  Runtime runtime;
  const char* src = R"(
class MyMeta(type):
    attr = 'foo'

class Foo(metaclass=MyMeta):
    pass
)";
  runFromCStr(&runtime, src);
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Object foo(&scope, moduleAt(&runtime, main, "Foo"));
  Object attr(&scope, runtime.newStrFromCStr("attr"));
  Object result(&scope,
                runtime.attributeAt(Thread::currentThread(), foo, attr));
  ASSERT_TRUE(result->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result), "foo");
}

TEST(InstanceAttributeTest, GetMissingAttributeThrowsAttributeError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo: pass
caught_attribute_error = False
try:
  Foo().non_existant_attribute
except AttributeError:
  caught_attribute_error = True
)");
  HandleScope scope;
  Object res(&scope, moduleAt(&runtime, "__main__", "caught_attribute_error"));
  EXPECT_EQ(*res, Bool::trueObj());
}

// Fetch an attribute defined on the class
TEST(InstanceAttributeTest, GetClassAttribute) {
  Runtime runtime;
  const char* src = R"(
class Foo:
  attr = 'testing 123'

def test(x):
  print(x.attr)
)";
  runFromCStr(&runtime, src);

  // Create the instance
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Function test(&scope, moduleAt(&runtime, main, "test"));
  Type type(&scope, moduleAt(&runtime, main, "Foo"));
  Tuple args(&scope, runtime.newTuple(1));
  Layout layout(&scope, type->instanceLayout());
  args->atPut(0, runtime.newInstance(layout));

  EXPECT_EQ(callFunctionToString(test, args), "testing 123\n");
}

// Fetch an attribute defined in __init__
TEST(InstanceAttributeTest, GetInstanceAttribute) {
  Runtime runtime;
  const char* src = R"(
class Foo:
  def __init__(self):
    self.attr = 'testing 123'

def test(x):
  Foo.__init__(x)
  print(x.attr)
)";
  runFromCStr(&runtime, src);

  // Create the instance
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Type type(&scope, moduleAt(&runtime, main, "Foo"));
  Tuple args(&scope, runtime.newTuple(1));
  Layout layout(&scope, type->instanceLayout());
  args->atPut(0, runtime.newInstance(layout));

  // Run __init__
  Function test(&scope, moduleAt(&runtime, main, "test"));
  EXPECT_EQ(callFunctionToString(test, args), "testing 123\n");
}

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
  runFromCStr(&runtime, src);

  // Create the instance
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Type type(&scope, moduleAt(&runtime, main, "Foo"));
  Tuple args(&scope, runtime.newTuple(1));
  Layout layout(&scope, type->instanceLayout());
  args->atPut(0, runtime.newInstance(layout));

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
  runFromCStr(&runtime, src);

  // Create an instance of Foo
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Type type(&scope, moduleAt(&runtime, main, "Foo"));
  Layout layout(&scope, type->instanceLayout());
  Instance foo1(&scope, runtime.newInstance(layout));
  LayoutId original_layout_id = layout->id();

  // Add overflow attributes that should force layout transitions
  Tuple args(&scope, runtime.newTuple(1));
  args->atPut(0, *foo1);
  Function test(&scope, moduleAt(&runtime, main, "test"));
  EXPECT_EQ(callFunctionToString(test, args), "100 200 hello\naaa bbb ccc\n");
  EXPECT_NE(foo1->layoutId(), original_layout_id);

  // Add the same set of attributes to a new instance, should arrive at the
  // same layout
  Instance foo2(&scope, runtime.newInstance(layout));
  args->atPut(0, *foo2);
  EXPECT_EQ(callFunctionToString(test, args), "100 200 hello\naaa bbb ccc\n");
}

// This is the real deal
TEST(InstanceAttributeTest, CallInstanceMethod) {
  Runtime runtime;
  const char* src = R"(
class Foo:
  def __init__(self):
    self.attr = 'testing 123'

  def doit(self):
    print(self.attr)
    self.attr = '321 testing'
    print(self.attr)

def test(x):
  Foo.__init__(x)
  x.doit()
)";
  runFromCStr(&runtime, src);

  // Create the instance
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Type type(&scope, moduleAt(&runtime, main, "Foo"));
  Tuple args(&scope, runtime.newTuple(1));
  Layout layout(&scope, type->instanceLayout());
  args->atPut(0, runtime.newInstance(layout));

  // Run __init__ then call the method
  Function test(&scope, moduleAt(&runtime, main, "test"));
  EXPECT_EQ(callFunctionToString(test, args), "testing 123\n321 testing\n");
}

TEST(InstanceAttributeTest, GetDataDescriptor) {
  Runtime runtime;
  const char* src = R"(
class DataDescr:
  def __set__(self, instance, value):
    pass

  def __get__(self, instance, owner):
    return (self, instance, owner)

class Foo:
  pass
)";
  runFromCStr(&runtime, src);

  // Create an instance of the descriptor and store it on the class
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Type descr_type(&scope, moduleAt(&runtime, main, "DataDescr"));
  Object type(&scope, moduleAt(&runtime, main, "Foo"));
  Object attr(&scope, runtime.newStrFromCStr("attr"));
  Layout descr_layout(&scope, descr_type->instanceLayout());
  Object descr(&scope, runtime.newInstance(descr_layout));
  setInTypeDict(&runtime, type, attr, descr);

  // Fetch it from the instance
  Layout instance_layout(&scope, RawType::cast(*type)->instanceLayout());
  Object instance(&scope, runtime.newInstance(instance_layout));
  Tuple result(&scope,
               runtime.attributeAt(Thread::currentThread(), instance, attr));
  ASSERT_EQ(result->length(), 3);
  EXPECT_EQ(runtime.typeOf(result->at(0)), *descr_type);
  EXPECT_EQ(result->at(1), *instance);
  EXPECT_EQ(result->at(2), *type);
}

TEST(InstanceAttributeTest, GetNonDataDescriptor) {
  Runtime runtime;
  const char* src = R"(
class Descr:
  def __get__(self, instance, owner):
    return (self, instance, owner)

class Foo:
  pass
)";
  runFromCStr(&runtime, src);

  // Create an instance of the descriptor and store it on the class
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Type descr_type(&scope, moduleAt(&runtime, main, "Descr"));
  Object type(&scope, moduleAt(&runtime, main, "Foo"));
  Object attr(&scope, runtime.newStrFromCStr("attr"));
  Layout descr_layout(&scope, descr_type->instanceLayout());
  Object descr(&scope, runtime.newInstance(descr_layout));
  setInTypeDict(&runtime, type, attr, descr);

  // Fetch it from the instance
  Layout instance_layout(&scope, RawType::cast(*type)->instanceLayout());
  Object instance(&scope, runtime.newInstance(instance_layout));

  RawObject result =
      runtime.attributeAt(Thread::currentThread(), instance, attr);
  ASSERT_EQ(RawTuple::cast(result)->length(), 3);
  EXPECT_EQ(runtime.typeOf(RawTuple::cast(result)->at(0)), *descr_type);
  EXPECT_EQ(RawTuple::cast(result)->at(1), *instance);
  EXPECT_EQ(RawTuple::cast(result)->at(2), *type);
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
  runFromCStr(&runtime, src);

  // Create the instance
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Type type(&scope, moduleAt(&runtime, main, "Foo"));
  Tuple args(&scope, runtime.newTuple(1));
  Layout layout(&scope, type->instanceLayout());
  args->atPut(0, runtime.newInstance(layout));

  // Run the test
  Function test(&scope, moduleAt(&runtime, main, "test"));
  EXPECT_EQ(callFunctionToString(test, args), "foo bar baz\naaa bbb ccc\n");
}

TEST(InstanceAttributeDeathTest, FetchConditionalInstanceAttribute) {
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
  ASSERT_DEATH(runFromCStr(&runtime, src),
               "aborting due to pending exception: missing attribute");
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
)";
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "True\nTrue\nTrue\nTrue\n");
}

TEST(InstanceAttributeTest, DunderNew) {
  Runtime runtime;
  const char* src = R"(
class Foo:
    def __new__(self):
        print("New")
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
  RawObject attr =
      runtime.attributeAt(Thread::currentThread(), immediate, name);
  ASSERT_TRUE(attr->isBoundMethod());
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
  ASSERT_TRUE(data->isTuple());

  Tuple result(&scope, *data);
  ASSERT_EQ(result->length(), 2);

  Object descr(&scope, moduleAt(&runtime, main, "descr"));
  EXPECT_EQ(result->at(0), *descr);

  Object foo(&scope, moduleAt(&runtime, main, "foo"));
  EXPECT_EQ(result->at(1), *foo);
}

TEST(InstanceAttributeDeletionDeathTest, DeleteUnknownAttribute) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
class Foo:
    pass

foo = Foo()
del foo.bar
)";
  EXPECT_DEATH(compileAndRunToString(&runtime, src), "missing attribute");
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
  ASSERT_TRUE(data->isTuple());

  Tuple result(&scope, *data);
  ASSERT_EQ(result->length(), 2);

  Object foo(&scope, moduleAt(&runtime, main, "foo"));
  EXPECT_EQ(result->at(0), *foo);
  ASSERT_TRUE(result->at(1)->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(result->at(1)), "bar");
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
  ASSERT_TRUE(data->isTuple());

  Tuple result(&scope, *data);
  ASSERT_EQ(result->length(), 2);

  Object bar(&scope, moduleAt(&runtime, main, "bar"));
  EXPECT_EQ(result->at(0), *bar);
  ASSERT_TRUE(result->at(1)->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(result->at(1)), "baz");
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
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object data(&scope, moduleAt(&runtime, main, "args"));
  ASSERT_TRUE(data->isTuple());

  Tuple args(&scope, *data);
  ASSERT_EQ(args->length(), 2);

  Object descr(&scope, moduleAt(&runtime, main, "descr"));
  EXPECT_EQ(args->at(0), *descr);

  Object foo(&scope, moduleAt(&runtime, main, "Foo"));
  EXPECT_EQ(args->at(1), *foo);
}

TEST(ClassAttributeDeletionDeathTest, DeleteUnknownAttribute) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
class Foo:
    pass

del Foo.bar
)";
  EXPECT_DEATH(compileAndRunToString(&runtime, src), "missing attribute");
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
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object data(&scope, moduleAt(&runtime, main, "args"));
  ASSERT_TRUE(data->isTuple());

  Tuple args(&scope, *data);
  ASSERT_EQ(args->length(), 2);

  Object foo(&scope, moduleAt(&runtime, main, "Foo"));
  EXPECT_EQ(args->at(0), *foo);

  Object attr(&scope, runtime.internStrFromCStr("bar"));
  EXPECT_EQ(args->at(1), *attr);
}

TEST(ModuleAttributeDeletionDeathTest, DeleteUnknownAttribute) {
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
  args->atPut(0, *main);
  EXPECT_DEATH(callFunction(test, args), "missing attribute");
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
  args->atPut(0, *main);
  EXPECT_EQ(callFunction(test, args), SmallInt::fromWord(123));

  Object attr(&scope, runtime.newStrFromCStr("foo"));
  Object module(&scope, *main);
  EXPECT_EQ(runtime.attributeAt(Thread::currentThread(), module, attr),
            Error::object());
}

TEST(RuntimeIntTest, NewSmallIntWithDigits) {
  Runtime runtime;
  HandleScope scope;

  Int zero(&scope, runtime.newIntWithDigits(View<uword>(nullptr, 0)));
  ASSERT_TRUE(zero->isSmallInt());
  EXPECT_EQ(zero->asWord(), 0);

  uword digit = 1;
  RawObject one = runtime.newIntWithDigits(View<uword>(&digit, 1));
  ASSERT_TRUE(one->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(one)->value(), 1);

  digit = kMaxUword;
  RawObject negative_one = runtime.newIntWithDigits(View<uword>(&digit, 1));
  ASSERT_TRUE(negative_one->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(negative_one)->value(), -1);

  word min_small_int = RawSmallInt::kMaxValue;
  digit = static_cast<uword>(min_small_int);
  Int min_smallint(&scope, runtime.newIntWithDigits(View<uword>(&digit, 1)));
  ASSERT_TRUE(min_smallint->isSmallInt());
  EXPECT_EQ(min_smallint->asWord(), min_small_int);

  word max_small_int = RawSmallInt::kMaxValue;
  digit = static_cast<uword>(max_small_int);
  Int max_smallint(&scope, runtime.newIntWithDigits(View<uword>(&digit, 1)));
  ASSERT_TRUE(max_smallint->isSmallInt());
  EXPECT_EQ(max_smallint->asWord(), max_small_int);
}

TEST(RuntimeIntTest, NewLargeIntWithDigits) {
  Runtime runtime;
  HandleScope scope;

  word negative_large_int = RawSmallInt::kMinValue - 1;
  uword digit = static_cast<uword>(negative_large_int);
  Int negative_largeint(&scope,
                        runtime.newIntWithDigits(View<uword>(&digit, 1)));
  ASSERT_TRUE(negative_largeint->isLargeInt());
  EXPECT_EQ(negative_largeint->asWord(), negative_large_int);

  word positive_large_int = RawSmallInt::kMaxValue + 1;
  digit = static_cast<uword>(positive_large_int);
  Int positive_largeint(&scope,
                        runtime.newIntWithDigits(View<uword>(&digit, 1)));
  ASSERT_TRUE(positive_largeint->isLargeInt());
  EXPECT_EQ(positive_largeint->asWord(), positive_large_int);
}

TEST(RuntimeIntTest, BinaryAndWithSmallInts) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, SmallInt::fromWord(0xEA));   // 0b11101010
  Int right(&scope, SmallInt::fromWord(0xDC));  // 0b11011100
  Object result(&scope,
                runtime.intBinaryAnd(Thread::currentThread(), left, right));
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*result)->value(), 0xC8);  // 0b11001000
}

TEST(RuntimeIntTest, BinaryAndWithLargeInts) {
  Runtime runtime;
  HandleScope scope;
  // {0b00001111, 0b00110000, 0b00000001}
  Int left(&scope, newIntWithDigits(&runtime, {0x0F, 0x30, 0x1}));
  // {0b00000011, 0b11110000, 0b00000010, 0b00000111}
  Int right(&scope, newIntWithDigits(&runtime, {0x03, 0xF0, 0x2, 0x7}));
  Object result(&scope,
                runtime.intBinaryAnd(Thread::currentThread(), left, right));
  // {0b00000111, 0b01110000}
  Int expected(&scope, newIntWithDigits(&runtime, {0x03, 0x30}));
  ASSERT_TRUE(result->isLargeInt());
  EXPECT_EQ(expected->compare(Int::cast(result)), 0);

  Object result_commuted(
      &scope, runtime.intBinaryAnd(Thread::currentThread(), right, left));
  ASSERT_TRUE(result_commuted->isLargeInt());
  EXPECT_EQ(Int::cast(result)->compare(Int::cast(result_commuted)), 0);
}

TEST(RuntimeIntTest, BinaryAndWithNegativeLargeInts) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, SmallInt::fromWord(-42));  // 0b11010110
  Int right(&scope, newIntWithDigits(&runtime,
                                     {static_cast<uword>(-1), 0xF0, 0x2, 0x7}));
  Object result(&scope,
                runtime.intBinaryAnd(Thread::currentThread(), left, right));
  ASSERT_TRUE(result->isLargeInt());
  Int expected(&scope, newIntWithDigits(&runtime, {static_cast<uword>(-42),
                                                   0xF0, 0x2, 0x7}));
  EXPECT_EQ(expected->compare(Int::cast(result)), 0);
}

TEST(RuntimeIntTest, BinaryOrWithSmallInts) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, SmallInt::fromWord(0xAA));   // 0b10101010
  Int right(&scope, SmallInt::fromWord(0x9C));  // 0b10011100
  Object result(&scope,
                runtime.intBinaryOr(Thread::currentThread(), left, right));
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*result)->value(), 0xBE);  // 0b10111110
}

TEST(RuntimeIntTest, BinaryOrWithLargeInts) {
  Runtime runtime;
  HandleScope scope;
  // {0b00001100, 0b00110000, 0b00000001}
  Int left(&scope, newIntWithDigits(&runtime, {0x0C, 0x30, 0x1}));
  // {0b00000011, 0b11010000, 0b00000010, 0b00000111}
  Int right(&scope, newIntWithDigits(&runtime, {0x03, 0xD0, 0x2, 0x7}));
  Object result(&scope,
                runtime.intBinaryOr(Thread::currentThread(), left, right));
  // {0b00001111, 0b11110000, 0b00000011, 0b00000111}
  Int expected(&scope, newIntWithDigits(&runtime, {0x0F, 0xF0, 0x3, 0x7}));
  ASSERT_TRUE(result->isLargeInt());
  EXPECT_EQ(expected->compare(Int::cast(result)), 0);

  Object result_commuted(
      &scope, runtime.intBinaryOr(Thread::currentThread(), right, left));
  ASSERT_TRUE(result_commuted->isLargeInt());
  EXPECT_EQ(Int::cast(result)->compare(Int::cast(result_commuted)), 0);
}

TEST(RuntimeIntTest, BinaryOrWithNegativeLargeInts) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, SmallInt::fromWord(-42));  // 0b11010110
  Int right(&scope, newIntWithDigits(&runtime, {static_cast<uword>(-4), 0xF0,
                                                0x2, static_cast<uword>(-1)}));
  Object result(&scope,
                runtime.intBinaryOr(Thread::currentThread(), left, right));
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(result)->value(), -2);
}

TEST(RuntimeIntTest, BinaryXorWithSmallInts) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, SmallInt::fromWord(0xAA));   // 0b10101010
  Int right(&scope, SmallInt::fromWord(0x9C));  // 0b10011100
  Object result(&scope,
                runtime.intBinaryXor(Thread::currentThread(), left, right));
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*result)->value(), 0x36);  // 0b00110110
}

TEST(RuntimeIntTest, BinaryXorWithLargeInts) {
  Runtime runtime;
  HandleScope scope;
  // {0b00001100, 0b00110000, 0b00000001}
  Int left(&scope, newIntWithDigits(&runtime, {0x0C, 0x30, 0x1}));
  // {0b00000011, 0b11010000, 0b00000010, 0b00000111}
  Int right(&scope, newIntWithDigits(&runtime, {0x03, 0xD0, 0x2, 0x7}));
  Object result(&scope,
                runtime.intBinaryXor(Thread::currentThread(), left, right));
  // {0b00001111, 0b11100000, 0b00000011, 0b00000111}
  Int expected(&scope, newIntWithDigits(&runtime, {0x0F, 0xE0, 0x3, 0x7}));
  ASSERT_TRUE(result->isLargeInt());
  EXPECT_EQ(expected->compare(Int::cast(result)), 0);

  Object result_commuted(
      &scope, runtime.intBinaryXor(Thread::currentThread(), right, left));
  ASSERT_TRUE(result_commuted->isLargeInt());
  EXPECT_EQ(Int::cast(result)->compare(Int::cast(result_commuted)), 0);
}

TEST(RuntimeIntTest, BinaryXorWithNegativeLargeInts) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, SmallInt::fromWord(-42));  // 0b11010110
  Int right(&scope, newIntWithDigits(&runtime, {static_cast<uword>(-1), 0xf0,
                                                0x2, static_cast<uword>(-1)}));
  Object result(&scope,
                runtime.intBinaryXor(Thread::currentThread(), left, right));
  Int expected(&scope,
               newIntWithDigits(&runtime, {0x29, ~static_cast<uword>(0xF0),
                                           ~static_cast<uword>(0x2), 0}));
  ASSERT_TRUE(result->isLargeInt());
  EXPECT_EQ(expected->compare(Int::cast(result)), 0);
}

TEST(RuntimeIntTest, NormalizeLargeIntToSmallInt) {
  Runtime runtime;
  HandleScope scope;

  LargeInt lint_42(&scope, newLargeIntWithDigits({42}));
  Object norm_42(&scope, runtime.normalizeLargeInt(lint_42));
  ASSERT_TRUE(norm_42->isSmallInt());
  EXPECT_EQ(SmallInt::cast(norm_42)->value(), 42);

  LargeInt lint_neg1(&scope, newLargeIntWithDigits({uword(-1)}));
  Object norm_neg1(&scope, runtime.normalizeLargeInt(lint_neg1));
  ASSERT_TRUE(norm_neg1->isSmallInt());
  EXPECT_EQ(SmallInt::cast(norm_neg1)->value(), -1);

  LargeInt lint_min(&scope,
                    newLargeIntWithDigits({uword(RawSmallInt::kMinValue)}));
  Object norm_min(&scope, runtime.normalizeLargeInt(lint_min));
  ASSERT_TRUE(norm_min->isSmallInt());
  EXPECT_EQ(SmallInt::cast(norm_min)->value(), RawSmallInt::kMinValue);

  LargeInt lint_max(&scope, newLargeIntWithDigits({RawSmallInt::kMaxValue}));
  Object norm_max(&scope, runtime.normalizeLargeInt(lint_max));
  ASSERT_TRUE(norm_max->isSmallInt());
  EXPECT_EQ(SmallInt::cast(norm_max)->value(), RawSmallInt::kMaxValue);

  LargeInt lint_sext_neg_4(&scope,
                           newLargeIntWithDigits({uword(-4), kMaxUword}));
  Object norm_neg_4(&scope, runtime.normalizeLargeInt(lint_sext_neg_4));
  ASSERT_TRUE(norm_neg_4->isSmallInt());
  EXPECT_EQ(SmallInt::cast(norm_neg_4)->value(), -4);

  LargeInt lint_sext_neg_13(
      &scope,
      newLargeIntWithDigits({uword(-13), kMaxUword, kMaxUword, kMaxUword}));
  Object norm_neg_13(&scope, runtime.normalizeLargeInt(lint_sext_neg_13));
  ASSERT_TRUE(norm_neg_13->isSmallInt());
  EXPECT_EQ(SmallInt::cast(norm_neg_13)->value(), -13);

  LargeInt lint_zext_66(&scope, newLargeIntWithDigits({66, 0}));
  Object norm_66(&scope, runtime.normalizeLargeInt(lint_zext_66));
  ASSERT_TRUE(norm_66->isSmallInt());
  EXPECT_EQ(SmallInt::cast(norm_66)->value(), 66);
}

TEST(RuntimeIntTest, NormalizeLargeIntToLargeInt) {
  Runtime runtime;
  HandleScope scope;

  LargeInt lint_max(&scope, newLargeIntWithDigits({kMaxWord}));
  Object norm_max(&scope, runtime.normalizeLargeInt(lint_max));
  ASSERT_TRUE(norm_max->isLargeInt());
  EXPECT_EQ(RawLargeInt::cast(norm_max)->asWord(), kMaxWord);

  LargeInt lint_min(&scope, newLargeIntWithDigits({uword(kMinWord)}));
  Object norm_min(&scope, runtime.normalizeLargeInt(lint_min));
  ASSERT_TRUE(norm_min->isLargeInt());
  EXPECT_EQ(RawLargeInt::cast(norm_min)->asWord(), kMinWord);

  LargeInt lint_max_sub_7_zext(&scope,
                               newLargeIntWithDigits({kMaxWord - 7, 0, 0}));
  Object norm_max_sub_7(&scope, runtime.normalizeLargeInt(lint_max_sub_7_zext));
  ASSERT_TRUE(norm_max_sub_7->isLargeInt());
  EXPECT_EQ(RawLargeInt::cast(norm_max_sub_7)->asWord(), kMaxWord - 7);

  LargeInt lint_min_plus_9_sext(
      &scope, newLargeIntWithDigits({uword(kMinWord) + 9, kMaxUword}));
  Object norm_min_plus_9(&scope,
                         runtime.normalizeLargeInt(lint_min_plus_9_sext));
  ASSERT_TRUE(norm_min_plus_9->isLargeInt());
  EXPECT_EQ(RawLargeInt::cast(norm_min_plus_9)->asWord(), kMinWord + 9);

  LargeInt lint_no_sext(&scope, newLargeIntWithDigits({0, kMaxUword}));
  Object norm_no_sext(&scope, runtime.normalizeLargeInt(lint_no_sext));
  ASSERT_TRUE(norm_no_sext->isLargeInt());
  EXPECT_EQ(RawLargeInt::cast(norm_no_sext)->numDigits(), 2);
  EXPECT_EQ(RawLargeInt::cast(norm_no_sext)->digitAt(0), uword{0});
  EXPECT_EQ(RawLargeInt::cast(norm_no_sext)->digitAt(1), kMaxUword);

  LargeInt lint_no_zext(&scope, newLargeIntWithDigits({kMaxUword, 0}));
  Object norm_no_zext(&scope, runtime.normalizeLargeInt(lint_no_zext));
  ASSERT_TRUE(norm_no_zext->isLargeInt());
  EXPECT_EQ(RawLargeInt::cast(norm_no_zext)->numDigits(), 2);
  EXPECT_EQ(RawLargeInt::cast(norm_no_zext)->digitAt(0), kMaxUword);
  EXPECT_EQ(RawLargeInt::cast(norm_no_zext)->digitAt(1), uword{0});
}

TEST(RuntimeIntTest, BinaryLshiftWithPositive) {
  Runtime runtime;
  HandleScope scope;
  // 2 << 3 = 16
  Int num(&scope, SmallInt::fromWord(2));
  Object result(&scope,
                runtime.intBinaryLshift(Thread::currentThread(), num, 3));
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*result)->value(), 16);
}

TEST(RuntimeIntTest, BinaryLshiftWithNegative) {
  Runtime runtime;
  HandleScope scope;
  // -2 << 1 = -4
  Int num(&scope, SmallInt::fromWord(-2));
  Object result(&scope,
                runtime.intBinaryLshift(Thread::currentThread(), num, 1));
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*result)->value(), -4);
}

TEST(RuntimeIntTest, BinaryLshiftWithZero) {
  Runtime runtime;
  HandleScope scope;
  // 0 << x = 0
  Int zero(&scope, SmallInt::fromWord(0));
  Object result(&scope,
                runtime.intBinaryLshift(Thread::currentThread(), zero, 123));
  EXPECT_EQ(result, zero);
}

TEST(RuntimeIntTest, BinaryLshiftReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  // (SmallInt::max >> 2) << 2 = SmallInt::max with last two bits zeroed
  Int max(&scope, SmallInt::fromWord(SmallInt::kMaxValue >> 2));
  Object result(&scope, runtime.intBinaryLshift(thread, max, 2));
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(SmallInt::cast(result)->value(), SmallInt::kMaxValue & ~0x3);

  // (SmallInt::min >> 2) << 2 = SmallInt::min with last two bits zeroed
  Int min(&scope, SmallInt::fromWord(SmallInt::kMinValue >> 2));
  result = runtime.intBinaryLshift(thread, min, 2);
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(SmallInt::cast(result)->value(), SmallInt::kMinValue & ~0x3);
}

TEST(RuntimeIntTest, BinaryLshiftFitsOneWord) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  // Shift a 1 to the second most significant bit, verify result has 1 word
  Int num(&scope, SmallInt::fromWord(4));  // 0b100
  Int result(&scope, runtime.intBinaryLshift(thread, num, kBitsPerWord - 4));
  ASSERT_EQ(result->numDigits(), 1);
  EXPECT_EQ(result->digitAt(0), uword{4} << (kBitsPerWord - 4));

  // Same for negative - shift 0 to second most significant bit
  num = Int::cast(SmallInt::fromWord(~4));
  result = runtime.intBinaryLshift(thread, num, kBitsPerWord - 4);
  ASSERT_EQ(result->numDigits(), 1);
  EXPECT_EQ(result->digitAt(0), ~uword{4} << (kBitsPerWord - 4));
}

TEST(RuntimeIntTest, BinaryLshiftDoesNotFitInOneWord) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  // Test that when we shift 1 into the highest significant bit of the first
  // word (sign bit), an extra word is added to preserve the sign
  // 0100 << 1 = 0000 1000
  Int num(&scope, SmallInt::fromWord(4));  // 0b0100
  Int result(&scope, runtime.intBinaryLshift(thread, num, kBitsPerWord - 3));
  ASSERT_EQ(result->numDigits(), 2);
  EXPECT_EQ(result->digitAt(0), uword{4} << (kBitsPerWord - 3));
  EXPECT_EQ(result->digitAt(1), 0U);

  // Same for negative, shifting 0 into the highest significant bit
  // 1011 << 1 = 1111 0110
  num = Int::cast(SmallInt::fromWord(~4));  // 0b0100
  result = runtime.intBinaryLshift(thread, num, kBitsPerWord - 3);
  ASSERT_EQ(result->numDigits(), 2);
  EXPECT_EQ(result->digitAt(0), ~uword{4} << (kBitsPerWord - 3));
  EXPECT_EQ(result->digitAt(1), kMaxUword);
}

TEST(RuntimeIntTest, BinaryLshiftWithLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  // shift a positive number by 2 words + 2
  // 0001 0001 << 10 = 0100 0100 0000 0000
  Int num(&scope, testing::newIntWithDigits(&runtime, {1, 1}));
  Int result(&scope,
             runtime.intBinaryLshift(thread, num, 2 * kBitsPerWord + 2));
  ASSERT_EQ(result->numDigits(), 4);
  EXPECT_EQ(result->digitAt(0), uword{0});
  EXPECT_EQ(result->digitAt(1), uword{0});
  EXPECT_EQ(result->digitAt(2), uword{4});
  EXPECT_EQ(result->digitAt(3), uword{4});

  // shift a negative number by 2 words + 2
  // 1110 1110 << 10 = 1011 1000 0000 0000
  num = Int::cast(
      testing::newIntWithDigits(&runtime, {kMaxUword - 1, kMaxUword - 1}));
  result = runtime.intBinaryLshift(thread, num, 2 * kBitsPerWord + 2);
  ASSERT_EQ(result->numDigits(), 4);
  EXPECT_EQ(result->digitAt(0), uword{0});
  EXPECT_EQ(result->digitAt(1), uword{0});
  EXPECT_EQ(result->digitAt(2), kMaxUword - 7);  // kMaxUword - 7 == -8
  EXPECT_EQ(result->digitAt(3), kMaxUword - 4);  // kMaxUword - 4 == -5
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
  Layout layout(&scope, type->instanceLayout());
  HeapObject instance(&scope, runtime.newInstance(layout));
  Object attr(&scope, runtime.newStrFromCStr("unknown"));
  EXPECT_EQ(runtime.instanceDel(Thread::currentThread(), instance, attr),
            Error::object());
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
  Layout layout(&scope, runtime.layoutAt(instance->header()->layoutId()));
  Object attr(&scope, runtime.internStrFromCStr("bar"));
  AttributeInfo info;
  Thread* thread = Thread::currentThread();
  ASSERT_TRUE(runtime.layoutFindAttribute(thread, layout, attr, &info));
  ASSERT_TRUE(info.isInObject());

  // After successful deletion, the instance should have a new layout and should
  // no longer reference the previous value
  EXPECT_EQ(runtime.instanceDel(thread, instance, attr), NoneType::object());
  Layout new_layout(&scope, runtime.layoutAt(instance->header()->layoutId()));
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
  Layout layout(&scope, runtime.layoutAt(instance->header()->layoutId()));
  Object attr(&scope, runtime.internStrFromCStr("bar"));
  AttributeInfo info;
  Thread* thread = Thread::currentThread();
  ASSERT_TRUE(runtime.layoutFindAttribute(thread, layout, attr, &info));
  ASSERT_TRUE(info.isOverflow());

  // After successful deletion, the instance should have a new layout and should
  // no longer reference the previous value
  EXPECT_EQ(runtime.instanceDel(thread, instance, attr), NoneType::object());
  Layout new_layout(&scope, runtime.layoutAt(instance->header()->layoutId()));
  EXPECT_NE(*layout, *new_layout);
  EXPECT_FALSE(runtime.layoutFindAttribute(thread, new_layout, attr, &info));
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
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));

  Object foo(&scope, moduleAt(&runtime, main, "Foo"));
  EXPECT_TRUE(foo->isType());

  Object bar(&scope, moduleAt(&runtime, main, "Bar"));
  EXPECT_TRUE(bar->isType());
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
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));

  Object foo(&scope, moduleAt(&runtime, main, "Foo"));
  EXPECT_FALSE(foo->isType());
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
  runFromCStr(&runtime, src);
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
  runFromCStr(&runtime, src);
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
  runFromCStr(&runtime, src);
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
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object mymeta(&scope, moduleAt(&runtime, main, "MyMeta"));
  Object foo(&scope, moduleAt(&runtime, main, "Foo"));
  EXPECT_EQ(runtime.typeOf(*foo), *mymeta);
  EXPECT_FALSE(foo->isType());
  EXPECT_TRUE(runtime.isInstanceOfType(*foo));
}

TEST(ImportlibTest, SysMetaPathIsList) {
  const char* src = R"(
import sys

meta_path = sys.meta_path
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object meta_path(&scope, moduleAt(&runtime, main, "meta_path"));
  ASSERT_TRUE(meta_path->isList());
}

TEST(SubclassingTest, SubclassBuiltinSubclass) {
  const char* src = R"(
class Test(Exception):
  pass
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object value(&scope, moduleAt(&runtime, main, "Test"));
  ASSERT_TRUE(value->isType());

  Type type(&scope, *value);
  ASSERT_TRUE(type->mro()->isTuple());

  Tuple mro(&scope, type->mro());
  ASSERT_EQ(mro->length(), 4);
  EXPECT_EQ(mro->at(0), *type);
  EXPECT_EQ(mro->at(1), runtime.typeAt(LayoutId::kException));
  EXPECT_EQ(mro->at(2), runtime.typeAt(LayoutId::kBaseException));
  EXPECT_EQ(mro->at(3), runtime.typeAt(LayoutId::kObject));
}

TEST(ModuleImportTest, ModuleImportsAllPublicSymbols) {
  Runtime runtime;
  HandleScope scope;

  // Create Module
  Object name(&scope, runtime.newStrFromCStr("foo"));
  Module module(&scope, runtime.newModule(name));

  // Add symbols
  Dict module_dict(&scope, module->dict());
  Object symbol_str1(&scope, runtime.newStrFromCStr("public_symbol"));
  Object symbol_str2(&scope, runtime.newStrFromCStr("_private_symbol"));
  runtime.dictAtPutInValueCell(module_dict, symbol_str1, symbol_str1);
  runtime.dictAtPutInValueCell(module_dict, symbol_str2, symbol_str2);

  // Import public symbols to dictionary
  Dict symbols_dict(&scope, runtime.newDict());
  runtime.moduleImportAllFrom(symbols_dict, module);
  EXPECT_EQ(symbols_dict->numItems(), 1);

  ValueCell result(&scope, runtime.dictAt(symbols_dict, symbol_str1));
  EXPECT_PYSTRING_EQ(RawStr::cast(result->value()), "public_symbol");
}

TEST(HeapFrameTest, Create) {
  const char* src = R"(
def gen():
  yield 12
)";

  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Object gen(&scope, moduleAt(&runtime, "__main__", "gen"));
  ASSERT_TRUE(gen->isFunction());
  Code code(&scope, RawFunction::cast(*gen)->code());
  Object frame_obj(&scope, runtime.newHeapFrame(code));
  ASSERT_TRUE(frame_obj->isHeapFrame());
  HeapFrame heap_frame(&scope, *frame_obj);
  EXPECT_EQ(heap_frame->maxStackSize(), code->stacksize());
}

extern "C" struct _inittab _PyImport_Inittab[];

TEST(ModuleImportTest, ImportModuleFromInitTab) {
  Runtime runtime;
  runFromCStr(&runtime, "import _empty");
  HandleScope scope;
  Object mod(&scope, moduleAt(&runtime, "__main__", "_empty"));
  EXPECT_TRUE(mod->isModule());
}

TEST(ModuleTest, NewModuleSetsDictValues) {
  Runtime runtime;
  HandleScope scope;

  // Create Module
  Object name(&scope, runtime.newStrFromCStr("mymodule"));
  Module module(&scope, runtime.newModule(name));

  Str mod_name(&scope, moduleAt(&runtime, module, "__name__"));
  EXPECT_TRUE(mod_name->equalsCStr("mymodule"));
  EXPECT_EQ(moduleAt(&runtime, module, "__doc__"), NoneType::object());
  EXPECT_EQ(moduleAt(&runtime, module, "__package__"), NoneType::object());
  EXPECT_EQ(moduleAt(&runtime, module, "__loader__"), NoneType::object());
  EXPECT_EQ(moduleAt(&runtime, module, "__spec__"), NoneType::object());
}

TEST(FunctionAttrTest, SetAttribute) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
def foo(): pass
foo.x = 3
)");
  HandleScope scope;
  Function function(&scope, moduleAt(&runtime, "__main__", "foo"));
  Dict function_dict(&scope, function->dict());
  Object key(&scope, runtime.newStrFromCStr("x"));
  Object value(&scope, runtime.dictAt(function_dict, key));
  ASSERT_TRUE(value->isInt());
  EXPECT_EQ(Int::cast(value)->asWord(), 3);
}

TEST(FunctionAttrTest, GetAttribute) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
def foo(): pass
foo.x = 3
value = foo.x
)");
  HandleScope scope;
  Object value(&scope, moduleAt(&runtime, "__main__", "value"));
  ASSERT_TRUE(value->isInt());
  EXPECT_EQ(Int::cast(value)->asWord(), 3);
}

TEST(FunctionAttrTest, GetAttributePrefersBuiltinAttributesOverDict) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
def foo(): pass
foo.__doc__ = "bar"
foo.__dict__['__doc__'] = "baz"
value = foo.__doc__
)");
  HandleScope scope;
  Object value(&scope, moduleAt(&runtime, "__main__", "value"));
  ASSERT_TRUE(value->isStr());
  EXPECT_TRUE(RawStr::cast(*value)->equalsCStr("bar"));
}

TEST(FunctionAttrDeathTest, GetInvalidAttribute) {
  Runtime runtime;
  EXPECT_DEATH(runFromCStr(&runtime, R"(
def foo(): pass
value = foo.x
)"),
               "missing attr");
}

TEST(RuntimeTest, LazilyInitializationOfFunctionDict) {
  Runtime runtime;
  HandleScope scope;
  Function function(&scope, runtime.newFunction());
  ASSERT_TRUE(function->dict()->isNoneType());

  Object dict_name(&scope, runtime.newStrFromCStr("__dict__"));
  runtime.attributeAt(Thread::currentThread(), function, dict_name);
  EXPECT_TRUE(function->dict()->isDict());
}

TEST(RuntimeTest, SetFunctionDict) {
  Runtime runtime;
  HandleScope scope;
  Function function(&scope, runtime.newFunction());
  ASSERT_TRUE(function->dict()->isNoneType());

  Object dict_name(&scope, runtime.newStrFromCStr("__dict__"));
  Object dict(&scope, runtime.newDict());
  runtime.attributeAtPut(Thread::currentThread(), function, dict_name, dict);

  Object result(&scope, runtime.attributeAt(Thread::currentThread(), function,
                                            dict_name));
  EXPECT_TRUE(result->isDict());
  EXPECT_EQ(*dict, *result);
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
  varnames->atPut(0, *foo);
  varnames->atPut(1, *bar);
  varnames->atPut(2, *baz);
  cellvars->atPut(0, *foobar);
  cellvars->atPut(1, *foobaz);
  Object none(&scope, NoneType::object());
  Tuple empty_tuple(&scope, runtime.newTuple(0));
  Code code(&scope, runtime.newCode(argcount, kwargcount, 0, 0, 0, none, none,
                                    none, varnames, empty_tuple, cellvars, none,
                                    none, 0, none));
  EXPECT_TRUE(code->cell2arg()->isNoneType());
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
  varnames->atPut(0, *foo);
  varnames->atPut(1, *bar);
  varnames->atPut(2, *baz);
  cellvars->atPut(0, *baz);
  cellvars->atPut(1, *foobar);
  Object none(&scope, NoneType::object());
  Tuple empty_tuple(&scope, runtime.newTuple(0));
  Code code(&scope, runtime.newCode(argcount, kwargcount, 0, 0, 0, none, none,
                                    none, varnames, empty_tuple, cellvars, none,
                                    none, 0, none));
  ASSERT_FALSE(code->cell2arg()->isNoneType());
  Tuple cell2arg(&scope, code->cell2arg());
  ASSERT_EQ(cell2arg->length(), 2);

  ASSERT_TRUE(cell2arg->at(0)->isInt());
  Int cell2arg_value(&scope, cell2arg->at(0));
  EXPECT_EQ(cell2arg_value->asWord(), 2);
  EXPECT_EQ(cell2arg->at(1), NoneType::object());
}

// Set is not special except that it is a builtin type with sealed attributes.
TEST(RuntimeTest, SetHasSameSizeCreatedTwoDifferentWays) {
  Runtime runtime;
  HandleScope scope;
  Layout layout(&scope, runtime.layoutAt(LayoutId::kSet));
  Set set1(&scope, runtime.newInstance(layout));
  Set set2(&scope, runtime.newSet());
  EXPECT_EQ(set1->size(), set2->size());
}

// Set is not special except that it is a builtin type with sealed attributes.
TEST(RuntimeTest, SealedClassLayoutDoesNotHaveSpaceForOverflowAttributes) {
  Runtime runtime;
  HandleScope scope;
  Layout layout(&scope, runtime.layoutAt(LayoutId::kSet));
  EXPECT_TRUE(layout->overflowAttributes().isNoneType());
  word expected_set_size = kPointerSize * layout->numInObjectAttributes();
  EXPECT_EQ(layout->instanceSize(), expected_set_size);
}

TEST(RuntimeTest, SettingNewAttributeOnSealedClassThrows) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Str attr(&scope, runtime.newStrFromCStr("attr"));
  Str value(&scope, runtime.newStrFromCStr("value"));
  Thread* thread = Thread::currentThread();
  Object result(&scope, runtime.instanceAtPut(thread, set, attr, value));
  ASSERT_TRUE(result->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kAttributeError));
  EXPECT_TRUE(thread->pendingExceptionValue().isStr());
}

// Exception attributes can be set on the fly.
TEST(RuntimeTest, NonSealedClassHasSpaceForOverflowAttrbutes) {
  Runtime runtime;
  HandleScope scope;
  Layout layout(&scope, runtime.layoutAt(LayoutId::kMemoryError));
  EXPECT_TRUE(layout->overflowAttributes().isTuple());
  EXPECT_EQ(
      layout->instanceSize(),
      (layout->numInObjectAttributes() + 1) * kPointerSize);  // 1=overflow
}

// User-defined class attributes can be set on the fly.
TEST(RuntimeTest, UserCanSetOverflowAttributeOnUserDefinedClass) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class C(): pass
a = C()
)");
  HandleScope scope;
  HeapObject a(&scope, moduleAt(&runtime, "__main__", "a"));
  Str attr(&scope, runtime.newStrFromCStr("attr"));
  Str value(&scope, runtime.newStrFromCStr("value"));
  Object result(&scope,
                runtime.instanceAtPut(Thread::currentThread(), a, attr, value));
  ASSERT_FALSE(result->isError());
  EXPECT_EQ(runtime.instanceAt(Thread::currentThread(), a, attr), *value);
}

TEST(RuntimeTest, IsMappingReturnsFalseOnSet) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  EXPECT_FALSE(runtime.isMapping(Thread::currentThread(), set));
}

TEST(RuntimeTest, IsMappingReturnsTrueOnDict) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  EXPECT_TRUE(runtime.isMapping(Thread::currentThread(), dict));
}

TEST(RuntimeTest, IsMappingReturnsTrueOnList) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, runtime.newList());
  EXPECT_TRUE(runtime.isMapping(Thread::currentThread(), list));
}

TEST(RuntimeTest, IsMappingReturnsTrueOnCustomClassWithMethod) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class C():
  def __getitem__(self, key):
    pass
o = C()
)");
  HandleScope scope;
  Object obj(&scope, moduleAt(&runtime, "__main__", "o"));
  EXPECT_TRUE(runtime.isMapping(Thread::currentThread(), obj));
}

TEST(RuntimeTest, IsMappingWithClassAttrNotCallableReturnsTrue) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class C():
  __getitem__ = 4
o = C()
)");
  HandleScope scope;
  Object obj(&scope, moduleAt(&runtime, "__main__", "o"));
  EXPECT_TRUE(runtime.isMapping(Thread::currentThread(), obj));
}

TEST(RuntimeTest, IsMappingReturnsFalseOnCustomClassWithoutMethod) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class C():
  pass
o = C()
)");
  HandleScope scope;
  Object obj(&scope, moduleAt(&runtime, "__main__", "o"));
  EXPECT_FALSE(runtime.isMapping(Thread::currentThread(), obj));
}

TEST(RuntimeTest, IsMappingWithInstanceAttrReturnsFalse) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class C():
  pass
o = C()
o.__getitem__ = 4
)");
  HandleScope scope;
  Object obj(&scope, moduleAt(&runtime, "__main__", "o"));
  EXPECT_FALSE(runtime.isMapping(Thread::currentThread(), obj));
}

}  // namespace python
